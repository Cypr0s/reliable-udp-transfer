/** ------------- Reliable UDP Transfer ---------------
 * @file    rdt.c
 * @author  Kristian Luptak (xluptak00)
 * @date    26.4.2026
 * @brief   Main file for the RDT
 */

#include "rdt.h"

// https://en.wikipedia.org/wiki/C_signal_handling (taken from project 1)
volatile sig_atomic_t status = 0;

// SIGINT/ SIGTERM handling function
static void catch_function(int signo) {
    status = signo;
}



/**
 * @brief   entry of program
 */
int main(int argc, char** argv) {

    // https://en.wikipedia.org/wiki/C_signal_handling setup handling signals (taken from project 1)
    if (signal(SIGINT, catch_function) == SIG_ERR) {
        fprintf(stderr, "An error occurred while setting a signal handler.\n");
        return EXIT_FAILURE;
    }

    if (signal(SIGTERM, catch_function) == SIG_ERR) {
        fprintf(stderr, "An error occurred while setting a signal handler.\n");
        return EXIT_FAILURE;
    }

    
    // parse args
    Args args = {0};
    ExitCode exit = EXIT_SUCCESS;
    exit = parse_arguments(argc, argv, &args);
    if(exit == EXIT_HELP) {
        return EXIT_SUCCESS;
    }

    // other err
    if(exit) return exit;
    
    // resolve address / ip
    struct addrinfo* addresses;
    exit = resolve_address(args.address, &addresses, args.port, args.app_side);
    if(exit) return exit;

    // create socket
    int32_t socket_fd = create_socket(addresses);
    if(socket_fd == -1) {
        freeaddrinfo(addresses);
        return EXIT_SOCKET;
    }
    // actually run app for server or client
    if(args.app_side == SERVER) {
        exit = run_as_server(socket_fd, addresses, &args);
    }
    else if(args.app_side == CLIENT) {
        exit = run_as_client(socket_fd, addresses, &args);
    }

    freeaddrinfo(addresses);
    close(socket_fd);
    if(exit == EXIT_SIGNAL) return EXIT_SUCCESS;
    return exit;
}