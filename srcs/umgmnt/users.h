#ifndef USERS_H
#define USERS_H

#include "../utils/stdint.h"
#include "../utils/utils.h"

#define INIT_USERS_CONFIG   "/users.config"
#define USERS_CONFIG        "/etc/users.config"
#define NO_USER_LOGIN       "root"
#define NO_USER_PASS        "root"

#define MAX_USER_NAME    32
#define SHA256_HEX_LEN   64
#define SALT_LEN         16

typedef struct
{
    char name[MAX_USER_NAME];
    char pass_hash[SHA256_HEX_LEN + 1];
    uint32_t uid;
    uint32_t gid;
    uint32_t home_inode;  /* Inode number of home directory */
    uint32_t shell_inode; /* Inode number of shell executable */
    bool is_valid;        /* Mark entry as active/deleted */
} user_t;

bool find_user_by_name(const char *name, user_t* u);
int check_password(const char *password, const char *encrypt);
int encrypt_password(const char *password, char *encrypt);
void add_user(user_t *new_user);

void init_users_api();
void list_users();

bool user_exists(const char *name);
bool current_user_is_valid();

#endif