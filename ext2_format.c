#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define EXT2_BLOCK_SIZE    1024
#define TOTAL_BLOCKS       8192
#define INODES_COUNT       1024
#define FIRST_DATA_BLOCK   1
#define BLOCKS_PER_GROUP   8192
#define INODES_PER_GROUP   1024

#define S_IFDIR  0x4000
#define S_IFREG  0x8000

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

typedef struct
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[255];
} ext2_dir_entry_t;

static uint32_t next_free_inode = 5;
static uint32_t next_free_block = 13;

void write_block(FILE *fp, uint32_t block_num, const void *buf)
{
    if (fseek(fp, block_num * EXT2_BLOCK_SIZE, SEEK_SET) != 0)
        exit(1);
    if (fwrite(buf, EXT2_BLOCK_SIZE, 1, fp) != 1)
        exit(1);   
}

void write_inode(FILE *fp, uint32_t inode_num, const ext2_inode_t *inode)
{
    uint32_t index = inode_num - 1;
    uint32_t table_block = 5;
    uint32_t inodes_per_block = EXT2_BLOCK_SIZE / sizeof(ext2_inode_t);
    uint32_t block_in_table = index / inodes_per_block;
    uint32_t offset_in_block = (index % inodes_per_block) * sizeof(ext2_inode_t);
    uint8_t block_buf[EXT2_BLOCK_SIZE];
    memset(block_buf, 0, EXT2_BLOCK_SIZE);

    fseek(fp, (table_block + block_in_table) * EXT2_BLOCK_SIZE, SEEK_SET);
    fread(block_buf, EXT2_BLOCK_SIZE, 1, fp);
    memcpy(block_buf + offset_in_block, inode, sizeof(ext2_inode_t));
    write_block(fp, table_block + block_in_table, block_buf);
}

void add_dir_entry_to_root(FILE *fp, const char *fs_name, uint32_t file_inode)
{
    uint8_t block_buf[EXT2_BLOCK_SIZE];
    if (fseek(fp, 10 * EXT2_BLOCK_SIZE, SEEK_SET) != 0)
        exit(1);
    if (fread(block_buf, EXT2_BLOCK_SIZE, 1, fp) != 1)
        exit(1);
    
    uint32_t offset = 0;
    ext2_dir_entry_t *entry = NULL;
    while (offset < EXT2_BLOCK_SIZE)
    {
        entry = (ext2_dir_entry_t *)(block_buf + offset);
        if (entry->rec_len == 0) break;
        offset += entry->rec_len;
        if (offset >= EXT2_BLOCK_SIZE) break;
    }
    
    uint32_t last_offset = offset - entry->rec_len;
    entry = (ext2_dir_entry_t *)(block_buf + last_offset);
    uint16_t ideal_len = ((8 + entry->name_len) + 3) & ~3;
    uint16_t available = entry->rec_len - ideal_len;
    uint8_t new_name_len = (uint8_t)strlen(fs_name);
    uint16_t new_entry_size = ((8 + new_name_len) + 3) & ~3;
    if (available < new_entry_size)
        exit(1);

    entry->rec_len = ideal_len;
    uint32_t new_entry_offset = last_offset + ideal_len;
    ext2_dir_entry_t *new_entry = (ext2_dir_entry_t *)(block_buf + new_entry_offset);
    new_entry->inode = file_inode;
    new_entry->name_len = new_name_len;
    new_entry->file_type = 1;
    new_entry->rec_len = EXT2_BLOCK_SIZE - new_entry_offset;
    memcpy(new_entry->name, fs_name, new_name_len);
    new_entry->name[new_name_len] = '\0';
    write_block(fp, 10, block_buf);
}

