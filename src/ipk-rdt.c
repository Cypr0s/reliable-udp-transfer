#include "ipk-rdt.h"

ExitCode main(int argc, char** argv) {
    Args args = {0};
    // parse args
    ExitCode parse_result = parse_arguments(argc, argv, &args);
    if(parse_result == EXIT_HELP) {
        return EXIT_SUCCESS;
    }

    if(parse_result != EXIT_SUCCESS) {
        return parse_result;
    }

    // resolve address / ip

    // create socket

    // actually run app for server or client

    return EXIT_SUCCESS;
}