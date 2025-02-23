#include "users.h"
#include "../ide/ext2.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"

#include "../display/display.h"

bool find_user_by_name(const char *name, user_t* u)
{
    // ext2_file_t *fp;
    
    // fp = ext2_open(USERS_CONFIG, "rb");
    // if (!fp)
    // {
    //     // printf("Config file not found.\n");
    //     // if (strcmp(name, NO_USER_LOGIN) == 0)
    //     // {
    //     //     strcpy(u->name, "root");
    //     //     strcpy(u->pass_hash, "foo");
    //     //     u->uid = 0;
    //     //     u->gid = 0;
    //     //     u->home_inode = 2;
    //     //     u->shell_inode = 2;
    //     //     u->is_valid = true;
    //     //     printf("Login as root.\n");
    //     //     return true;
    //     // }
    //     return false;
    // }

    // while (ext2_read(fp, u, sizeof(user_t)))
    // {
    //     if (u->is_valid && strcmp(u->name, name) == 0)
    //     {
    //         ext2_close(fp);
    //         return true;
    //     }
    // }

    // ext2_close(fp);
    // printf("User '%s' not found.\n", name);
    return false;
}

bool user_exists(const char *name)
{
    user_t u;
    return find_user_by_name(name, &u);
}

void add_user(user_t *new_user)
{
    // ext2_file_t *fp;

    // if (user_exists(new_user->name))
    // {
    //     printf("User '%s' already exists.\n", new_user->name);
    //     return;
    // }
    // fp = ext2_open(USERS_CONFIG, "a");
    // if (!fp)
    // {
    //     return;
    // }
    // ext2_write(fp, new_user, sizeof(user_t));
    // ext2_close(fp);
}

void list_users()
{
    // ext2_file_t *fp;
    // user_t u;

    // fp = ext2_open(USERS_CONFIG, "rb");
    // if (!fp)
    // {
    //     return;
    // }

    // while (ext2_read(fp, &u, sizeof(user_t)))
    // {
    //     printf("User: '%s' %s\n", u.name, u.is_valid ? "active" : "deleted");
    // }

    // ext2_close(fp);
}

void check_file_location()
{
    // ext2_file_t *fp;
    // fp = ext2_open(USERS_CONFIG, "rb");
    // if (fp)
    // {
    //     ext2_close(fp);
    //     return;
    // }
    // fp = ext2_open(INIT_USERS_CONFIG, "rb");
    // if (!fp)
    // {
    //     printf("No users config file found.\n");
    //     return;
    // }
    // ext2_close(fp);
    // mv_file(INIT_USERS_CONFIG, USERS_CONFIG);
    return;
}
