#include "ext2.h"
#include "ide.h"
#include "../utils/utils.h"
#include "../memory/memory.h"
#include "../display/display.h"
#include "../kshell/kshell.h"
#include "../keyboard/keyboard.h"

#define SECTOR_SIZE        512
#define EXT2_BLOCK_SIZE    1024
#define SECTORS_PER_BLOCK  (EXT2_BLOCK_SIZE / SECTOR_SIZE)

#define TOTAL_BLOCKS 8192
#define INODES_COUNT 1024
#define FIRST_DATA_BLOCK 1
#define BLOCKS_PER_GROUP 8192
#define INODES_PER_GROUP 1024

#define MAX_PATH_LENGTH 256

void cmd_ls();
void cmd_cd();
void cmd_cat();
void cmd_touch();
void cmd_mkdir();
void cmd_pwd();
void cmd_write();

int ext2_read_inode(uint32_t inode_num, ext2_inode_t* inode);
static int ext2_write_inode(uint32_t inode_num, const ext2_inode_t* inode);

command_t ext2_commands[] = {
    {"ls", "List directory contents", cmd_ls},
    {"cd", "Change directory", cmd_cd},
    {"cat", "Concatenate files and print on the standard output", cmd_cat},
    {"touch", "Change file timestamps", cmd_touch},
    {"mkdir", "Make directories", cmd_mkdir},
    {"pwd", "Print name of current/working directory", cmd_pwd},
    {"write", "Write 'Hello from ext2!' to hello.txt", cmd_write},
    {NULL, NULL, NULL}
};

static uint32_t g_current_dir_inode_num = 2; /* root inode is #2 */
static ext2_inode_t g_current_dir_inode;
static ext2_group_desc_t g_single_gd;
static uint32_t next_free_inode = 5;   /* In ext2_format, inodes 2-4 are in use */
static uint32_t next_free_block = 13;  /* Blocks 10,11,12 are already used */

static void write_block(uint32_t block_num, const void* buf)
{
    uint32_t start_lba = block_num * SECTORS_PER_BLOCK;
    for (int i = 0; i < SECTORS_PER_BLOCK; i++)
    {
        ide_write_sector(start_lba + i, (uint16_t*)((uint8_t*)buf + (i * SECTOR_SIZE)));
    }
}

void read_block(uint32_t block_num, void* buf)
{
    uint32_t start_lba = block_num * SECTORS_PER_BLOCK;
    for (int i = 0; i < SECTORS_PER_BLOCK; i++)
    {
        ide_read_sector(start_lba + i, (uint16_t*)((uint8_t*)buf + (i * SECTOR_SIZE)));
    }
}

