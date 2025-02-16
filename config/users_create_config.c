#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

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

user_t find_user_by_name(const char *name)
{
    FILE *fp = fopen(USERS_CONFIG, "rb");
    if (!fp)
    {
        return (user_t){ .is_valid = false };
    }
    user_t u;

    while (fread(&u, sizeof(user_t), 1, fp))
    {
        if (u.is_valid && strcmp(u.name, name) == 0)
        {
            fclose(fp);
            return u;
        }
    }

    fclose(fp);
    return (user_t){ .is_valid = false };
}

void add_user(user_t *new_user)
{
    FILE *fp = fopen(USERS_CONFIG, "ab");
    fwrite(new_user, sizeof(user_t), 1, fp);
    fclose(fp);
}

int main()
{
    user_t u = find_user_by_name("root");
    if (u.is_valid)
    {
        printf("User 'root' already exists.\n");
        printf("User name: %s\n", u.name);
        printf("Password hash: %s\n", u.pass_hash);
        printf("UID: %u\n", u.uid);
        printf("GID: %u\n", u.gid);
        printf("Home inode: %u\n", u.home_inode);
        printf("Shell inode: %u\n", u.shell_inode);
    }
    else
    {
        user_t new_user = {
            .uid = 0,
            .gid = 0,
            .home_inode = 2,
            .shell_inode = 3,
            .is_valid = true
        };
        strcpy(new_user.name, "root");
        strcpy(new_user.pass_hash, "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92");
        add_user(&new_user);
        printf("User 'root' added successfully. '%d'\n", new_user.is_valid);
    }
}
