#ifndef USHELL_H
#define USHELL_H

typedef struct
{
    const char* cmd;
    const char* desc;
    void (*func)(char** args, int argc);
} u_command_t;

typedef enum {
    FOO = 0,
    U_SECTION_T_MAX
} u_section_t;

void ushell(void);

#endif