int ext2_format(void)
{
    ext2_superblock_t sb;
    memset(&sb, 0, sizeof(sb));

    sb.s_inodes_count = INODES_COUNT;
    sb.s_blocks_count = TOTAL_BLOCKS;
    sb.s_free_blocks_count = TOTAL_BLOCKS - 2;
    sb.s_free_inodes_count = INODES_COUNT - 1;
    sb.s_first_data_block = FIRST_DATA_BLOCK;
    sb.s_log_block_size = 0;
    sb.s_log_frag_size = 0;
    sb.s_blocks_per_group = BLOCKS_PER_GROUP;
    sb.s_frags_per_group = BLOCKS_PER_GROUP;
    sb.s_inodes_per_group = INODES_PER_GROUP;
    sb.s_magic = 0xEF53;
    sb.s_state = 1;
    sb.s_errors = 1;

    static uint8_t block_buf[EXT2_BLOCK_SIZE];
    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    memcpy(block_buf, &sb, sizeof(sb));
    write_block(1, block_buf);

    ext2_group_desc_t gd;
    memset(&gd, 0, sizeof(gd));
    gd.bg_block_bitmap = 3;
    gd.bg_inode_bitmap = 4;
    gd.bg_inode_table = 5;
    gd.bg_free_blocks_count = 0;
    gd.bg_free_inodes_count = 0;

    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    memcpy(block_buf, &gd, sizeof(gd));
    write_block(2, block_buf);

    g_single_gd = gd;

    ext2_inode_t root;
    memset(&root, 0, sizeof(root));
    root.i_mode = S_IFDIR | 0755;
    root.i_size = 1024;
    root.i_links_count = 2;
    root.i_block[0] = 10;

    ext2_write_inode(2, &root);

    ext2_inode_t foo;
    memset(&foo, 0, sizeof(foo));
    foo.i_mode = S_IFDIR | 0755;
    foo.i_size = 1024;
    foo.i_links_count= 2;
    foo.i_block[0] = 11;
    ext2_write_inode(3, &foo);

    const char* file_content = "Hello from ext2!\n";
    size_t file_len = strlen(file_content);

    ext2_inode_t hello;
    memset(&hello, 0, sizeof(hello));
    hello.i_mode = S_IFREG | 0644;
    hello.i_size = file_len;
    hello.i_links_count= 1;
    hello.i_block[0]   = 12;
    ext2_write_inode(4, &hello);

    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    memcpy(block_buf, file_content, file_len);
    write_block(12, block_buf);

    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    
    ext2_dir_entry_t* d = (ext2_dir_entry_t*)block_buf;

    d->inode = 2;
    d->name_len = 1;
    d->file_type = 2;
    d->rec_len = 12;
    d->name[0] = '.';

    ext2_dir_entry_t* d2 = (ext2_dir_entry_t*)((uint8_t*)d + d->rec_len);
    d2->inode = 2;
    d2->name_len = 2;
    d2->file_type = 2;
    d2->rec_len = 12;
    d2->name[0] = '.';
    d2->name[1] = '.';

    ext2_dir_entry_t* d3 = (ext2_dir_entry_t*)((uint8_t*)d2 + d2->rec_len);
    d3->inode = 3;
    d3->name_len = 3;
    d3->file_type = 2;
    d3->rec_len = 12;
    d3->name[0] = 'f';
    d3->name[1] = 'o';
    d3->name[2] = 'o';

    ext2_dir_entry_t* d4 = (ext2_dir_entry_t*)((uint8_t*)d3 + d3->rec_len);
    d4->inode = 4;
    d4->name_len = 9; 
    d4->file_type = 1;
    d4->rec_len = EXT2_BLOCK_SIZE - ( (uint8_t*)d4 - (uint8_t*)block_buf );
    memcpy(d4->name, "hello.txt", 9);
    d4->name[9] = '\0';

    write_block(10, block_buf);

    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    d = (ext2_dir_entry_t*)block_buf;

    /* "." in foo => inode=3 */
    d->inode = 3;
    d->name_len = 1;
    d->file_type = 2; // dir
    d->rec_len = 12;
    d->name[0] = '.';

    /* '..' */
    d2 = (ext2_dir_entry_t*)((uint8_t*)d + d->rec_len);
    d2->inode = 2;
    d2->name_len = 2;
    d2->file_type = 2;
    d2->rec_len = EXT2_BLOCK_SIZE - 12;
    d2->name[0] = '.';
    d2->name[1] = '.';

    write_block(11, block_buf);

    if(ext2_read_superblock(&sb) < 0)
    {
        printf("Error: superblock read failed after format.\n");
    }

    g_single_gd = gd;

    ext2_read_inode(2, &g_current_dir_inode);
    g_current_dir_inode_num = 2;

    printf("[ext2_format] Demo format complete. Root has 'foo' directory and 'hello.txt'.\n");
    return 0;
}

static uint32_t recalc_next_free_block(void)
{
    uint8_t *used = kmalloc((TOTAL_BLOCKS + 1) * sizeof(uint8_t));
    if (!used)
    {
        printf("Failed to allocate memory for block scan.\n");
        return 0;
    }
    
    for (uint32_t b = 0; b < 5; b++)
    {
        used[b] = 1;
    }
    used[5] = 1;
    used[10] = used[11] = used[12] = 1;
    
    ext2_inode_t inode;
    for (uint32_t ino = 1; ino <= INODES_COUNT; ino++)
    {
        ext2_read_inode(ino, &inode);
        if (inode.i_mode != 0)
        {
            for (int i = 0; i < 12; i++)
            {
                if (inode.i_block[i] != 0 && inode.i_block[i] < TOTAL_BLOCKS)
                {
                    used[inode.i_block[i]] = 1;
                }
            }
        }
    }
    
    uint32_t free_block = 0;
    for (uint32_t b = 1; b < TOTAL_BLOCKS; b++)
    {
        if (!used[b])
        {
            free_block = b;
            break;
        }
    }
    kfree(used);
    return free_block;
}

