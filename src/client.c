#include "client.h"

ExitCode run_as_client(int32_t socket_fd, struct addrinfo* address, ArgsPtr args) {
    int32_t in_fd = open_file(args->file_in, args->app_side);
    ExitCode exit = EXIT_SUCCESS;
    if(in_fd == -1) return EXIT_OPEN;

    // connect socket
    if(connect(socket_fd, address->ai_addr, address->ai_addrlen)){
        perror("connect");
        if(in_fd != STDERR_FILENO && in_fd != STDIN_FILENO && in_fd != STDOUT_FILENO) {
            close(in_fd);
        }
        return EXIT_SOCKET;
    }

    // set resending timer
    if(set_receive_timeout(socket_fd, RESEND_TIMEOUT)) {
        if(in_fd != STDERR_FILENO && in_fd != STDIN_FILENO && in_fd != STDOUT_FILENO) {
            close(in_fd);
        }
        return EXIT_SOCKET;
    }

    uint32_t conn_id, seq;
    conn_id = (uint32_t) rand();
    seq = (uint32_t) rand();

    exit = handle_connection(socket_fd, args->timeout_sec, &seq, conn_id, FLAG_SYN);
    if(exit) {
        if(in_fd != STDERR_FILENO && in_fd != STDIN_FILENO && in_fd != STDOUT_FILENO) {
            close(in_fd);
        }
        return exit;
    }

    exit = send_data(socket_fd, in_fd, args->timeout_sec, &seq, conn_id);
    if(exit) {
        if(in_fd != STDERR_FILENO && in_fd != STDIN_FILENO && in_fd != STDOUT_FILENO) {
            close(in_fd);
        }
        return exit;
    }

    exit = handle_connection(socket_fd, args->timeout_sec, &seq, conn_id, FLAG_FIN);
    
    if(in_fd != STDERR_FILENO && in_fd != STDIN_FILENO && in_fd != STDOUT_FILENO) {
        close(in_fd);
    }
    return exit;
}

ExitCode handle_connection(int32_t socket_fd, 
                                uint32_t max_timeout, 
                                uint32_t* seq,
                                uint32_t conn_id,
                                FlagsEnum H_F_flag
                            ) {
    // create first packet SYN / FIN

    char message[MAX_PROTOCOL_SIZE];
    
    create_header(H_F_flag, NULL, 0, message, conn_id, (*seq)++, 0);

    // start timeout
    struct timespec last_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    ExitCode exit = EXIT_SUCCESS;
    while(1) {
        int32_t bytes = send(socket_fd, message, HEADER_SIZE, 0);
        if(bytes <= 0) {
            perror("sendto");
            return EXIT_SOCKET;
        }

        int32_t received = recv(socket_fd, message, MAX_PROTOCOL_SIZE, 0);
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) {
            fprintf(stderr, "Maximum timeout time has passed\n");
            return exit;
        }

        if(received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }
        if(check_malformed((unsigned char*)message, received) != EXIT_SUCCESS) continue;

        // check for valid packet
        ProtocolHeaderPtr header = (ProtocolHeaderPtr) message;
        if(header->conn_id != conn_id) continue;
        if(header->flags != (FLAG_ACK | H_F_flag)) continue;
        if(header->ack_num == *seq) break;
    }

    // send ack back
    ProtocolHeaderPtr header = (ProtocolHeaderPtr) message;
    create_header(FLAG_ACK, NULL, 0, message, conn_id, 0, header->seq_num + 1);
    // send
    int32_t bytes = send(socket_fd, message, HEADER_SIZE, 0);
    if(bytes <= 0) {
        perror("sendto");
        return EXIT_SOCKET;
    }
    (*seq)++;
    return EXIT_SUCCESS;
}


