#include "users.h"
#include "../ide/ext2.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"

#include "../display/display.h"

bool find_user_by_name(const char *name, user_t* u)
{
    ext2_file_t *fp;
    
    fp = ext2_open("users.config", "rb");
    if (!fp)
    {
        return false;
    }

    while (ext2_read(fp, u, sizeof(user_t)))
    {
        if (u->is_valid && strcmp(u->name, name) == 0)
        {
            ext2_close(fp);
            return true;
        }
    }

    ext2_close(fp);
    return false;
}

bool user_exists(const char *name)
{
    user_t u;
    return find_user_by_name(name, &u);
}

void add_user(user_t *new_user)
{
    ext2_file_t *fp;

    if (user_exists(new_user->name))
    {
        printf("User '%s' already exists.\n", new_user->name);
        return;
    }
    fp = ext2_open("users.config", "a");
    if (!fp)
    {
        return;
    }
    ext2_write(fp, new_user, sizeof(user_t));
    ext2_close(fp);
}

void list_users()
{
    ext2_file_t *fp;
    user_t u;

    fp = ext2_open("users.config", "rb");
    if (!fp)
    {
        return;
    }

    while (ext2_read(fp, &u, sizeof(user_t)))
    {
        printf("User: '%s' %s\n", u.name, u.is_valid ? "active" : "deleted");
    }

    ext2_close(fp);
}

