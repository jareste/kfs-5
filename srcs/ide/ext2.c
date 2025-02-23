#include "ext2.h"
#include "../ide/ide.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"
#include "../kshell/kshell.h"
#include "../display/display.h"


/* We define IDE_SECTOR_SIZE here (if not defined in ide.h) */
#ifndef IDE_SECTOR_SIZE
#define IDE_SECTOR_SIZE 512
#endif

/* Declare external IDE functions.
   Ensure these are implemented in your IDE module. */
extern int ide_read_sectors(uint32_t lba, uint8_t count, void *buffer);
extern int ide_write_sectors(uint32_t lba, uint8_t count, void *buffer);

/* --- Derived constants --- */
#define SECTORS_PER_BLOCK (EXT2_BLOCK_SIZE / IDE_SECTOR_SIZE)

/* --- Global FS structure --- */
struct ext2_fs
{
    struct ext2_super_block sb;
    struct ext2_group_desc  gd;
    uint8_t* inode_bitmap;  /* one block */
    uint8_t* block_bitmap;  /* one block */
} ext2;

static uint32_t current_dir = EXT2_ROOT_INODE;  /* current working directory inode */

static void ext2_read_block(uint32_t block, void *buf)
{
    uint32_t lba = EXT2_PARTITION_START + block * SECTORS_PER_BLOCK;
    if (ide_read_sectors(lba, SECTORS_PER_BLOCK, buf) < 0)
        kernel_panic("ext2: read block error");
}

static void ext2_write_block(uint32_t block, void *buf)
{
    uint32_t lba = EXT2_PARTITION_START + block * SECTORS_PER_BLOCK;
    if (ide_write_sectors(lba, SECTORS_PER_BLOCK, buf) < 0)
        kernel_panic("ext2: write block error");
}

static void ext2_read_inode(uint32_t inode_num, struct ext2_inode *inode)
{
    uint32_t index = inode_num - 1;
    uint32_t block_offset = (index * EXT2_INODE_SIZE) / EXT2_BLOCK_SIZE;
    uint32_t offset_in_block = (index * EXT2_INODE_SIZE) % EXT2_BLOCK_SIZE;
    uint8_t* block = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(ext2.gd.bg_inode_table + block_offset, block);
    memcpy(inode, block + offset_in_block, sizeof(struct ext2_inode));
    kfree(block);
}

static void ext2_write_inode(uint32_t inode_num, struct ext2_inode *inode) {
    uint32_t index = inode_num - 1;
    uint32_t block_offset = (index * EXT2_INODE_SIZE) / EXT2_BLOCK_SIZE;
    uint32_t offset_in_block = (index * EXT2_INODE_SIZE) % EXT2_BLOCK_SIZE;
    uint8_t* block = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(ext2.gd.bg_inode_table + block_offset, block);
    memcpy(block + offset_in_block, inode, sizeof(struct ext2_inode));
    ext2_write_block(ext2.gd.bg_inode_table + block_offset, block);
    kfree(block);
}

static uint32_t ext2_allocate_inode(void)
{
    uint32_t total = ext2.sb.s_inodes_count;
    for (uint32_t i = 0; i < total; i++)
    {
        uint32_t byte = i / 8;
        uint8_t bit = 1 << (i % 8);
        if (!(ext2.inode_bitmap[byte] & bit))
        {
            ext2.inode_bitmap[byte] |= bit;
            ext2_write_block(ext2.gd.bg_inode_bitmap, ext2.inode_bitmap);
            return i + 1;
        }
    }
    return 0;
}

static uint32_t ext2_allocate_block(void)
{
    uint32_t total = ext2.sb.s_blocks_count;
    for (uint32_t i = 0; i < total; i++)
    {
        uint32_t byte = i / 8;
        uint8_t bit = 1 << (i % 8);
        if (!(ext2.block_bitmap[byte] & bit))
        {
            ext2.block_bitmap[byte] |= bit;
            ext2_write_block(ext2.gd.bg_block_bitmap, ext2.block_bitmap);
            return i + 1;
        }
    }
    return 0;
}

