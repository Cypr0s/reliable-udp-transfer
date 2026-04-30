/** ------------- IPK 2 - RDT ---------------
 * @file        server.c
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Contains Functions which program runs as server, establishing connection, 
 *              secure data transfer using selective repeat
 */


#include "server.h"

/**
 * @brief           runs the server: binds socket, performs handshake, 
 *                  receives data and does teardown
 * @param socket_fd file descriptor of the socket
 * @param address   address info to bind to
 * @param args      parsed CLI args
 * @return          EXIT_SUCCESS on success
 *                  different error codes on error
 */
ExitCode run_as_server(int32_t socket_fd, struct addrinfo* address, ArgsPtr args) {
    ExitCode exit = EXIT_SUCCESS;

    // open out file
    int32_t out_fd = open_file(args->file_out, args->app_side);
    if(out_fd == -1) return EXIT_OPEN;

    // bind socket
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
} // run_as_server

/**
 * @brief  performs three way handshake with client,
 *         firstly waits for SYN, after getting it connects to address and sends SYN + ACK back
 *         and waits for ACK or DATA <- DATA means packet loss
 * @return EXIT_SUCCESS on success, 
 *         different error codes on error
 */
ExitCode server_handshake(int32_t socket_fd, 
                          uint32_t max_timeout, 
                          uint32_t* expected_seq,
                          uint32_t* conn_id
                        ) {
    ExitCode exit = EXIT_SUCCESS;

    // buffer for holding incoming packets
    unsigned char buffer[MAX_PROTOCOL_SIZE];

    // sender addr
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    // start timeout timer
    struct timespec last_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // wait for empty SYN until timeout
    while(1) {
        // timeout checker
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) {
            fprintf(stderr, "Maximum timeout time has passed\n");
            return exit;
        }

        // receive data
        int32_t received = recvfrom(socket_fd, 
                                    buffer, 
                                    MAX_PROTOCOL_SIZE, 
                                    MSG_DONTWAIT, 
                                    (struct sockaddr *)&client_addr, 
                                    &client_addr_size
                                );
        // no data on socket
        if (received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }

        // check whether message is invalid
        if(check_malformed((unsigned char*)buffer, received) != EXIT_SUCCESS) continue;
        if(((ProtocolHeaderPtr) buffer)->flags == FLAG_RST) return EXIT_SIGNAL;
        if(((ProtocolHeaderPtr) buffer)->flags != FLAG_SYN) continue;

        // valid SYN message break from loop
        break;
    }

    // set conn id
    *conn_id = ((ProtocolHeaderPtr) buffer)->conn_id;

    // connect to address
    if(connect(socket_fd, (struct sockaddr*) &client_addr, client_addr_size) == -1) {
        perror("connect");
        return EXIT_SOCKET;
    }

    // set next expected seq
    *expected_seq = ((ProtocolHeaderPtr) buffer)->seq_num;

    // create SYN + ACK and send back
    unsigned char message_back[HEADER_SIZE];
    uint32_t server_seq = rand();
    create_header(FLAG_SYN | FLAG_ACK, NULL, 0, message_back, *conn_id, server_seq, ++(*expected_seq));
    
    // send
    int32_t bytes = send(socket_fd, message_back, HEADER_SIZE, 0);
    if(bytes <= 0) {
        perror("sendto");
        return EXIT_SOCKET;
    }

    // update global timeout
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // start resending timeout
    struct timespec resend_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &resend_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // wait for empty ack, or retransmit on retransmit timeout
    while(1) {
        // SIGINT/SIGTERM
        if(status) {
            create_header(FLAG_RST, NULL, 0, message_back, *conn_id, 0, 0);
            int32_t bytes = send(socket_fd, message_back, HEADER_SIZE, 0);
            if(bytes <= 0) {
                perror("sendto");
                return EXIT_SOCKET;
            }
            return EXIT_SIGNAL;
        }
        // global timeout check
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit){
            fprintf(stderr, "Maximum timeout time has passed\n"); 
            return exit;
        }

        // response timeout check
        exit = resolve_timeout(resend_timeout, RESEND_TIMEOUT);
        if(exit == EXIT_CLOCK) return EXIT_CLOCK;

        if(exit) {
            // resend
            int32_t bytes = send(socket_fd, message_back, HEADER_SIZE, 0);
            if(bytes <= 0) {
                perror("sendto");
                return EXIT_SOCKET;
            }
            // reset retransmit timer
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
    
        // check whether message is invalid
        if(check_malformed((unsigned char*)buffer, received) != EXIT_SUCCESS) continue;
        ProtocolHeaderPtr header = (ProtocolHeaderPtr) buffer;
        // check connection id
        if(header->conn_id != *conn_id) continue;
        if(header->flags == FLAG_RST) return EXIT_SIGNAL;

        // ACK message is from handshake, DATA message could come if handshake ACK got lost
        if(header->flags != FLAG_ACK && header->flags != FLAG_DATA) continue; 

        // ack
        if(header->ack_num == server_seq + 1) break;
        // data, client moved on
        if(header->seq_num == *expected_seq + 1) break;
    }

    // sucessfull handshake
    (*expected_seq)++;
    return EXIT_SUCCESS;
} // server handshake