static uint32_t recalc_next_free_inode(void)
{
    ext2_inode_t inode;

    for (uint32_t ino = 5; ino <= INODES_COUNT; ino++)
    {
        ext2_read_inode(ino, &inode);
        if (inode.i_mode == 0)
        {
            return ino;
        }
    }
    return 0;
}


int ext2_mount()
{
    ext2_superblock_t sb;
    if (ext2_read_superblock(&sb) < 0)
    {
        printf("No valid ext2 found; formatting...\n");
        install_all_cmds(ext2_commands, GLOBAL);
        return 0;
    }

    ext2_group_desc_t gd;
    static uint8_t block_buf[EXT2_BLOCK_SIZE];
    read_block(2, block_buf);
    memcpy(&gd, block_buf, sizeof(gd));
    g_single_gd = gd;

    next_free_inode = recalc_next_free_inode();
    next_free_block = recalc_next_free_block();

    ext2_read_inode(2, &g_current_dir_inode);
    g_current_dir_inode_num = 2;

    install_all_cmds(ext2_commands, GLOBAL);

    printf("next_free_inode=%d, next_free_block=%d\n", next_free_inode, next_free_block);

    printf("Mounted ext2 from disk successfully!\n");
    return 0;
}

int ext2_read_superblock(ext2_superblock_t* sb)
{
    if(!sb) return -1;

    static uint8_t block_buf[EXT2_BLOCK_SIZE];
    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    read_block(1, block_buf);

    memcpy(sb, block_buf, sizeof(ext2_superblock_t));
    if(sb->s_magic != 0xEF53)
    {
        printf("ext2_read_superblock: Invalid magic: %x (should be EF53)\n", sb->s_magic);
        return -1;
    }
    return 0;
}

int ext2_read_inode(uint32_t inode_num, ext2_inode_t* inode)
{
    uint32_t index = inode_num - 1;
    uint32_t inode_table_block = g_single_gd.bg_inode_table;
    uint32_t inodes_per_block = EXT2_BLOCK_SIZE / sizeof(ext2_inode_t);

    uint32_t block_in_table = index / inodes_per_block;
    uint32_t offset_in_block = (index % inodes_per_block) * sizeof(ext2_inode_t);

    static uint8_t block_buf[EXT2_BLOCK_SIZE];
    read_block(inode_table_block + block_in_table, block_buf);

    memcpy(inode, block_buf + offset_in_block, sizeof(ext2_inode_t));
    return 0; 
}

static int ext2_write_inode(uint32_t inode_num, const ext2_inode_t* inode)
{
    uint32_t index = inode_num - 1;
    uint32_t inode_table_block = g_single_gd.bg_inode_table;
    uint32_t inodes_per_block = EXT2_BLOCK_SIZE / sizeof(ext2_inode_t);

    uint32_t block_in_table = index / inodes_per_block;
    uint32_t offset_in_block = (index % inodes_per_block) * sizeof(ext2_inode_t);

    static uint8_t block_buf[EXT2_BLOCK_SIZE];
    read_block(inode_table_block + block_in_table, block_buf);
    memcpy(block_buf + offset_in_block, inode, sizeof(ext2_inode_t));
    write_block(inode_table_block + block_in_table, block_buf);

    return 0;
}

int ext2_read_data(const ext2_inode_t* inode, uint32_t offset, uint32_t count, uint8_t* out_buf)
{
    if(offset >= inode->i_size) return 0;

    if(offset + count > inode->i_size)
    {
        count = inode->i_size - offset;
    }

    uint32_t block_size = EXT2_BLOCK_SIZE; 
    uint32_t bytes_read = 0;

    while(bytes_read < count)
    {
        uint32_t block_index = (offset + bytes_read) / block_size;
        uint32_t offset_in_block = (offset + bytes_read) % block_size;

        uint32_t disk_block = 0;
        if(block_index < 12)
        {
            disk_block = inode->i_block[block_index];
        }
        else
        {
            /* Indirect block what to do?? */
        }
        if(disk_block == 0)
        {
            break; 
        }

        static uint8_t block_buf[EXT2_BLOCK_SIZE];
        read_block(disk_block, block_buf);

        uint32_t bytes_can_copy = block_size - offset_in_block;
        if(bytes_can_copy > (count - bytes_read))
        {
            bytes_can_copy = count - bytes_read;
        }

        memcpy(out_buf + bytes_read, block_buf + offset_in_block, bytes_can_copy);
        bytes_read += bytes_can_copy;
    }

    return bytes_read;
}

