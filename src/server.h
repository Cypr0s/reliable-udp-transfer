/** ------------- IPK 2 - RDT ---------------
 * @headerfile  server.h
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Contains declaration of functions which program runs as server
 */

#ifndef SERVER_H
#define SERVER_H

#include "error.h"  // error codes
#include "util.h"   // AppSideEnum, open_file
#include "CLI_parse.h" // ArgsPtr
#include "socket.h" // socket related
#include "protocol.h"   // protocol
#include <signal.h> // singal

extern volatile sig_atomic_t status; // signal


#define ITEM_FULL 1

typedef struct {
    unsigned char data[MAX_PROTOCOL_SIZE];
    uint16_t data_size;
    unsigned char flags;  // item full
} ServerItem;



ExitCode run_as_server(int32_t socket_fd, struct addrinfo* address, ArgsPtr args);

ExitCode server_handshake(int32_t socket_fd, 
                                uint32_t max_timeout, 
                                uint32_t* expected_seq,
                                uint32_t* conn_id
                            );

ExitCode receive_data(int32_t socket_fd, 
                        int32_t out_file,
                        uint32_t max_timeout, 
                        uint32_t* seq,
                        uint32_t conn_id
                    );

ExitCode server_teardown(int32_t socket_fd, 
                         uint32_t max_timeout, 
                         uint32_t expected_seq,
                         uint32_t conn_id
                        );

#endif // SERVER_H