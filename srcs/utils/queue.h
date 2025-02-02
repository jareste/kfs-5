#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_SIZE 10

typedef struct
{
    int pid;
    int status;
} data_t;

typedef struct
{
    data_t  data[QUEUE_SIZE];
    int     front;
    int     rear;
} Queue;

void init_queue(Queue *q);
void enqueue(Queue *q, int pid, int status);
int dequeue(Queue *q, data_t *data);
int is_empty(Queue *q);
int is_full(Queue *q);

#endif /* QUEUE_H */