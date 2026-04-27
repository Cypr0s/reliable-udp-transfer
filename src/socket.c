

#include "socket.h"

/**
 * @brief   creates socket based on provided address info, taken from and implemented recursively
 *          https://git.fit.vutbr.cz/NESFIT/IPK-Examples/src/branch/main/examples/c/DemoUdp/client.c
 * @param   address - struct addrinfo with
 * @return  socket file descriptor on success or -1 on error
 */
int32_t create_socket(struct addrinfo* address) {
    if (address == NULL) {
        fprintf(stderr, "No more addresses to try creating sockets for\n");
        return -1;
    }

    int32_t socket_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if(socket_fd == -1) {
        perror("socket");
        return create_socket(address->ai_next);
    }
    return socket_fd;
}

ExitCode bind_socket(int32_t socket_fd, struct addrinfo* address) {
    if(socket_fd == -1 || address == NULL) {
        return EXIT_SOCKET;
    }
    int32_t result = bind(socket_fd, address->ai_addr, address->ai_addrlen);
    if(result == -1) {
        perror("bind");
        return EXIT_SOCKET;
    }
    return EXIT_SUCCESS;
}

