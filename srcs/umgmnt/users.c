#include "users.h"
#include "../ide/ext2.h"
#include "../utils/utils.h"
#include "../utils/stdint.h"

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

void add_user(user_t *new_user)
{
    ext2_file_t *fp = ext2_open("users.config", "ab");
    if (!fp)
    {
        return;
    }
    ext2_write(fp, new_user, sizeof(user_t));
    ext2_close(fp);
}