uint32_t ext2_lookup(const ext2_inode_t* dir_inode, const char* name)
{
    if(!dir_inode) return 0;

    uint32_t offset = 0;
    static uint8_t block_buf[EXT2_BLOCK_SIZE];

    while(offset < dir_inode->i_size)
    {
        uint32_t block_index = offset / EXT2_BLOCK_SIZE;
        uint32_t offset_in_block = offset % EXT2_BLOCK_SIZE;

        uint32_t disk_block = dir_inode->i_block[block_index]; 
        if(!disk_block) break;

        read_block(disk_block, block_buf);

        while(offset_in_block < EXT2_BLOCK_SIZE)
        {
            ext2_dir_entry_t* dentry = (ext2_dir_entry_t*)(block_buf + offset_in_block);
            if(dentry->inode == 0)
            {
                offset += dentry->rec_len;
                offset_in_block += dentry->rec_len;
                continue;
            }

            if(dentry->name_len == strlen(name) &&
               memcmp(dentry->name, name, dentry->name_len) == 0)
            {
                return dentry->inode;
            }

            offset += dentry->rec_len;
            offset_in_block += dentry->rec_len;
            if(offset >= dir_inode->i_size) break;
        }
    }
    return 0;
}

int convert_path_to_inode(const char* path)
{
    if(path[0] != '/')
    {
        return ext2_lookup(&g_current_dir_inode, path);
    }

    if(strcmp(path, "/") == 0)
    {
        return 2;
    }

    ext2_inode_t inode;
    ext2_read_inode(2, &inode);

    char path_copy[MAX_PATH_LENGTH];
    strcpy(path_copy, path);

    char* token = strtok(path_copy, "/");
    uint32_t ino = 2;
    while(token)
    {
        ino = ext2_lookup(&inode, token);
        if(ino == 0)
        {
            printf("Path not found: %s. Defaulting to root\n", token);
            return 2;
        }

        ext2_read_inode(ino, &inode);
        token = strtok(NULL, "/");
    }

    return ino;
}

static void join_path(const char *current, const char *name, char *dest, int maxlen)
{
    int current_len = strlen(current);
    if (current_len >= maxlen)
    {
        dest[0] = '\0';
        return;
    }
    strcpy(dest, current);
    
    if (strcmp(current, "/") != 0)
    {
        if (dest[current_len - 1] != '/')
        {
            if (current_len < maxlen - 1)
            {
                dest[current_len] = '/';
                dest[current_len + 1] = '\0';
                current_len++;
            }
        }
    }
    strncat(dest, name, maxlen - current_len - 1);
}

static int search_from_dir(uint32_t dir_inode, uint32_t target_inode,
                           const char *current_path, int maxlen, char *result_path)
{
    ext2_inode_t dir;
    ext2_read_inode(dir_inode, &dir);
    if (!(dir.i_mode & S_IFDIR))
    {
        return -1;
    }
    
    uint8_t block_buf[EXT2_BLOCK_SIZE];
    read_block(dir.i_block[0], block_buf);
    
    uint32_t offset = 0;
    while (offset < dir.i_size)
    {
        ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(block_buf + offset);
        if (entry->inode != 0)
        {
            char name[256];
            memcpy(name, entry->name, entry->name_len);
            name[entry->name_len] = '\0';
            
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
            {
                if (entry->inode == target_inode)
                {
                    join_path(current_path, name, result_path, maxlen);
                    return 0;
                }
                ext2_inode_t child;
                ext2_read_inode(entry->inode, &child);
                if (child.i_mode & S_IFDIR)
                {
                    char new_path[256];
                    join_path(current_path, name, new_path, sizeof(new_path));
                    if (search_from_dir(entry->inode, target_inode, new_path, maxlen, result_path) == 0)
                        return 0;
                }
            }
        }
        offset += entry->rec_len;
    }
    return -1;
}

int convert_inode_to_path(int inode, char* buffer)
{
    if (inode == 2)
    {
        strcpy(buffer, "/");
        return 0;
    }
    return search_from_dir(2, (uint32_t)inode, "/", MAX_PATH_LENGTH, buffer);
}