static void ext2_free_inode(uint32_t inode_num)
{
    uint32_t index = inode_num - 1;
    uint32_t byte = index / 8;
    uint8_t bit = 1 << (index % 8);
    ext2.inode_bitmap[byte] &= ~bit;
    ext2_write_block(ext2.gd.bg_inode_bitmap, ext2.inode_bitmap);
}

static void ext2_free_block(uint32_t block)
{
    uint32_t index = block - 1;
    uint32_t byte = index / 8;
    uint8_t bit = 1 << (index % 8);
    ext2.block_bitmap[byte] &= ~bit;
    ext2_write_block(ext2.gd.bg_block_bitmap, ext2.block_bitmap);
}

static int ext2_add_dir_entry(uint32_t parent_inode_num, const char *name,
                              uint32_t inode_num, uint8_t file_type)
{
    struct ext2_inode parent;
    ext2_read_inode(parent_inode_num, &parent);
    uint32_t block;
    if (parent.i_block[0] == 0)
    {
        block = ext2_allocate_block();
        if (block == 0)
        {
            printf("No free block available\n");
            return -1;
        }
        parent.i_block[0] = block;
        parent.i_blocks = 2;  /* block count in 512-byte units */
        parent.i_size = EXT2_BLOCK_SIZE;
        ext2_write_inode(parent_inode_num, &parent);
        uint8_t* buf = kmalloc(EXT2_BLOCK_SIZE);
        memset(buf, 0, EXT2_BLOCK_SIZE);
        ext2_write_block(block, buf);
        kfree(buf);
    }
    else
    {
        block = parent.i_block[0];
    }
    
    uint8_t* blkbuf = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(block, blkbuf);
    int offset = 0;
    struct ext2_dir_entry *de;
    while (offset < EXT2_BLOCK_SIZE)
    {
        de = (struct ext2_dir_entry *)(blkbuf + offset);
        if (de->inode == 0)
            break;
        /* compute ideal length for this entry */
        int ideal = ((8 + de->name_len + 3) / 4) * 4;
        int avail = de->rec_len - ideal;
        if (avail >= ((8 + strlen(name) + 3) / 4) * 4)
        {
            de->rec_len = ideal;
            offset += ideal;
            de = (struct ext2_dir_entry *)(blkbuf + offset);
            de->inode = inode_num;
            de->rec_len = avail;
            de->name_len = strlen(name);
            de->file_type = file_type;
            memcpy(de->name, name, de->name_len);
            break;
        }
        offset += de->rec_len;
    }
    if (offset >= EXT2_BLOCK_SIZE)
    {
        printf("Directory full, cannot add entry\n");
        kfree(blkbuf);
        return -1;
    }
    ext2_write_block(block, blkbuf);
    kfree(blkbuf);
    return 0;
}

static int ext2_remove_dir_entry(uint32_t parent_inode_num, const char *name)
{
    struct ext2_inode parent;
    ext2_read_inode(parent_inode_num, &parent);
    uint8_t* blkbuf = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(parent.i_block[0], blkbuf);
    int offset = 0;
    struct ext2_dir_entry *prev = NULL, *de = NULL;
    int found = 0;
    while (offset < EXT2_BLOCK_SIZE)
    {
        de = (struct ext2_dir_entry *)(blkbuf + offset);
        if (de->inode != 0)
        {
            char tmp[256];
            memcpy(tmp, de->name, de->name_len);
            tmp[de->name_len] = '\0';
            if (strcmp(tmp, name) == 0) { found = 1; break; }
        }
        prev = de;
        offset += de->rec_len;
    }
    if (!found)
    {
        kfree(blkbuf);
        return -1;
    }
    if (prev == NULL)
    {
        de->inode = 0;
    }
    else
    {
        prev->rec_len += de->rec_len;
    }
    ext2_write_block(parent.i_block[0], blkbuf);
    kfree(blkbuf);
    return 0;
}

