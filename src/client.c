#include "client.h"

#define RESEND_TIMEOUT 256

ExitCode run_as_client(int32_t socket_fd, struct addrinfo* address, ArgsPtr args) {
    int32_t in_fd = open(args->file_in, args->app_side);
    ExitCode exit = EXIT_SUCCESS;
    if(in_fd == -1) return EXIT_OPEN;

    if(connect(socket_fd, address->ai_addr, address->ai_addrlen)){
        perror("connect");
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


    // send
    int32_t bytes = send(socket_fd, msg, HEADER_SIZE, 0);
    if(bytes <= 0) {
        perror("sendto");
        return EXIT_SOCKET;
    }

    // start timeout
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
    ExitCode exit = EXIT_SUCCESS;
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

        int32_t received = recv(socket_fd, message, MAX_PROTOCOL_SIZE, MSG_DONTWAIT);
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