int ext2_list_dir(const ext2_inode_t* dir_inode)
{
    uint32_t offset = 0;
    static uint8_t block_buf[EXT2_BLOCK_SIZE];

    while(offset < dir_inode->i_size)
    {
        uint32_t block_index = offset / EXT2_BLOCK_SIZE;
        uint32_t offset_in_block = offset % EXT2_BLOCK_SIZE;
        uint32_t disk_block = dir_inode->i_block[block_index];
        if(!disk_block) break;

        read_block(disk_block, block_buf);

        while(offset_in_block < EXT2_BLOCK_SIZE)
        {
            ext2_dir_entry_t* dentry = (ext2_dir_entry_t*)(block_buf + offset_in_block);
            if(dentry->inode == 0)
            {
                offset += dentry->rec_len;
                offset_in_block += dentry->rec_len;
                continue;
            }

            printf("%s ", dentry->name);

            offset += dentry->rec_len;
            offset_in_block += dentry->rec_len;
            if(offset >= dir_inode->i_size) break;
        }
    }
    printf("\n");
    return 0;
}

void cmd_cd()
{
    char* dirname;
    ext2_inode_t tmp;
    uint32_t target_ino;

    puts("Enter directory name: ");
    dirname = get_line();
    if (strcmp(dirname, ".") == 0)
    {
        return;
    }

    target_ino = convert_path_to_inode(dirname);
    if (target_ino == 0)
    {
        printf("Directory not found!\n");
        return;
    }

    printf("Found directory %s with inode %d\n", dirname, target_ino);
    ext2_read_inode(target_ino, &tmp);
    if (!S_ISDIR(tmp.i_mode))
    {
        printf("%s is not a directory!\n", dirname);
        return;
    }

    char buffer[MAX_PATH_LENGTH];
    convert_inode_to_path(target_ino, buffer);
    printf("Current path: %s\n", buffer);
    g_current_dir_inode_num = target_ino;
    memcpy(&g_current_dir_inode, &tmp, sizeof(ext2_inode_t));
    
}

void cmd_ls()
{
    ext2_list_dir(&g_current_dir_inode);
}

void cmd_cat(void)
{
    char* filename;
    puts("Enter file name: ");
    filename = get_line();
    uint32_t ino = convert_path_to_inode(filename);
    if(!ino)
    {
        printf("File not found!\n");
        return;
    }

    ext2_inode_t file_inode;
    ext2_read_inode(ino, &file_inode);

    if(!S_ISREG(file_inode.i_mode))
    {
        printf("%s is not a regular file!\n", filename);
        return;
    }

    uint8_t* buf = kmalloc(file_inode.i_size);
    if(!buf) return;

    int n = ext2_read_data(&file_inode, 0, file_inode.i_size, buf);
    for(int i = 0; i < n; i++)
    {
        putc(buf[i]);
    }
    // putc('\n');
    kfree(buf);
}


void cmd_pwd()
{
    char buffer[MAX_PATH_LENGTH];
    convert_inode_to_path(g_current_dir_inode_num, buffer);
    printf("%s\n", buffer);
}

/* Allocate a new inode number (this is a very simple scheme) */
static uint32_t ext2_alloc_inode(void)
{
    if(next_free_inode > INODES_COUNT)
    {
        return 0;
    }
    return next_free_inode++;
}

static uint32_t ext2_alloc_block(void)
{
    if(next_free_block >= TOTAL_BLOCKS)
    {
        return 0;
    }
    return next_free_block++;
}

static int ext2_add_dir_entry(ext2_inode_t* parent_inode, uint32_t parent_inode_num, const char* name, uint32_t child_inode, uint8_t file_type)
{
    uint8_t block_buf[EXT2_BLOCK_SIZE];
    uint32_t block = parent_inode->i_block[0];
    if (block == 0)
    {
        block = ext2_alloc_block();
        if (block == 0)
        {
            return -1;
        }
        parent_inode->i_block[0] = block;
        parent_inode->i_size = EXT2_BLOCK_SIZE;
        memset(block_buf, 0, EXT2_BLOCK_SIZE);
        write_block(block, block_buf);
    }
    
    read_block(block, block_buf);
    uint32_t offset = 0;
    ext2_dir_entry_t* entry = NULL;
    while (offset < EXT2_BLOCK_SIZE)
    {
        entry = (ext2_dir_entry_t*)(block_buf + offset);
        if(entry->inode == 0)
        {
            break;
        }
        uint16_t ideal_len = 4 * (((8 + entry->name_len) + 3) / 4);
        uint16_t available = entry->rec_len - ideal_len;
        uint16_t new_entry_size = 4 * (((8 + strlen(name)) + 3) / 4);
        if (available >= new_entry_size)
        {
            entry->rec_len = ideal_len;
            ext2_dir_entry_t* new_entry = (ext2_dir_entry_t*)((uint8_t*)entry + ideal_len);
            new_entry->inode = child_inode;
            new_entry->name_len = strlen(name);
            new_entry->file_type = file_type;
            new_entry->rec_len = available;
            memcpy(new_entry->name, name, new_entry->name_len);
            new_entry->name[new_entry->name_len] = '\0';
            write_block(block, block_buf);
            ext2_write_inode(parent_inode_num, parent_inode);
            return 0;
        }
        offset += entry->rec_len;
    }
    return -1;
}

