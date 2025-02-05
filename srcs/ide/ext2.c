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

#define S_IFMT  0xF000
#define S_IFDIR 0x4000
#define S_IFREG 0x8000

static inline int S_ISDIR(uint16_t m) { return ((m & S_IFMT) == S_IFDIR); }
static inline int S_ISREG(uint16_t m) { return ((m & S_IFMT) == S_IFREG); }

void cmd_ls();
void cmd_cd();
void cmd_cat();
// void cmd_touch(const char* filename);
// void cmd_mkdir(const char* dirname);
void cmd_pwd();

static int ext2_read_inode(uint32_t inode_num, ext2_inode_t* inode);
static int ext2_write_inode(uint32_t inode_num, const ext2_inode_t* inode);

command_t ext2_commands[] = {
    {"ls", "List directory contents", cmd_ls},
    {"cd", "Change directory", cmd_cd},
    {"cat", "Concatenate files and print on the standard output", cmd_cat},
    // {"touch", "Change file timestamps", cmd_touch},
    // {"mkdir", "Make directories", cmd_mkdir},
    {"pwd", "Print name of current/working directory", cmd_pwd},
    {NULL, NULL, NULL}
};

static uint32_t g_current_dir_inode_num = 2; /* root inode is #2 */
static ext2_inode_t g_current_dir_inode;
static ext2_group_desc_t g_single_gd;

static void write_block(uint32_t block_num, const void* buf)
{
    uint32_t start_lba = block_num * SECTORS_PER_BLOCK;
    for (int i = 0; i < SECTORS_PER_BLOCK; i++)
    {
        ide_write_sector(start_lba + i, (uint16_t*)((uint8_t*)buf + (i * SECTOR_SIZE)));
    }
}

static void read_block(uint32_t block_num, void* buf)
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

    install_all_cmds(ext2_commands, GLOBAL);

    printf("[ext2_format] Demo format complete. Root has 'foo' directory and 'hello.txt'.\n");
    return 0;
}

int ext2_mount()
{
    ext2_superblock_t sb;
    if (ext2_read_superblock(&sb) < 0)
    {
        printf("No valid ext2 found; formatting...\n");
        return ext2_format();
    }

    ext2_group_desc_t gd;
    static uint8_t block_buf[EXT2_BLOCK_SIZE];
    read_block(2, block_buf);
    memcpy(&gd, block_buf, sizeof(gd));
    g_single_gd = gd;

    ext2_read_inode(2, &g_current_dir_inode);
    g_current_dir_inode_num = 2;

    install_all_cmds(ext2_commands, GLOBAL);


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

static int ext2_read_inode(uint32_t inode_num, ext2_inode_t* inode)
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

    puts("Enter directory name: ");
    dirname = get_line();
    if(strcmp(dirname, ".") == 0)
    {
        return;
    }

    uint32_t target_ino = ext2_lookup(&g_current_dir_inode, dirname);
    if(target_ino == 0)
    {
        printf("Directory not found!\n");
        return;
    }

    ext2_inode_t tmp;
    ext2_read_inode(target_ino, &tmp);
    if(!S_ISDIR(tmp.i_mode))
    {
        printf("%s is not a directory!\n", dirname);
        return;
    }

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
    uint32_t ino = ext2_lookup(&g_current_dir_inode, filename);
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
    printf("/ (Not implemented yet)\n");
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
