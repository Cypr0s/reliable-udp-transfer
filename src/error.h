#ifndef ERROR_H
#define ERROR_H

#include <errno.h>  // errno

typedef enum ExitEnum {
    EXIT_SUCCESS = 0,
    EXIT_FAILURE = 1,
    EXIT_PARSE = 2,
    EXIT_HELP = 3, // defined as non zero but its NOT an error, code will exit with 0
    EXIT_MALLOC = 99,
} ExitCode;

#endif // ERROR_H