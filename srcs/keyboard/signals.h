#ifndef SIGNALS_H
#define SIGNALS_H

typedef struct
{
    signal_handler_t handlers[MAX_SIGNALS]; /* Handler for each signal, signal() would set it. */
    uint32_t pending_signals; /* 'flag' of pending signals */
    uint32_t blocked_signals; /* 'flag' of blocked signals */
} signal_context_t;

typedef void (*signal_handler_t)(int);
typedef int pid_t;

void signal(int signal, signal_handler_t handler);
void kill(pid_t pid, int signal);
void handle_signals();
void init_signals();

#endif