static int ext2_lookup(uint32_t dir_inode_num, const char *name, uint32_t *child)
{
    struct ext2_inode dir;
    ext2_read_inode(dir_inode_num, &dir);
    if (!(dir.i_mode & 0x4000))
    {  /* not a directory */
        printf("Not a directory\n");
        return -1;
    }
    uint8_t* blkbuf = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(dir.i_block[0], blkbuf);
    int offset = 0;
    struct ext2_dir_entry *de;
    while (offset < EXT2_BLOCK_SIZE)
    {
        de = (struct ext2_dir_entry *)(blkbuf + offset);
        if (de->inode != 0)
        {
            char tmp[256];
            memcpy(tmp, de->name, de->name_len);
            tmp[de->name_len] = '\0';
            if (strcmp(tmp, name) == 0)
            {
                *child = de->inode;
                kfree(blkbuf);
                return 0;
            }
        }
        offset += de->rec_len;
    }
    kfree(blkbuf);
    return -1;
}

static int ext2_resolve_path(const char *path, uint32_t *inode_out)
{
    uint32_t cur = (path[0]=='/') ? EXT2_ROOT_INODE : current_dir;
    char token[256];
    const char *p = path;
    if (path[0]=='/') p++;
    while (*p)
    {
        int i = 0;
        while (*p && *p != '/') token[i++] = *p++;
        token[i] = '\0';
        if (i == 0) break;
        uint32_t child;
        if (ext2_lookup(cur, token, &child) < 0) return -1;
        cur = child;
        if (*p) p++;
    }
    *inode_out = cur;
    return 0;
}

static int ext2_create_file(uint32_t parent_inode_num, const char *name, uint16_t mode)
{
    uint32_t new_inode = ext2_allocate_inode();
    if (new_inode == 0)
    {
        printf("No free inode available\n");
        return -1;
    }
    struct ext2_inode file;
    memset(&file, 0, sizeof(file));
    file.i_mode = mode; /* e.g. 0x8000 for regular file */
    file.i_size = 0;
    file.i_links_count = 1;
    file.i_blocks = 0;
    ext2_write_inode(new_inode, &file);
    if (ext2_add_dir_entry(parent_inode_num, name, new_inode, (mode & 0x4000) ? EXT2_FT_DIR : EXT2_FT_REG_FILE) < 0)
        return -1;
    return new_inode;
}

/* --- Command Implementations --- */
void ext2_cmd_ls(const char *path)
{
    uint32_t inode_num;
    if (ext2_resolve_path(path, &inode_num) < 0)
    {
        printf("Directory not found\n");
        return;
    }
    struct ext2_inode dir;
    ext2_read_inode(inode_num, &dir);
    if (!(dir.i_mode & 0x4000))
    {
        printf("Not a directory\n");
        return;
    }
    uint8_t* blkbuf = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(dir.i_block[0], blkbuf);
    int offset = 0;
    struct ext2_dir_entry *de;
    while (offset < EXT2_BLOCK_SIZE)
    {
        de = (struct ext2_dir_entry *)(blkbuf + offset);
        if (de->inode != 0)
        {
            char name[256];
            memcpy(name, de->name, de->name_len);
            name[de->name_len] = '\0';
            printf("%s  ", name);
        }
        offset += de->rec_len;
    }
    printf("\n");
    kfree(blkbuf);
}

void ext2_cmd_cat(const char *path)
{
    uint32_t inode_num;
    if (ext2_resolve_path(path, &inode_num) < 0)
    {
        printf("File not found\n");
        return;
    }
    struct ext2_inode file;
    ext2_read_inode(inode_num, &file);
    if (file.i_mode & 0x4000)
    {
        printf("Is a directory\n");
        return;
    }
    if (file.i_block[0] == 0)
        return;
    uint8_t* blkbuf = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(file.i_block[0], blkbuf);
    int size = file.i_size < EXT2_BLOCK_SIZE ? file.i_size : EXT2_BLOCK_SIZE;
    for (int i = 0; i < size; i++)
        putc(blkbuf[i]);
    kfree(blkbuf);
}

