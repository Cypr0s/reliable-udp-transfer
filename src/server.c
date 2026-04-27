#include "server.h"

#define GET_TIME()

ExitCode run_as_server(int32_t socket_fd, struct addrinfo* address, ArgsPtr args) {
    int out_fd = open_file(args->file_out, args->app_side);
    if(out_fd == -1) {
        return EXIT_OPEN;
    }
    if(bind_socket(socket_fd, address) != EXIT_SUCCESS) {
        close(out_fd);
        return EXIT_SOCKET;
    }

    // handshake
    // data transfer
    // teardown

    close(out_fd);
    return EXIT_SUCCESS;
}

ExitCode server_handle_handshake(int32_t socket_fd, 
                                uint32_t max_timeout, 
                                struct sockaddr_storage* client_addr, 
                                struct socklen_t* client_address_size,
                                uint32_t expected_seq
                            ) {
    // buffer for holding incoming packets
    char buffer[MAX_PROTOCOL_SIZE];

    // start first timer
    struct timespec last_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }


    ExitCode exit;
    while(1) {
        resolve_timeout(last_timeout, max_timeout);
        if(exit) return exit;
        // at max returned MAX_PROTOCOL_SIZE bytes, int is enough
        int32_t message = recvfrom(socket_fd, buffer, MAX_PROTOCOL_SIZE, MSG_DONTWAIT, (struct sockaddr *) client_addr, client_address_size);
        if (message <= 0) {
            return;
        }
    }

}