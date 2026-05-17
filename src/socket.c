/** ------------- Reliable UDP Transfer ---------------
 * @file        socket.c
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Contains socket handling functions definitions
 */

#include "socket.h"

/**
 * @brief   creates socket based on provided address info, taken from and implemented recursively
 *          https://git.fit.vutbr.cz/NESFIT/IPK-Examples/src/branch/main/examples/c/DemoUdp/client.c
 * @param   address - struct addrinfo with
 * @return  socket file descriptor on success or -1 on error
 */
int32_t create_socket(struct addrinfo* address) {
    if(address == NULL) {
        fprintf(stderr, "No more addresses to try creating sockets for\n");
        return -1;
    }
    // create socket
    int32_t socket_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if(socket_fd == -1) {
        perror("socket");
        return create_socket(address->ai_next);
    }
    return socket_fd;
} // create_socket


/**
 * @brief           binds socket to address
 * 
 * @param socket_fd socket file descriptor
 * @param address   address which will be socket binded to
 * @return          EXIT_SUCCESS on success, 
 *                  EXIT_SOCKET on bind error
 */
ExitCode bind_socket(int32_t socket_fd, struct addrinfo* address) {
    // invalid socket or address
    if(socket_fd == -1 || address == NULL) {
        return EXIT_SOCKET;
    }

    // bind 
    int32_t result = bind(socket_fd, address->ai_addr, address->ai_addrlen);
    if(result == -1) {
        perror("bind");
        return EXIT_SOCKET;
    }
    return EXIT_SUCCESS;
} // bind_socket


/**
 * @brief           sets the receive timeout for a socket
 * @param socket_fd socket file descriptor
 * @param ms        timeout in milliseconds
 * @return          EXIT_SUCCESS on success, 
 *                  EXIT_SOCKET on setsockopt error
 */
ExitCode set_receive_timeout(int32_t socket_fd, int32_t ms) {
    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = ms * 1000,
    };

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        perror("Setsockopt");
        return EXIT_SOCKET;
    }

    return EXIT_SUCCESS;
} // set_receive_timeout

