#include "server.h"

ExitCode run_as_server(int32_t socket_fd, struct addrinfo* address, ArgsPtr args) {
    ExitCode exit = EXIT_SUCCESS;
    int32_t out_fd = open_file(args->file_out, args->app_side);
    if(out_fd == -1) return EXIT_OPEN;

    if(bind_socket(socket_fd, address) != EXIT_SUCCESS) {
        return EXIT_SOCKET;
    }

    // handshake
    uint32_t expected_seq, conn_id;
    exit = server_handle_handshake(socket_fd, args->timeout_sec, &expected_seq, &conn_id);
    if(exit) return exit;
    // data transfer
    exit = receive_data(socket_fd, out_fd, args->timeout_sec, expected_seq, conn_id);
    // teardown

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
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) return exit;
        // at max returned MAX_PROTOCOL_SIZE bytes, int is enough
        int32_t received = recvfrom(socket_fd, buffer, MAX_PROTOCOL_SIZE, MSG_DONTWAIT, (struct sockaddr *)&client_addr, &client_addr_size);
        if (received <= 0) {
            // not timeout yet (socket timeout)
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }
        
        // check malformed packet
        if(check_malformed((unsigned char*)buffer, received, conn_id) != EXIT_SUCCESS) continue;
        if(((ProtocolHeaderPtr) buffer)->flags != SYN) continue; 
        break;
    }
    // set conn id
    *conn_id = ((ProtocolHeaderPtr) buffer)->conn_id;

    // connect
    if(connect(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size) == -1) {
        perror("connect");
        return EXIT_SOCKET;
    }

    *expected_seq = ((ProtocolHeaderPtr) buffer)->seq_num;

    // create SYN + ACK and send back
    char message_back[HEADER_SIZE];
    char* msg = message_back;
    uint32_t server_seq = rand();
    create_header(SYN | ACK, NULL, 0, &msg, *conn_id, server_seq, ++(*expected_seq));
    
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

    // start resend timeout
    struct timespec resend_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &resend_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // wait for empty ack, or retransmit
    while(1) {
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) return exit;

        // resend if no response
        exit = resolve_timeout(resend_timeout, RESEND_TIMEOUT);
        if(exit) {
            // resend
            int32_t bytes = send(socket_fd, msg, HEADER_SIZE, 0);
            if(bytes <= 0) {
                perror("sendto");
                return EXIT_SOCKET;
            }
            // reset timer
            if(clock_gettime(CLOCK_MONOTONIC, &resend_timeout) != 0) {
                perror("clock_gettime");
                return EXIT_CLOCK;
            }
        }
        // receive from connected
        int32_t received = recv(socket_fd, buffer, MAX_PROTOCOL_SIZE, MSG_DONTWAIT);
        if(received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }
        
        // check malformed packet
        if(check_malformed((unsigned char*)buffer, received, conn_id) != EXIT_SUCCESS) continue;
    
        ProtocolHeaderPtr header = (ProtocolHeaderPtr) buffer;
        if(header->flags != ACK) continue; 
        if(*expected_seq + 1 == header->seq_num && header->ack_num == server_seq + 1) break;
    }
    // sucessfull handshake
    (*expected_seq)++;
    return EXIT_SUCCESS;
}


ExitCode receive_data(int32_t socket_fd, 
                        int32_t out_file,
                        uint32_t max_timeout, 
                        uint32_t* seq,
                        uint32_t* conn_id
                    ) {
    // buffer for receiving messages
    char received_message[MAX_PROTOCOL_SIZE];
    ExitCode exit = EXIT_SUCCESS;

    // start timeout
    struct timespec last_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // infinite loop 
    while(1) {
        // check timeout
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) return exit;

        // receive DATA packet
        int32_t received = recv(socket_fd, received_message, MAX_PROTOCOL_SIZE, 0);
        if(received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }

        // validate packet
        if(check_malformed((unsigned char*)received_message, received, conn_id) != EXIT_SUCCESS) continue;

        ProtocolHeaderPtr header = (ProtocolHeaderPtr) received_message;
        // check for data flag
        if(header->flags == DATA) {
            // check sequence number
            if(header->seq_num != *seq) continue;

            // write data to file
            if(write(out_file, header->data, header->payload_size) == -1) {
                perror("write");
                return EXIT_WRITE;
            }

            // send ACK back
            char ack_msg[HEADER_SIZE];
            char* msg = ack_msg;
            create_header(ACK, NULL, 0, &msg, *conn_id, *seq, header->seq_num + 1);
            
            int32_t bytes = send(socket_fd, msg, HEADER_SIZE, 0);
            if(bytes <= 0) {
                perror("send");
                return EXIT_SOCKET;
            }

            // reset timeout on progress
            if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
                perror("clock_gettime");
                return EXIT_CLOCK;
            }
        }
        else if(header->flags == FIN) {
            break;
        }
        else {
            continue;
        }
    }
    
    return EXIT_SUCCESS;
}
