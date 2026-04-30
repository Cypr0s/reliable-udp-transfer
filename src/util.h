/** ------------- IPK 2 - RDT ---------------
 * @headerfile  util.h
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Contains Utility functions declarations, struct and enum definitions
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h> // types
#include <string.h> // strcmp
#include <stdio.h>  // fprintf, stderr
#include <stdio.h>  // fopen, fileno
#include <unistd.h> // close
#include <fcntl.h>  // open
#include "error.h"  // error codes
#include <time.h>    // struct timespec, CLOCK_MONOTONIC

#define S_TO_MS 1000

#define RESEND_TIMEOUT 50 // miliseconds

#define WINDOW_SIZE 128 

typedef enum {
    NONE = 0,
    SERVER = 1,
    CLIENT = 2,
} AppSideEnum;

// different sent message types
typedef enum {
    FLAG_SYN = 1,
    FLAG_ACK = 2,
    FLAG_FIN = 4,
    FLAG_RST = 8,
    FLAG_DATA = 16,
} FlagsEnum;

// stores parsed arguments
typedef struct {
    AppSideEnum app_side;
    uint16_t port;  // port
    char* address; // ip address / hostname
    char* file_in; // file to read from (client)
    char* file_out; // file to write to (server)
    uint32_t timeout_sec;
} Args, *ArgsPtr;

int32_t open_file(const char* path, AppSideEnum app_side);

ExitCode resolve_timeout(struct timespec last_sent, uint32_t max_timeout_ms);

#endif // UTIL_H