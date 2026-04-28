#include "client.h"

ExitCode run_as_client(int32_t socket_fd, struct addrinfo* address, ArgsPtr args) {
    int32_t in_fd = open(args->file_in, args->app_side);
    ExitCode exit = EXIT_SUCCESS;
    if(in_fd == -1) return EXIT_OPEN;

    // connect socket
    if(connect(socket_fd, address->ai_addr, address->ai_addrlen)){
        perror("connect");
        return EXIT_SOCKET;
    }

    // set resending timer
    if(set_receive_timeout(socket_fd, RESEND_TIMEOUT)) {
        return EXIT_SOCKET;
    }

    uint32_t conn_id, start_seq;
    conn_id = (uint32_t) rand();
    start_seq = (uint32_t) rand();

    exit = client_handle_handshake(socket_fd, args->timeout_sec, &start_seq, conn_id);

}

ExitCode client_handle_handshake(int32_t socket_fd, 
                                uint32_t max_timeout, 
                                uint32_t* seq,
                                uint32_t conn_id
                            ) {
    

    // create first SYN packet
    char message[MAX_PROTOCOL_SIZE];
    char* msg = message;
    create_header(SYN, NULL, 0, &msg, conn_id, (*seq)++, 0);

    // start timeout
    struct timespec last_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }

    ExitCode exit = EXIT_SUCCESS;
    while(1) {
        int32_t bytes = send(socket_fd, msg, HEADER_SIZE, 0);
        if(bytes <= 0) {
            perror("sendto");
            return EXIT_SOCKET;
        }

        int32_t received = recv(socket_fd, message, MAX_PROTOCOL_SIZE, 0);
        exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
        if(exit) return exit;

        if(received <= 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return EXIT_SOCKET;
        }

        if(check_malformed((unsigned char*)message, received, ACK | SYN, conn_id) != EXIT_SUCCESS) continue;

        // check for correct ack num
        int32_t message_ack_num = ((ProtocolHeaderPtr) message)->ack_num;
        if(message_ack_num == *seq) break;
    }

    // send ack back
    ProtocolHeaderPtr header = (ProtocolHeaderPtr) message;
    create_header(ACK, NULL, 0, &msg, conn_id, (*seq)++, header->seq_num + 1);
    // send
    int32_t bytes = send(socket_fd, msg, HEADER_SIZE, 0);
    if(bytes <= 0) {
        perror("sendto");
        return EXIT_SOCKET;
    }

    return EXIT_SUCCESS;
}


ExitCode send_data(int32_t socket_fd, 
                        int32_t in_file,
                        uint32_t max_timeout, 
                        uint32_t* seq,
                        uint32_t* conn_id
                    ) {
    char data[MAX_DATA_SIZE];
    int32_t data_size;
    ExitCode exit = EXIT_SUCCESS;

    // read untill empty
    while((data_size = read(in_file, data, MAX_DATA_SIZE)) > 0) {
        // start timeout
        struct timespec last_timeout;
        if(clock_gettime(CLOCK_MONOTONIC, &last_timeout) != 0) {
            perror("clock_gettime");
            return EXIT_CLOCK;
        }

        // send message with data
        char message[MAX_PROTOCOL_SIZE];
        char* msg = message;
        create_header(DATA, data, data_size, &msg, conn_id, *seq, 0);
        // send msg, wait for response
        while(1) {
            int32_t bytes = send(socket_fd, msg, HEADER_SIZE + data_size, 0);
            if(bytes <= 0) {
                perror("sendto");
                return EXIT_SOCKET;
            }

            int32_t received = recv(socket_fd, message, MAX_PROTOCOL_SIZE, 0);
            exit = resolve_timeout(last_timeout, max_timeout * S_TO_MS);
            if(exit) return exit;

            if(received <= 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
                return EXIT_SOCKET;
            }

            if(check_malformed((unsigned char*)message, received, ACK, conn_id) != EXIT_SUCCESS) continue;

            // check for correct ack num
            int32_t message_ack_num = ((ProtocolHeaderPtr) message)->ack_num;
            if(message_ack_num == *seq + 1) break;
        }
        // correctly sent message
        (*seq)++;
    }

    // read error
    if(data_size == -1) {
        perror("read");
        return EXIT_READ;
    }
    return EXIT_SUCCESS;
}
