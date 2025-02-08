#include "users.h"
#include "../ide/ext2.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"

#include "../display/display.h"

user_t find_user_by_name(const char *name)
{
    ext2_file_t *fp = ext2_open("users.config", "rb");
    if (!fp)
    {
        return (user_t){ .is_valid = false };
    }
    user_t u;

    while (ext2_read(fp, &u, sizeof(user_t)))
    {
        if (u.is_valid && strcmp(u.name, name) == 0)
        {
            ext2_close(fp);
            return u;
        }
    }

    ext2_close(fp);
    return (user_t){ .is_valid = false };
}

bool user_exists(const char *name)
{
    return find_user_by_name(name).is_valid;
}

void add_user(user_t *new_user)
{
    if (user_exists(new_user->name))
    {
        printf("User '%s' already exists.\n", new_user->name);
        return;
    }
    ext2_file_t *fp = ext2_open("users.config", "a");
    if (!fp)
    {
        return;
    }
    ext2_write(fp, new_user, sizeof(user_t));
    ext2_close(fp);
}

void list_users()
{
    ext2_file_t *fp = ext2_open("users.config", "rb");
    if (!fp)
    {
        return;
    }
    user_t u;

    while (ext2_read(fp, &u, sizeof(user_t)))
    {
        printf("User: '%s' %s\n", u.name, u.is_valid ? "active" : "deleted");
    }

    ext2_close(fp);
}

