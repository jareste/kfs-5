#include "sockets.h"
#include "../memory/memory.h"

#define MAX_SOCKETS 1024
#define DEFAULT_SOCKET_BUFFER_SIZE 1024

kernel_socket_t socket_table[MAX_SOCKETS];

int allocate_socket(int source_pid)
{
    for (int i = 0; i < MAX_SOCKETS; i++)
    {
        if (!socket_table[i].is_bound)
        {
            socket_table[i].socket_id = i;
            socket_table[i].source_pid = source_pid;
            socket_table[i].buffer = kmalloc(DEFAULT_SOCKET_BUFFER_SIZE);
            socket_table[i].is_bound = 1;
            return i;
        }
    }
    return -1;
}

void free_socket(int socket_id)
{
    if (socket_table[socket_id].is_bound)
    {
        kfree(socket_table[socket_id].buffer);
        socket_table[socket_id].is_bound = 0;
    }
}

int socket_send(int socket_id, char *data, size_t length)
{
    kernel_socket_t *sock = &socket_table[socket_id];
    if (!sock->is_connected) return -1;

    if (length > sock->buffer_size) return -1;
    memcpy(sock->buffer, data, length);
    
    /* TODO how to notify the process? */
    return 0;
}

int socket_recv(int socket_id, char *buffer, size_t length)
{
    kernel_socket_t *sock = &socket_table[socket_id];

    if (length > sock->buffer_size) return -1;
    memcpy(buffer, sock->buffer, length);

    memset(sock->buffer, 0, sock->buffer_size);
    return 0;
}