/**
 * @brief receives data from the client using a selective repeat, 
 *        inspired by: https://en.wikipedia.org/wiki/C_signal_handling
 * 
 */
ExitCode receive_data(int32_t socket_fd, 
                        int32_t out_file,
                        uint32_t max_timeout, 
                        uint32_t* seq,
                        uint32_t conn_id
                    ) {
    // window/ buffer for holding messages
    ServerItem window[WINDOW_SIZE];
    // start seq
    uint32_t expected_seq = *seq;

    // buffer for receiving messages
    ExitCode exit = EXIT_SUCCESS;
    unsigned char received_message[MAX_PROTOCOL_SIZE];

    // start timeout
    struct timespec last_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // infinite loop 
    while(1) {
        // SIGINT/SIGTERM
        if(status) {
            create_header(FLAG_RST, NULL, 0, received_message, conn_id, 0, 0);
            int32_t bytes = send(socket_fd, received_message, HEADER_SIZE, 0);
            if(bytes <= 0) {
                perror("sendto");
                return EXIT_SOCKET;
            }
            return EXIT_SIGNAL;
        }
        // check timeout
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) {
            fprintf(stderr, "Maximum timeout time has passed\n");
            return exit;
        }

        // receive message
        int32_t received = recv(socket_fd, received_message, MAX_PROTOCOL_SIZE, MSG_DONTWAIT);
        if(received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }

        // validate packet
        if(check_malformed((unsigned char*)received_message, received) != EXIT_SUCCESS) continue;

        ProtocolHeaderPtr header = (ProtocolHeaderPtr) received_message;
        if(header->conn_id != conn_id) continue;

        // check for data flag
        if(header->flags == FLAG_DATA) {
            // doesnt fit into window
            if(header->seq_num >= expected_seq + WINDOW_SIZE) continue;
            // reset timeout valid seq packet
            if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
                perror("clock_gettime");
                return EXIT_CLOCK;
            }

            // send ACK back
            unsigned char ack_msg[HEADER_SIZE];
            create_header(FLAG_ACK, NULL, 0, ack_msg, conn_id, 0, header->seq_num + 1);
            int32_t bytes = send(socket_fd, ack_msg, HEADER_SIZE, 0);
            if(bytes <= 0) {
                perror("send");
                return EXIT_SOCKET;
            }

            // old packet
            if(header->seq_num < expected_seq) continue;

            // new hdr, store into window, reset timer
            if(!(window[header->seq_num % WINDOW_SIZE].flags & ITEM_FULL)) {
                memcpy(window[header->seq_num % WINDOW_SIZE].data, received_message, received);
                window[header->seq_num % WINDOW_SIZE].data_size = received;
                window[header->seq_num % WINDOW_SIZE].flags = ITEM_FULL;
            }


            // write values, move window
            while(window[expected_seq % WINDOW_SIZE].flags & ITEM_FULL) {
                ProtocolHeaderPtr hdr = (ProtocolHeaderPtr) (window[expected_seq % WINDOW_SIZE].data);
                // write, reset
                if(write(out_file, hdr->data, hdr->payload_size) == -1) {
                    perror("write");
                    return EXIT_WRITE;
                }
                window[expected_seq % WINDOW_SIZE].flags = 0;
                expected_seq++;

                // reset timeout on progress
                if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
                    perror("clock_gettime");
                    return EXIT_CLOCK;
                }
            }
        }
        else if(header->flags == FLAG_FIN) {
            // DATA transferred
            break;
        }
        else if(header->flags == FLAG_RST) {
            // SIGINT / SIGTERM from client
            return EXIT_SIGNAL;
        }
        else {
            continue;
        }

    }
    *seq = expected_seq;
    return EXIT_SUCCESS;
} // receive_data


/**
 * @brief   Performs the teardown with the client, FIN->FIN,ACK->ACK
 * @return  EXIT success on success
 *          different errors on error
 * 
 */
ExitCode server_teardown(int32_t socket_fd, 
                         uint32_t max_timeout, 
                         uint32_t seq,
                         uint32_t conn_id
                        ) {
    // send FIN+ACK
    unsigned char message[MAX_PROTOCOL_SIZE];
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

    // start retransmit timeout
    struct timespec resend_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &resend_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // wait until empty ack
    ExitCode exit = EXIT_SUCCESS;
    while(1) {
        // SIGINT/SIGTERM
        if(status) {
            create_header(FLAG_RST, NULL, 0, message, conn_id, 0, 0);
            int32_t bytes = send(socket_fd, message, HEADER_SIZE, 0);
            if(bytes <= 0) {
                perror("sendto");
                return EXIT_SOCKET;
            }
            return EXIT_SIGNAL;
        }
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) {
            // maximum time passes but client finished sucessfully, (packet loss)
            return EXIT_SUCCESS;
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
        
        // check malformed message
        if(check_malformed((unsigned char*)message, received) != EXIT_SUCCESS) continue;
    
        ProtocolHeaderPtr header = (ProtocolHeaderPtr) message;
        if(header->conn_id != conn_id) continue;
        if(header->flags == FLAG_RST) return EXIT_SIGNAL;
        if(header->flags == FLAG_ACK && header->ack_num == server_seq + 1) break;
    }
    // sucessfull teardown
    return EXIT_SUCCESS;
} // server teardown
