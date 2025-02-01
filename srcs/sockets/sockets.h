#ifndef SOCKETS_H
#define SOCKETS_H

#include "../utils/utils.h"
#include "../utils/stdint.h"

typedef struct
{
    int socket_id;
    int source_pid; /* Who created the socket */
    int target_pid; /* Who is the socket bound to */
    char *buffer; /* Buffer to be sent */
    size_t buffer_size;
    bool is_bound; /* Indicates if a process is bound to the socket */
    bool is_connected; /* Socket is connected ? */
} kernel_socket_t;


#endif // SOCKETS_H
