#ifndef TIMERS_H
#define TIMERS_H

void init_timer();
void irq_handler_timer();
void sleep(uint32_t seconds);

#endif