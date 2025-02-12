#ifndef FS_H
#define FS_H

#include <stdint.h>

#define MAX_NAME_LENGTH 256

typedef enum
{
    FS_NODE_UNKNOWN = 0,
    FS_NODE_FILE,
    FS_NODE_DIR
} fs_node_type_t;


typedef struct fs_node
{
    char *name;
    uint32_t size;
    fs_node_type_t type;
    uint32_t inode;
    uint16_t links;
    uint16_t rights;
    
    struct fs_node *master;
    struct fs_node *father;
    struct fs_node *children;
    struct fs_node *next_of_kin;
} fs_node_t;

fs_node_t* fs_create_node(const char* name, uint32_t size, fs_node_type_t type,
                           uint32_t inode, uint16_t links, uint16_t rights);

void fs_destroy_tree(fs_node_t* node);

fs_node_t* fs_build_tree_from_inode(uint32_t inode_num, const char* name,
                                      fs_node_t* father, fs_node_t* master);

void fs_print_tree(fs_node_t* node, int indent);

#endif // FS_H
