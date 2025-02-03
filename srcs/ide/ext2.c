#include "ext2.h"
#include "ide.h"
#include "../utils/utils.h"
#include "../memory/memory.h"
#include "../display/display.h"

#define SECTOR_SIZE        512
#define EXT2_BLOCK_SIZE    1024
#define SECTORS_PER_BLOCK  (EXT2_BLOCK_SIZE / SECTOR_SIZE)

#define TOTAL_BLOCKS 8192
#define INODES_COUNT 1024
#define FIRST_DATA_BLOCK 1
#define BLOCKS_PER_GROUP 8192
#define INODES_PER_GROUP 1024

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

    sb.s_inodes_count       = INODES_COUNT;
    sb.s_blocks_count       = TOTAL_BLOCKS;
    sb.s_free_blocks_count  = TOTAL_BLOCKS - 2; /* Assume block #0 and #1 used */
    sb.s_free_inodes_count  = INODES_COUNT - 1; /* let's assume inode #1 (or #2) is used for root */
    sb.s_first_data_block   = FIRST_DATA_BLOCK;
    sb.s_log_block_size     = 0;
    sb.s_log_frag_size      = 0;
    sb.s_blocks_per_group   = BLOCKS_PER_GROUP;
    sb.s_frags_per_group    = BLOCKS_PER_GROUP;
    sb.s_inodes_per_group   = INODES_PER_GROUP;
    sb.s_magic              = 0xEF53; /* ext2 signature */
    sb.s_state              = 1; /* FS state: EXT2_VALID_FS */
    sb.s_errors             = 1; /* continue on error */

    static uint8_t block_buf[EXT2_BLOCK_SIZE];
    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    memcpy(block_buf, &sb, sizeof(sb));

    write_block(1, block_buf);

    printf("ext2_format Formatting complete.\n");
    return 0;
}

int ext2_read_superblock(ext2_superblock_t* sb)
{
    if(!sb)
    {
        return -1;
    }

    static uint8_t block_buf[EXT2_BLOCK_SIZE];
    memset(block_buf, 0, EXT2_BLOCK_SIZE);

    read_block(1, block_buf);

    memcpy(sb, block_buf, sizeof(ext2_superblock_t));

    if(sb->s_magic != 0xEF53)
    {
        printf("ext2_read_superblock Invalid magic: %x (should be EF53)\n", sb->s_magic);
        return -1;
    }

    return 0;
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
