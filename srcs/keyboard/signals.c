#include "signals.h"
#include "../utils/utils.h"
#include "../display/display.h"
#include "idt.h"

#define MAX_SIGNALS 32

typedef struct {
    signal_handler_t handlers[MAX_SIGNALS]; /* Handler for each signal, signal() would set it. */
    uint32_t pending_signals; /* 'flag' of pending signals */
} signal_context_t;

/* TODO move it to each task as now we only have the 'core' */
signal_context_t kernel_signals;

/* Same as signal() in Unix */
void signal(int signal, signal_handler_t handler)
{
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        kernel_signals.handlers[signal] = handler;
    }
}

/* Later on, this must be called before each task action, therefore, if the signal
 * is not called on the ongoing task, we must mark it as pending and first thing to
 * do in the task is to handle the signals.
 */
void handle_signals()
{
    for (int i = 0; i < MAX_SIGNALS; i++)
    {
        if (kernel_signals.pending_signals & (1 << i))
        {
            kernel_signals.pending_signals &= ~(1 << i);
            if (kernel_signals.handlers[i])
            {
                kernel_signals.handlers[i](i);
            }
        }
    }
}

/* Same as kill() in Unix */
void kill(pid_t pid, int signal)
{
    if (signal >= 0 && signal < MAX_SIGNALS)
    {
        kernel_signals.pending_signals |= (1 << signal); /* Set the signal */
    }
    handle_signals();
}

static void signal_handler(int signal)
{
    printf("Signal %d received\n", signal);
}

static void panic_signal_handler(int signal)
{
    printf("Panic signal %d received\n", signal);
    kernel_panic("Panic signal received");
}

void init_signals()
{
    for (int i = 0; i < MAX_SIGNALS; i++)
    {
        signal(i, signal_handler);
    }
    signal(0, panic_signal_handler);
    signal(6, panic_signal_handler);
    signal(14, panic_signal_handler);
    signal(13, panic_signal_handler);
    kernel_signals.pending_signals = 0;
}

