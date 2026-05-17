/** ------------- Reliable UDP Transfer ---------------
 * @headerfile  client.h
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Contains declaration of functions which program runs as client
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "error.h"
#include "util.h"   // AppSideEnum, open_file
#include "CLI_parse.h" // ArgsPtr
#include "socket.h" // socket related
#include "error.h"  // errors
#include "protocol.h"   // header,  ...
#include "util.h"   // flagsenum
#include <signal.h> // singal

extern volatile sig_atomic_t status; // signal

#define ITEM_ACK 2
#define ITEM_FULL 1

// window item
typedef struct {
    unsigned char data[MAX_PROTOCOL_SIZE];
    uint16_t data_size;
    unsigned char flags; // ack and empty
    struct timespec sent_time;
} ClientItem;

// sliding window
typedef struct {
    ClientItem items[WINDOW_SIZE];
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