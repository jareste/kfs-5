#ifndef EXT_2
#define EXT_2

#include "../utils/stdint.h"

#define S_IFMT  0xF000
#define S_IFDIR 0x4000
#define S_IFREG 0x8000

static inline int S_ISDIR(uint16_t m) { return ((m & S_IFMT) == S_IFDIR); }
static inline int S_ISREG(uint16_t m) { return ((m & S_IFMT) == S_IFREG); }

#pragma pack(push, 1)
typedef struct
{
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
} ext2_superblock_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
} ext2_group_desc_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;

    uint32_t i_block[12];
    uint32_t i_block_indirect;
    uint32_t i_block_double;
    uint32_t i_block_triple;
} ext2_inode_t;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[255];
} ext2_dir_entry_t;
#pragma pack(pop)

/* Structure to represent an open file.
   (For now we support only one block per file.) */
typedef struct
{
    uint32_t inode_num;   /* inode number of the file */
    ext2_inode_t inode;   /* a copy of the file inode */
    uint32_t pos;         /* current file position */
} ext2_file_t;

int ext2_format(void);

int ext2_mount();

int ext2_read_superblock(ext2_superblock_t* sb);

void ext2_demo();

int ext2_write(ext2_file_t* file, const void* buf, uint32_t len);
int ext2_close(ext2_file_t* file);
int ext2_write(ext2_file_t* file, const void* buf, uint32_t len);
ext2_file_t* ext2_open(const char* filename, const char* mode);
int ext2_read(ext2_file_t* file, void* buf, uint32_t len);

#endif // EXT_2
