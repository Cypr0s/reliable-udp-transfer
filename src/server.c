#include "server.h"

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
                                uint32_t* expected_seq,
                                uint32_t* conn_id
                            ) {
    // buffer for holding incoming packets
    char buffer[MAX_PROTOCOL_SIZE];

    // sender addr
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    // start first timer
    struct timespec last_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    ExitCode exit = EXIT_SUCCESS;
    // wait for empty SYN until timeout
    while(1) {
        exit = resolve_timeout(last_timeout, max_timeout);
        if(exit) return exit;
        // at max returned MAX_PROTOCOL_SIZE bytes, int is enough
        int32_t received = recvfrom(socket_fd, buffer, MAX_PROTOCOL_SIZE, MSG_DONTWAIT, (struct sockaddr *)&client_addr, &client_addr_size);
        if (received <= 0) {
            // not timeout yet (socket timeout)
            if(received == EAI_AGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }
        
        // check malformed packet
        if(check_malformed((unsigned char*)buffer, received, SYN, conn_id) != EXIT_SUCCESS) continue;
        break;
    }
    // set conn id
    *conn_id = ((ProtocolHeaderPtr) buffer)->conn_id;

    // connect
    if(connect(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size) == -1) {
        perror("connect");
        return exit;
    }

    uint32_t message_seq = ((ProtocolHeaderPtr) buffer)->seq_num;

    // create SYN + ACK and send back
    char message_back[HEADER_SIZE];
    char* msg = message_back;

    create_header(SYN | ACK, NULL, 0, &msg, *conn_id, message_seq, message_seq + 1);
    
    // send
    int32_t bytes = send(socket_fd, msg, HEADER_SIZE, 0);
    if(bytes <= 0) {
        perror("sendto");
        return EXIT_SOCKET;
    }

    // update timeout
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // wait for empty ack
    while(1) {
        exit = resolve_timeout(last_timeout, max_timeout);
        if(exit) return exit;
        
        // receive from connected
        int32_t received = recv(socket_fd, buffer, MAX_PROTOCOL_SIZE, MSG_DONTWAIT);
        if(received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }
        
        // check malformed packet
        if(check_malformed((unsigned char*)buffer, received, ACK, conn_id) != EXIT_SUCCESS) continue;
        break;
    }
    // sucessfull handshake
    *expected_seq = ((ProtocolHeaderPtr) buffer)->seq_num + 1;
    return EXIT_SUCCESS;
}