void cmd_mkdir()
{
    char* dirname;
    puts("Enter directory name: ");
    dirname = get_line();
    if (strlen(dirname) == 0)
    {
        printf("Empty name!\n");
        return;
    }
    if (ext2_lookup(&g_current_dir_inode, dirname) != 0)
    {
        printf("Entry '%s' already exists!\n", dirname);
        return;
    }

    uint32_t new_inode_num = ext2_alloc_inode();
    if(new_inode_num == 0)
    {
        printf("No free inodes!\n");
        return;
    }
    uint32_t new_block = ext2_alloc_block();
    if(new_block == 0)
    {
        printf("No free blocks!\n");
        return;
    }

    ext2_inode_t new_dir;
    memset(&new_dir, 0, sizeof(new_dir));
    new_dir.i_mode = S_IFDIR | 0755;
    new_dir.i_size = EXT2_BLOCK_SIZE;
    new_dir.i_links_count = 2;
    new_dir.i_block[0] = new_block;
    ext2_write_inode(new_inode_num, &new_dir);

    uint8_t block_buf[EXT2_BLOCK_SIZE];
    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    ext2_dir_entry_t* d = (ext2_dir_entry_t*)block_buf;
    d->inode = new_inode_num;
    d->name_len = 1;
    d->file_type = 2;
    d->rec_len = 12;
    d->name[0] = '.';

    ext2_dir_entry_t* d2 = (ext2_dir_entry_t*)((uint8_t*)d + d->rec_len);
    d2->inode = g_current_dir_inode_num;
    d2->name_len = 2;
    d2->file_type = 2;
    d2->rec_len = EXT2_BLOCK_SIZE - d->rec_len;
    d2->name[0] = '.';
    d2->name[1] = '.';

    write_block(new_block, block_buf);

    if (ext2_add_dir_entry(&g_current_dir_inode, g_current_dir_inode_num, dirname, new_inode_num, 2) < 0)
    {
        printf("Failed to add directory entry.\n");
        return;
    }
    g_current_dir_inode.i_links_count++;
    ext2_write_inode(g_current_dir_inode_num, &g_current_dir_inode);

    printf("Directory '%s' created.\n", dirname);
}

void cmd_touch()
{
    char* filename;
    puts("Enter file name: ");
    filename = get_line();
    if (strlen(filename) == 0)
    {
        printf("Empty name!\n");
        return;
    }
    if (ext2_lookup(&g_current_dir_inode, filename) != 0)
    {
        printf("Entry '%s' already exists!\n", filename);
        return;
    }

    uint32_t new_inode_num = ext2_alloc_inode();
    if(new_inode_num == 0)
    {
        printf("No free inodes!\n");
        return;
    }

    ext2_inode_t new_file;
    memset(&new_file, 0, sizeof(new_file));
    new_file.i_mode = S_IFREG | 0644;
    new_file.i_size = 0;
    new_file.i_links_count = 1;
    ext2_write_inode(new_inode_num, &new_file);

    if (ext2_add_dir_entry(&g_current_dir_inode, g_current_dir_inode_num, filename, new_inode_num, 1) < 0)
    {
        printf("Failed to add file entry.\n");
        return;
    }

    printf("File '%s' created.\n", filename);
}

