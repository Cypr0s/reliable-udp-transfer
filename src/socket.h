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


#endif // SOCKET_H