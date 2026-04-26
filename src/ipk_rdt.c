/** ------------- IPK 2 - RDT ---------------
 * @file    ipk_rdt.c
 * @author  Kristian Luptak (xluptak00)
 * @date    26.4.2026
 * @brief   Main file for the RDT
 */

#include "ipk_rdt.h"

int main(int argc, char** argv) {
    Args args = {0};
    ExitCode exit = EXIT_SUCCESS;
    // parse args
    ExitCode parse_result = parse_arguments(argc, argv, &args);
    if(parse_result == EXIT_HELP) {
        return EXIT_SUCCESS;
    }

    if(parse_result != EXIT_SUCCESS) {
        return parse_result;
    }

    // resolve address / ip
    struct addrinfo* addresses;
    exit = resolve_address(args.address, &addresses, args.port, args.app_side);
    if(exit != EXIT_SUCCESS) {
        return exit;
    }

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
    return EXIT_SUCCESS;
}