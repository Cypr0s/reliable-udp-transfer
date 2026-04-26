#ifndef ERROR_H
#define ERROR_H

#include <errno.h>  // errno
#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE

typedef enum ExitEnum {
    // EXIT_SUCCESS = 0, defined in stdlib.h
    // EXIT_FAILURE = 1, defined in stdlib.h
    EXIT_PARSE = 2,
    EXIT_HELP = 3, // defined as non zero but its NOT an error, code will exit with 0
    EXIT_GETADDRINFO = 4,
    EXIT_GETIFADDRS = 5,
    EXIT_MALLOC = 99,
} ExitCode;

#endif // ERROR_H