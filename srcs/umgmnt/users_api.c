#include "users.h"
#include "../utils/utils.h"
#include "../display/display.h"
#include "../kshell/kshell.h"
#include "../keyboard/keyboard.h"

static void cmd_logout();
static void cmd_login();

static command_t users_commands[] = {
    {"login", "Login to the system", cmd_login},
    {"logout", "Logout from the system", cmd_logout},
    {NULL, NULL, NULL}
};

static int login(char *username, char *password)
{
    user_t u;
    if (find_user_by_name(username, &u) == false)
    {
        printf("User '%s' not found.\n", username);
        return -1;
    }

    if (check_password(password, u.pass_hash) == 0)
    {
        printf("Invalid password.\n");
        return -1;
    }

    printf("Login successful as '%s'.\n", username);
    return 0;
}

static void cmd_login()
{
    char* buffer;
    char username[MAX_USER_NAME];
    char password[SHA256_HEX_LEN + 1];
 
    printf("Username: ");
    buffer = get_line();
    strcpy(username, buffer);
    printf("Password: ");
    start_ofuscation();
    buffer = get_line();
    stop_ofuscation();
    strcpy(password, buffer);

    login(username, password);
}

static void cmd_logout()
{
    printf("Logout done!\n");
}

void init_users_api()
{
    install_all_cmds(users_commands, GLOBAL);
}
