#ifndef TIMERS_H
#define TIMERS_H

void init_timer();
void irq_handler_timer();
void sleep(uint32_t seconds);
void get_kuptime(uint64_t *uptime);

#endif