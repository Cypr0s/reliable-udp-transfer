/** ------------- Reliable UDP Transfer ---------------
 * @headerfile  socket.h
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Contains socket handling functions declarations
 */


#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h> // socket functions
#include <unistd.h> // close fd
#include "error.h" // error codes
#include <stdint.h> // types
#include <stdio.h> // fprintf, stderr
#include <netdb.h> // struct addrinfo

int32_t create_socket(struct addrinfo* address);

ExitCode bind_socket(int32_t socket_fd, struct addrinfo* address);

ExitCode set_receive_timeout(int32_t socket_fd, int32_t ms);


#endif // SOCKET_H