ExitCode send_data(int32_t socket_fd, 
                        int32_t in_file,
                        uint32_t max_timeout, 
                        uint32_t* seq,
                        uint32_t conn_id
                    ) {
    // create window
    Window window = {0};
    uint8_t window_start = 0;
    ExitCode exit = EXIT_SUCCESS;

    // for data reading
    char data[MAX_DATA_SIZE];
    int32_t data_size;

    // message sending
    char message[MAX_PROTOCOL_SIZE];
    unsigned char eof = 0;

    // global timeout
    struct timespec last_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    // infinite loop
    while(!eof || window.filled_slots > 0) {
        // send messages
        while(!eof && window.filled_slots < WINDOW_SIZE) {
            // read data
            data_size = read(in_file, data, MAX_DATA_SIZE);
            if(data_size == 0) {
                eof = 1;
                break;
            }
            else if(data_size < 0) {
                return EXIT_READ;
            }
            // create header
            create_header(FLAG_DATA, data, data_size, message, conn_id, *seq, 0);
            int32_t bytes = send(socket_fd, message, HEADER_SIZE + data_size, 0);
            if(bytes <= 0) {
                perror("sendto");
                return EXIT_SOCKET;
            }
            // add entry to window
            uint16_t index = *seq % WINDOW_SIZE;
            if(clock_gettime(CLOCK_MONOTONIC, &window.items[index].sent_time) != 0) {
                perror("clock_gettime");
                return EXIT_CLOCK;
            }
            memcpy(window.items[index].header, message, bytes);
            window.items[index].flags |= ITEM_FULL;
            window.items[index].header_size = bytes;
            window.filled_slots++;
            (*seq)++;
        }
        // handle responses
        while(1) {
            int32_t received = recv(socket_fd, message, MAX_PROTOCOL_SIZE, 0);
            // no responses found
            if(received <= 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                return EXIT_SOCKET;
            }

            if(check_malformed((unsigned char*)message, received) != EXIT_SUCCESS) continue;

            ProtocolHeaderPtr header = (ProtocolHeaderPtr) message;
            // check for correct packet
            if(header->conn_id != conn_id || header->flags != FLAG_ACK) continue;
            // check whether its in window
            for(uint16_t i = 0; i < WINDOW_SIZE; i++) {
                if(!(window.items[i].flags & ITEM_FULL)) {
                    continue;
                }
                if(((ProtocolHeaderPtr)window.items[i].header)->seq_num + 1 == header->ack_num) {
                    // set acked
                    window.items[i].flags |= ITEM_ACK;
                    // reset timeout
                    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
                        perror("clock_gettime");
                        return EXIT_CLOCK;
                    }
                }
            }
        }

        // handle retransmits
        for(uint16_t i = 0; i < WINDOW_SIZE; i++) {
            if(!(window.items[i].flags & ITEM_FULL)) {
                    continue;
            }
            if(resolve_timeout(window.items[i].sent_time, RESEND_TIMEOUT) && !(window.items[i].flags & ITEM_ACK)) {
                // resend
                int32_t bytes = send(socket_fd, window.items[i].header, window.items[i].header_size, 0);
                if(bytes <= 0) {
                    perror("sendto");
                    return EXIT_SOCKET;
                }
                // reset timer
                if(clock_gettime(CLOCK_MONOTONIC, &window.items[i].sent_time) != 0) {
                    perror("clock_gettime");
                    return EXIT_CLOCK;
                }
            }
        }

        // check timeout
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) {
            fprintf(stderr, "Maximum timeout time has passed\n");
            return exit;
        }
        
        // move window
        for(uint32_t i = 0; i < WINDOW_SIZE; i++) {
            uint32_t index = (i + window_start) % WINDOW_SIZE;
            // check if acked
            if(!(window.items[index].flags & ITEM_ACK)) {
                break;
            }
            
            // reset item
            window.items[index].flags = 0;

            // slide window
            window.filled_slots--;
            window_start =(window_start + 1) % WINDOW_SIZE;
        }
    }
    return EXIT_SUCCESS;
}