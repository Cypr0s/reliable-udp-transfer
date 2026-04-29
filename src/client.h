#ifndef CLIENT_H
#define CLIENT_H

#include "error.h"
#include "util.h"   // AppSideEnum, open_file
#include "CLI_parse.h" // ArgsPtr
#include "socket.h" // socket related
#include "error.h"  // errors
#include "protocol.h"   // header,  ...
#include "util.h"   // flagsenum

#define ITEM_ACK 1
#define ITEM_FULL 2

typedef struct {
    unsigned char header[MAX_PROTOCOL_SIZE];
    uint16_t header_size;
    unsigned char flags; // ack and empty
    struct timespec sent_time;
} Item, *ItemPtr;

typedef struct {
    Item items[WINDOW_SIZE];
    uint16_t filled_slots;
} Window;

ExitCode run_as_client(int32_t socket_fd, struct addrinfo* address, ArgsPtr args);

ExitCode handle_connection(int32_t socket_fd, 
                                uint32_t max_timeout, 
                                uint32_t* seq,
                                uint32_t conn_id,
                                FlagsEnum H_F_flag
                            );

ExitCode send_data(int32_t socket_fd, 
                        int32_t in_file,
                        uint32_t max_timeout, 
                        uint32_t* seq,
                        uint32_t conn_id
                    );

#endif // CLIENT_H