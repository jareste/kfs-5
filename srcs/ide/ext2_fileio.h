#ifndef EXT2_FILEIO_H
#define EXT2_FILEIO_H

#include "../utils/stdint.h"
#include "../utils/utils.h"
#include "ext2.h"

typedef struct ext2_FILE
{
    uint32_t inode_num;         /* which inode this file refers to */
    struct ext2_inode inode;    /* cached inode */
    uint32_t pos;               /* current read/write offset */
    int mode;                   /* 0=read, 1=write (for simplicity) */
} ext2_FILE;

ext2_FILE *ext2_fopen(const char *path, const char *mode);
size_t ext2_fread(void *ptr, size_t size, size_t nmemb, ext2_FILE *stream);
size_t ext2_fwrite(const void *ptr, size_t size, size_t nmemb, ext2_FILE *stream);
int ext2_fclose(ext2_FILE *stream);

#endif