void ext2_cmd_touch(const char *path)
{
    uint32_t inode_num;
    if (ext2_resolve_path(path, &inode_num) == 0)
    {
        struct ext2_inode file;
        ext2_read_inode(inode_num, &file);
        file.i_ctime = 0; /* TODO update with current time */
        ext2_write_inode(inode_num, &file);
        return;
    }
    char parent_path[256], file_name[256];
    strcpy(parent_path, path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash)
    {
        strcpy(file_name, last_slash+1);
        if (last_slash == parent_path)
            strcpy(parent_path, "/");
        else
            *last_slash = '\0';
    }
    else
    {
        strcpy(file_name, path);
        strcpy(parent_path, ".");
    }
    uint32_t parent;
    if (ext2_resolve_path(parent_path, &parent) < 0)
    {
        printf("Parent directory not found\n");
        return;
    }
    ext2_create_file(parent, file_name, 0x8000);
}

void ext2_cmd_mkdir(const char *path)
{
    char parent_path[256], dir_name[256];
    strcpy(parent_path, path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash)
    {
        strcpy(dir_name, last_slash+1);
        if (last_slash == parent_path)
            strcpy(parent_path, "/");
        else
            *last_slash = '\0';
    }
    else
    {
        strcpy(dir_name, path);
        strcpy(parent_path, ".");
    }
    uint32_t parent;
    if (ext2_resolve_path(parent_path, &parent) < 0)
    {
        printf("Parent directory not found\n");
        return;
    }
    uint32_t new_inode = ext2_allocate_inode();
    if (new_inode == 0)
    {
        printf("No free inode available\n");
        return;
    }
    struct ext2_inode dir;
    memset(&dir, 0, sizeof(dir));
    dir.i_mode = 0x4000;  /* directory */
    dir.i_size = EXT2_BLOCK_SIZE;
    dir.i_links_count = 2; /* . and .. */
    uint32_t block = ext2_allocate_block();
    if (block == 0)
    {
        printf("No free block available\n");
        return;
    }
    dir.i_block[0] = block;
    dir.i_blocks = 2;
    ext2_write_inode(new_inode, &dir);
    /* Initialize new directory block with '.' and '..' */
    uint8_t* blkbuf = kmalloc(EXT2_BLOCK_SIZE);
    memset(blkbuf, 0, EXT2_BLOCK_SIZE);
    struct ext2_dir_entry *de = (struct ext2_dir_entry *)blkbuf;
    de->inode = new_inode;
    de->rec_len = 12;
    de->name_len = 1;
    de->file_type = EXT2_FT_DIR;
    strcpy(de->name, ".");
    struct ext2_dir_entry *de2 = (struct ext2_dir_entry *)(blkbuf + 12);
    de2->inode = parent;
    de2->rec_len = EXT2_BLOCK_SIZE - 12;
    de2->name_len = 2;
    de2->file_type = EXT2_FT_DIR;
    strcpy(de2->name, "..");
    ext2_write_block(block, blkbuf);
    kfree(blkbuf);
    ext2_add_dir_entry(parent, dir_name, new_inode, EXT2_FT_DIR);
}

void ext2_cmd_rm(const char *path)
{
    uint32_t inode_num;
    if (ext2_resolve_path(path, &inode_num) < 0)
    {
        printf("File not found\n");
        return;
    }
    struct ext2_inode file;
    ext2_read_inode(inode_num, &file);
    if (file.i_mode & 0x4000)
    {
        printf("Is a directory – use rmdir\n");
        return;
    }
    char parent_path[256], file_name[256];
    strcpy(parent_path, path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash)
    {
        strcpy(file_name, last_slash+1);
        if (last_slash == parent_path)
            strcpy(parent_path, "/");
        else
            *last_slash = '\0';
    }
    else
    {
        strcpy(file_name, path);
        strcpy(parent_path, ".");
    }
    uint32_t parent;
    if (ext2_resolve_path(parent_path, &parent) < 0)
    {
        printf("Parent directory not found\n");
        return;
    }
    ext2_remove_dir_entry(parent, file_name);
    if (file.i_block[0])
        ext2_free_block(file.i_block[0]);
    ext2_free_inode(inode_num);
}