void import_file(FILE *fp, const char *hfile, const char *fs_name)
{
    FILE *hp = fopen(hfile, "rb");
    if (!hp)
        exit(1);
    
    fseek(hp, 0, SEEK_END);
    long filesize = ftell(hp);
    rewind(hp);

    if (filesize > 12 * EXT2_BLOCK_SIZE)
    {
        fprintf(stderr, "File too large to import.\n");
        exit(1);
    }
    char *buffer = malloc(filesize);
    if (!buffer)
        exit(1);
    
    if (fread(buffer, 1, filesize, hp) != filesize)
        exit(1);
    
    fclose(hp);

    uint32_t new_inode = next_free_inode++;
    ext2_inode_t new_file;
    memset(&new_file, 0, sizeof(new_file));
    new_file.i_mode = S_IFREG | 0644;
    new_file.i_size = filesize;
    new_file.i_links_count = 1;
    new_file.i_blocks = 0;

    int blocks_needed = (filesize + EXT2_BLOCK_SIZE - 1) / EXT2_BLOCK_SIZE;
    if (blocks_needed > 12)
        exit(1);
    
    for (int i = 0; i < blocks_needed; i++)
    {
        uint32_t new_block = next_free_block++;
        new_file.i_block[i] = new_block;
    
        uint8_t *chunk = (uint8_t*)buffer + (i * EXT2_BLOCK_SIZE);
    
        uint32_t chunk_size = (filesize - i * EXT2_BLOCK_SIZE > EXT2_BLOCK_SIZE) ? EXT2_BLOCK_SIZE : (filesize - i * EXT2_BLOCK_SIZE);
    
        uint8_t block_buf[EXT2_BLOCK_SIZE];
        memset(block_buf, 0, EXT2_BLOCK_SIZE);
        memcpy(block_buf, chunk, chunk_size);
        write_block(fp, new_block, block_buf);
    }
    free(buffer);

    write_inode(fp, new_inode, &new_file);

    add_dir_entry_to_root(fp, fs_name, new_inode);
    printf("Imported file '%s' as '%s' (inode #%u, %d blocks, size %ld bytes).\n",
           hfile, fs_name, new_inode, blocks_needed, filesize);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s disk_image [file1 file2 ...]\n", argv[0]);
        return 1;
    }
    const char *disk_image = argv[1];
    FILE *fp = fopen(disk_image, "r+b");
    if (!fp)
        return 1;
    
    fseek(fp, TOTAL_BLOCKS * EXT2_BLOCK_SIZE - 1, SEEK_SET);
    fputc(0, fp);

    uint8_t block_buf[EXT2_BLOCK_SIZE];
    memset(block_buf, 0, EXT2_BLOCK_SIZE);

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
    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    memcpy(block_buf, &sb, sizeof(sb));
    write_block(fp, 1, block_buf);

    ext2_group_desc_t gd;
    memset(&gd, 0, sizeof(gd));
    gd.bg_block_bitmap = 3;
    gd.bg_inode_bitmap = 4;
    gd.bg_inode_table = 5;
    gd.bg_free_blocks_count = 0;
    gd.bg_free_inodes_count = 0;
    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    memcpy(block_buf, &gd, sizeof(gd));
    write_block(fp, 2, block_buf);

    ext2_inode_t root;
    memset(&root, 0, sizeof(root));
    root.i_mode = S_IFDIR | 0755;
    root.i_size = EXT2_BLOCK_SIZE;
    root.i_links_count = 2;
    root.i_block[0] = 10;
    write_inode(fp, 2, &root);

    ext2_inode_t foo;
    memset(&foo, 0, sizeof(foo));
    foo.i_mode = S_IFDIR | 0755;
    foo.i_size = EXT2_BLOCK_SIZE;
    foo.i_links_count = 2;
    foo.i_block[0] = 11;
    write_inode(fp, 3, &foo);

    const char *file_content = "Hello from ext2!\n";
    size_t file_len = strlen(file_content);
    ext2_inode_t hello;
    memset(&hello, 0, sizeof(hello));
    hello.i_mode = S_IFREG | 0644;
    hello.i_size = file_len;
    hello.i_links_count = 1;
    hello.i_block[0] = 12;
    write_inode(fp, 4, &hello);

    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    memcpy(block_buf, file_content, file_len);
    write_block(fp, 12, block_buf);

    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    ext2_dir_entry_t *d = (ext2_dir_entry_t *)block_buf;
    d->inode = 2;
    d->name_len = 1;
    d->file_type = 2;
    d->rec_len = 12;
    d->name[0] = '.';

    ext2_dir_entry_t *d2 = (ext2_dir_entry_t *)((uint8_t *)d + d->rec_len);
    d2->inode = 2;
    d2->name_len = 2;
    d2->file_type = 2;
    d2->rec_len = 12;
    d2->name[0] = '.';
    d2->name[1] = '.';

    ext2_dir_entry_t *d3 = (ext2_dir_entry_t *)((uint8_t *)d2 + d2->rec_len);
    d3->inode = 3;
    d3->name_len = 3;
    d3->file_type = 2;
    d3->rec_len = 12;
    d3->name[0] = 'f';
    d3->name[1] = 'o';
    d3->name[2] = 'o';

    ext2_dir_entry_t *d4 = (ext2_dir_entry_t *)((uint8_t *)d3 + d3->rec_len);
    d4->inode = 4;
    d4->name_len = 9;
    d4->file_type = 1;
    d4->rec_len = EXT2_BLOCK_SIZE - ((uint8_t *)d4 - block_buf);
    memcpy(d4->name, "hello.txt", 9);
    d4->name[9] = '\0';
    write_block(fp, 10, block_buf);

    memset(block_buf, 0, EXT2_BLOCK_SIZE);
    d = (ext2_dir_entry_t *)block_buf;
    d->inode = 3;
    d->name_len = 1;
    d->file_type = 2;
    d->rec_len = 12;
    d->name[0] = '.';
    d2 = (ext2_dir_entry_t *)((uint8_t *)d + d->rec_len);
    d2->inode = 2;
    d2->name_len = 2;
    d2->file_type = 2;
    d2->rec_len = EXT2_BLOCK_SIZE - 12;
    d2->name[0] = '.';
    d2->name[1] = '.';
    write_block(fp, 11, block_buf);

    for (int i = 2; i < argc; i++)
    {
        import_file(fp, argv[i], argv[i]);
    }

    fclose(fp);
    printf("Preformatted disk image '%s' created successfully.\n", disk_image);
    return 0;
}
