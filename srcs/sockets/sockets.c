#include "sockets.h"
#include "../memory/memory.h"
#include "../tasks/task.h"

#define MAX_SOCKETS 1024
#define DEFAULT_SOCKET_BUFFER_SIZE 1024

static kernel_socket_t socket_table[MAX_SOCKETS];

static void notify_task(int target_pid)
{
    task_t *target_task = get_task_by_pid(target_pid);
    if (target_task)
    {
        target_task->state = TASK_READY;
    }
}

int _socket()
{
    int source_pid = get_current_task()->pid;
    for (int i = 0; i < MAX_SOCKETS; i++)
    {
        if (!socket_table[i].is_bound)
        {
            socket_table[i].socket_id = i;
            socket_table[i].source_pid = source_pid;
            socket_table[i].buffer = kmalloc(DEFAULT_SOCKET_BUFFER_SIZE);
            socket_table[i].buffer_size = DEFAULT_SOCKET_BUFFER_SIZE;
            socket_table[i].is_bound = 1;
            return i;
        }
    }
    return -1;
}

int socket_connect(int socket_id)
{
    kernel_socket_t *sock = &socket_table[socket_id];
    if (!sock->is_bound) return -1;

    int target_pid = get_current_task()->pid;

    sock->target_pid = target_pid;
    sock->is_connected = 1;
    return 0;
}

int socket_send(int socket_id, char *data, size_t length)
{
    kernel_socket_t *sock = &socket_table[socket_id];
    if (!sock->is_connected) return -1;
    if (length > sock->buffer_size) return -1;

    memcpy(sock->buffer, data, length);

    notify_task(sock->target_pid);
    return 0;
}

int socket_recv(int socket_id, char *buffer, size_t length)
{
    kernel_socket_t *sock = &socket_table[socket_id];

    get_current_task()->state = TASK_WAITING;

    while (sock->buffer[0] == '\0') scheduler();

    get_current_task()->state = TASK_RUNNING;

    if (length > sock->buffer_size) return -1;

    memcpy(buffer, sock->buffer, length);
    memset(sock->buffer, 0, sock->buffer_size);
    return 0;
}
