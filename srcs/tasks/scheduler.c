#include "task.h"

task_t *current_task;
task_t *task_list[MAX_TASKS];
int task_count = 0;

void switch_to_task(task_t *next_task)
{
    /* i guess i must save current task state */
    save_state(current_task);

    /* and load next task state */
    load_state(next_task);

    current_task = next_task;
}

void schedule()
{
    int next_task_index = (current_task_index + 1) % task_count;
    current_task = task_list[next_task_index];
    switch_to_task(current_task);
}
