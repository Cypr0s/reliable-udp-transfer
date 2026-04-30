/** ------------- IPK 2 - RDT ---------------
 * @headerfile  error.h
 * @author      Kristian Luptak (xluptak00)
 * @date        26.4.2026
 * @brief       error functions and error codes
 */

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
    EXIT_SOCKET = 5,
    EXIT_OPEN = 6,
    EXIT_CLOCK = 7,
    EXIT_TIMEOUT = 8,
    EXIT_CORRUPT = 9,
    EXIT_READ = 10,
    EXIT_WRITE = 11,
    EXIT_SIGNAL = 12,
    EXIT_MALLOC = 99,
} ExitCode;

#endif // ERROR_H