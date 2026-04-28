
#include "error.h"
#include "util.h"   // AppSideEnum, open_file
#include "CLI_parse.h" // ArgsPtr
#include "socket.h" // socket related
#include "error.h"  // errors
#include "protocol.h"

ExitCode run_as_client(int32_t socket_fd, struct addrinfo* address, ArgsPtr args);

ExitCode client_handle_handshake(int32_t socket_fd, 
                                uint32_t max_timeout, 
                                uint32_t* seq,
                                uint32_t conn_id
                            );