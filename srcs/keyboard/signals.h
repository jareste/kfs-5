#ifndef SIGNALS_H
#define SIGNALS_H

typedef void (*signal_handler_t)(int);
typedef int pid_t;

void signal(int signal, signal_handler_t handler);
void kill(pid_t pid, int signal);
void handle_signals();
void init_signals();

#endif