int ext2_write_data(ext2_inode_t *inode, uint32_t offset, const uint8_t* data, uint32_t len)
{
    uint8_t block_buf[EXT2_BLOCK_SIZE];
    uint32_t block_size = EXT2_BLOCK_SIZE;

    if (inode->i_block[0] == 0)
    {
        inode->i_block[0] = ext2_alloc_block();
        if (inode->i_block[0] == 0)
        {
            printf("ext2_write_data: No free blocks available!\n");
            return -1;
        }
        memset(block_buf, 0, block_size);
    }
    else
    {
        read_block(inode->i_block[0], block_buf);
    }

    if (offset + len > block_size)
    {
        len = block_size - offset;
    }

    memcpy(block_buf + offset, data, len);
    write_block(inode->i_block[0], block_buf);

    if (offset + len > inode->i_size)
    {
        inode->i_size = offset + len;
    }

    return len;
}

ext2_file_t* ext2_open(const char* filename, const char* mode)
{
    // uint32_t ino = ext2_lookup(&g_current_dir_inode, filename);
    // if (ino == 0)
    // {
    //     printf("ext2_open: File '%s' not found!\n", filename);
    //     return NULL;
    // }

    uint32_t ino = convert_path_to_inode(filename);
    if (ino == 0)
    {
        printf("ext2_open: File '%s' not found!\n", filename);
        return NULL;
    }

    ext2_file_t* file = kmalloc(sizeof(ext2_file_t));
    if (!file)
    {
        printf("ext2_open: Out of memory!\n");
        return NULL;
    }

    file->inode_num = ino;
    ext2_read_inode(ino, &file->inode);

    if (strcmp(mode, "w") == 0)
    {
        file->inode.i_size = 0;
        file->pos = 0;
        file->inode.i_block[0] = 0; 
        ext2_write_inode(ino, &file->inode);
    }
    else if (strcmp(mode, "a") == 0)
    {
        file->pos = file->inode.i_size;
    }
    else
    {
        file->pos = 0;
    }

    return file;
}

int ext2_write(ext2_file_t* file, const void* buf, uint32_t len)
{
    if (!file) return -1;

    int written = ext2_write_data(&file->inode, file->pos, (const uint8_t*)buf, len);
    if (written < 0)
    {
        printf("ext2_write: Error writing data!\n");
        return -1;
    }
    file->pos += written;

    ext2_write_inode(file->inode_num, &file->inode);

    return written;
}

int ext2_read(ext2_file_t* file, void* buf, uint32_t len)
{
    if (!file) return -1;

    int read = ext2_read_data(&file->inode, file->pos, len, (uint8_t*)buf);
    if (read < 0)
    {
        printf("ext2_read: Error reading data!\n");
        return -1;
    }
    file->pos += read;

    return read;
}

int ext2_close(ext2_file_t* file)
{
    if (!file) return -1;

    ext2_write_inode(file->inode_num, &file->inode);
    kfree(file);
    return 0;
}

void cmd_write()
{
    printf("Enter filename: ");
    char* filename = get_line();
    ext2_file_t* f = ext2_open(filename, "w");
    if (!f)
    {
        printf("cmd_write_hello: Could not open 'hello.txt' for writing.\n");
        return;
    }
    printf("Enter message: ");
    char* message = get_line();
    int n = ext2_write(f, message, strlen(message));
    if (n < 0)
    {
        printf("cmd_write_hello: Error writing to file.\n");
    }
    else
    {
        printf("cmd_write_hello: Wrote %d bytes to 'hello.txt'.\n", n);
        ext2_write(f, "\n", 1);
    }
    ext2_close(f);
}

int set_actual_dir(uint32_t inode_num)
{
    ext2_read_inode(inode_num, &g_current_dir_inode);
    g_current_dir_inode_num = inode_num;
}

/* ############################################################################# */
/*                                    TESTS                                      */
/* ############################################################################# */

void ext2_demo()
{
    ext2_superblock_t sb;
    if(ext2_read_superblock(&sb) < 0)
    {
        printf("Failed to read ext2 superblock!\n");
        return;
    }

    printf("EXT2 Superblock Info:\n");
    printf("    Inodes Count : %u\n", sb.s_inodes_count);
    printf("    Blocks Count : %u\n", sb.s_blocks_count);
    printf("    Free Blocks  : %u\n", sb.s_free_blocks_count);
    printf("    Free Inodes  : %u\n", sb.s_free_inodes_count);
    printf("    First Data Bl: %u\n", sb.s_first_data_block);
    printf("    Magic        : %x\n", sb.s_magic);
}
