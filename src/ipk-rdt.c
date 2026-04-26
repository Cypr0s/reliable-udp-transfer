#include "ipk-rdt.h"

#define CHECK_ERR(exitcode) do {
    if(exit != EXIT_SUCCESS) {
        if(addresses != NULL) {
            freeaddrinfo(addresses);
        }
        return exit;
    }
} while(0);

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

    // actually run app for server or client
    freeaddrinfo(addresses);
    return EXIT_SUCCESS;
}