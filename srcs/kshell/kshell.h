#ifndef KSHELL_H
#define KSHELL_H

typedef struct
{
    const char* cmd;
    const char* desc;
    void (*func)();
} command_t;

void kshell();
void init_kshell();
// void install_command(const char* cmd, const char* desc, void (*func)());
void install_all_cmds(command_t* cmds);

#endif
