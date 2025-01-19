#ifndef KSHELL_H
#define KSHELL_H

void kshell();
void init_kshell();
void install_command(const char* cmd, const char* desc, void (*func)());

#endif
