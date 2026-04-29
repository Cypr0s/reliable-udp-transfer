#include "server.h"

ExitCode run_as_server(int32_t socket_fd, struct addrinfo* address, ArgsPtr args) {
    ExitCode exit = EXIT_SUCCESS;
    int32_t out_fd = open_file(args->file_out, args->app_side);
    if(out_fd == -1) return EXIT_OPEN;

    if(bind_socket(socket_fd, address) != EXIT_SUCCESS) {
        if(out_fd != STDERR_FILENO && out_fd != STDIN_FILENO && out_fd != STDOUT_FILENO) {
            close(out_fd);
        }
        return EXIT_SOCKET;
    }

    // handshake
    uint32_t expected_seq, conn_id;
    exit = server_handshake(socket_fd, args->timeout_sec, &expected_seq, &conn_id);
    if(exit) {
        if(out_fd != STDERR_FILENO && out_fd != STDIN_FILENO && out_fd != STDOUT_FILENO) {
            close(out_fd);
        }
        return exit;
    }
    // data transfer
    exit = receive_data(socket_fd, out_fd, args->timeout_sec, &expected_seq, conn_id);
    if(exit) {
        if(out_fd != STDERR_FILENO && out_fd != STDIN_FILENO && out_fd != STDOUT_FILENO) {
            close(out_fd);
        }
        return exit;
    }
    // teardown
    exit = server_teardown(socket_fd, args->timeout_sec, expected_seq, conn_id);

    if(out_fd != STDERR_FILENO && out_fd != STDIN_FILENO && out_fd != STDOUT_FILENO) {
        close(out_fd);
    }
    return exit;
}

ExitCode server_handshake(int32_t socket_fd, 
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
        if(exit) {
            fprintf(stderr, "Maximum timeout time has passed\n");
            return exit;
        }
        // at max returned MAX_PROTOCOL_SIZE bytes, int is enough
        int32_t received = recvfrom(socket_fd, buffer, MAX_PROTOCOL_SIZE, MSG_DONTWAIT, (struct sockaddr *)&client_addr, &client_addr_size);
        if (received <= 0) {
            // not timeout yet (socket timeout)
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }
        // check malformed packet
        if(check_malformed((unsigned char*)buffer, received) != EXIT_SUCCESS) continue;
        if(((ProtocolHeaderPtr) buffer)->flags != FLAG_SYN) continue; 
        break;
    }
    // set conn id
    *conn_id = ((ProtocolHeaderPtr) buffer)->conn_id;

    // connect
    if(connect(socket_fd, (struct sockaddr*) &client_addr, client_addr_size) == -1) {
        perror("connect");
        return EXIT_SOCKET;
    }

    *expected_seq = ((ProtocolHeaderPtr) buffer)->seq_num;

    // create SYN + ACK and send back
    char message_back[HEADER_SIZE];
    uint32_t server_seq = rand();
    create_header(FLAG_SYN | FLAG_ACK, NULL, 0, message_back, *conn_id, server_seq, ++(*expected_seq));
    
    // send
    int32_t bytes = send(socket_fd, message_back, HEADER_SIZE, 0);
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
        if(exit){
            fprintf(stderr, "Maximum timeout time has passed\n"); 
            return exit;
        }

        // resend if no response
        exit = resolve_timeout(resend_timeout, RESEND_TIMEOUT);
        if(exit) {
            // resend
            int32_t bytes = send(socket_fd, message_back, HEADER_SIZE, 0);
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
        if(check_malformed((unsigned char*)buffer, received) != EXIT_SUCCESS) continue;
        ProtocolHeaderPtr header = (ProtocolHeaderPtr) buffer;
        // check connection id
        if(header->conn_id != *conn_id) continue;
    
        if(header->flags != FLAG_ACK) continue; 

        if(header->ack_num == server_seq + 1) break;
    }
    // sucessfull handshake
    (*expected_seq)++;
    return EXIT_SUCCESS;
}


ExitCode receive_data(int32_t socket_fd, 
                        int32_t out_file,
                        uint32_t max_timeout, 
                        uint32_t* seq,
                        uint32_t conn_id
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
        if(exit) {
            fprintf(stderr, "Maximum timeout time has passed\n");
            return exit;
        }

        // receive DATA packet
        int32_t received = recv(socket_fd, received_message, MAX_PROTOCOL_SIZE, MSG_DONTWAIT);
        if(received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }
        

        // validate packet
        if(check_malformed((unsigned char*)received_message, received) != EXIT_SUCCESS) continue;

        ProtocolHeaderPtr header = (ProtocolHeaderPtr) received_message;
        // check connection id
        if(header->conn_id != conn_id) continue;
        // check sequence number
        if(header->seq_num != *seq) continue;
        // check for data flag
        if(header->flags == FLAG_DATA) {
            // write data to file
            if(write(out_file, header->data, header->payload_size) == -1) {
                perror("write");
                return EXIT_WRITE;
            }

            // send ACK back
            char ack_msg[HEADER_SIZE];
            create_header(FLAG_ACK, NULL, 0, ack_msg, conn_id, *seq, header->seq_num + 1);
            
            int32_t bytes = send(socket_fd, ack_msg, HEADER_SIZE, 0);
            if(bytes <= 0) {
                perror("send");
                return EXIT_SOCKET;
            }

            // reset timeout on progress
            if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
                perror("clock_gettime");
                return EXIT_CLOCK;
            }
            // next expected seq
            (*seq)++;
        }
        else if(header->flags == FLAG_FIN) {
            break;
        }
        else {
            continue;
        }
    }
    
    return EXIT_SUCCESS;
}

ExitCode server_teardown(int32_t socket_fd, 
                         uint32_t max_timeout, 
                         uint32_t seq,
                         uint32_t conn_id
                        ) {
    // send FIN+ACK
    char message[MAX_PROTOCOL_SIZE];
    uint32_t server_seq = rand();
    create_header(FLAG_FIN | FLAG_ACK, NULL, 0, message, conn_id, server_seq, ++seq);
    
    // send
    int32_t bytes = send(socket_fd, message, HEADER_SIZE, 0);
    if(bytes <= 0) {
        perror("sendto");
        return EXIT_SOCKET;
    }

    // update timeout
    struct timespec last_timeout;
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

    // wait until empty ack
    ExitCode exit = EXIT_SUCCESS;
    while(1) {
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) {
            fprintf(stderr, "Maximum timeout time has passed\n");
            if(exit) return exit;
        }

        // resend if no response
        exit = resolve_timeout(resend_timeout, RESEND_TIMEOUT);
        if(exit) {
            // resend
            int32_t bytes = send(socket_fd, message, HEADER_SIZE, 0);
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
        int32_t received = recv(socket_fd, message, MAX_PROTOCOL_SIZE, MSG_DONTWAIT);
        if(received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }
        
        // check malformed packet
        if(check_malformed((unsigned char*)message, received) != EXIT_SUCCESS) continue;
    
        ProtocolHeaderPtr header = (ProtocolHeaderPtr) message;
        if(header->conn_id != conn_id) continue;
        if(header->flags == FLAG_ACK && header->ack_num == server_seq + 1) break;
    }
    // sucessfull teardown
    return EXIT_SUCCESS;
}