void ext2_cmd_rmdir(const char *path)
{
    uint32_t inode_num;
    if (ext2_resolve_path(path, &inode_num) < 0)
    {
        printf("Directory not found\n");
        return;
    }
    struct ext2_inode dir;
    ext2_read_inode(inode_num, &dir);
    if (!(dir.i_mode & 0x4000))
    {
        printf("Not a directory\n");
        return;
    }
    uint8_t* blkbuf = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(dir.i_block[0], blkbuf);
    int offset = 0, count = 0;
    struct ext2_dir_entry *de;
    while (offset < EXT2_BLOCK_SIZE)
    {
        de = (struct ext2_dir_entry *)(blkbuf + offset);
        if (de->inode != 0)
            count++;
        offset += de->rec_len;
    }
    kfree(blkbuf);
    if (count > 2)
    {
        printf("Directory not empty\n");
        return;
    }
    char parent_path[256], dname[256];
    strcpy(parent_path, path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash)
    {
        strcpy(dname, last_slash+1);
        if (last_slash == parent_path)
            strcpy(parent_path, "/");
        else
            *last_slash = '\0';
    }
    else
    {
        strcpy(dname, path);
        strcpy(parent_path, ".");
    }
    uint32_t parent;
    if (ext2_resolve_path(parent_path, &parent) < 0)
    {
        printf("Parent directory not found\n");
        return;
    }
    ext2_remove_dir_entry(parent, dname);
    ext2_free_block(dir.i_block[0]);
    ext2_free_inode(inode_num);
}

void ext2_cmd_cd(const char *path)
{
    uint32_t inode_num;
    if (ext2_resolve_path(path, &inode_num) < 0)
    {
        printf("Directory not found\n");
        return;
    }
    struct ext2_inode inode;
    ext2_read_inode(inode_num, &inode);
    if (!(inode.i_mode & 0x4000))
    {
        printf("Not a directory\n");
        return;
    }
    current_dir = inode_num;
}

static void cmd_ls();
static void cmd_cat();
static void cmd_touch();
static void cmd_mkdir();
static void cmd_rm();
static void cmd_rmdir();
static void cmd_cd();

command_t ext2_commands[] = {
    {"ls", "List directory contents", cmd_ls},
    {"cat", "Concatenate files and print on the standard output", cmd_cat},
    {"touch", "Change file timestamps", cmd_touch},
    {"mkdir", "Make directories", cmd_mkdir},
    {"rm", "Remove files or directories", cmd_rm},
    {"rmdir", "Remove directories", cmd_rmdir},
    {"cd", "Change the shell working directory", cmd_cd},
    {NULL, NULL, NULL}
};

static void cmd_ls()
{
    ext2_cmd_ls(".");
}

static void cmd_cat()
{
    printf("Enter the file name: ");
    ext2_cmd_cat(get_line());
}

static void cmd_touch()
{
    printf("Enter the file name: ");
    ext2_cmd_touch(get_line());
}

static void cmd_mkdir()
{
    printf("Enter the directory name: ");
    ext2_cmd_mkdir(get_line());
}

static void cmd_rm()
{
    printf("Enter the file name: ");
    ext2_cmd_rm(get_line());
}

static void cmd_rmdir()
{
    printf("Enter the directory name: ");
    ext2_cmd_rmdir(get_line());
}

static void cmd_cd()
{
    printf("Enter the directory name: ");
    ext2_cmd_cd(get_line());
}

void ext2_mount(void)
{
    uint8_t* buf = kmalloc(EXT2_BLOCK_SIZE);
    /* Read superblock (located at block 1) */
    ext2_read_block(1, buf);
    memcpy(&ext2.sb, buf, sizeof(struct ext2_super_block));
    if (ext2.sb.s_magic != 0xEF53)
        kernel_panic("ext2: bad magic number");
    /* Read group descriptor (assumed to be in block 2) */
    ext2_read_block(2, buf);
    memcpy(&ext2.gd, buf, sizeof(struct ext2_group_desc));
    /* Load inode and block bitmaps */
    ext2.inode_bitmap = kmalloc(EXT2_BLOCK_SIZE);
    ext2.block_bitmap = kmalloc(EXT2_BLOCK_SIZE);
    ext2_read_block(ext2.gd.bg_inode_bitmap, ext2.inode_bitmap);
    ext2_read_block(ext2.gd.bg_block_bitmap, ext2.block_bitmap);
    kfree(buf);
    install_all_cmds(ext2_commands, GLOBAL);
}
