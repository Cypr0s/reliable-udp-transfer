#ifndef CLIPARSE_H
#define CLIPARSE_H

#include "error.h"  // error codes
#include <stdio.h>  // fprintf
#include <stdint.h>  // types
#include <string.h> // strcmp
#include <limits.h> // type limits
#include <stdlib.h> // strtol


typedef enum {
    NONE = 0,
    SERVER = 1,
    CLIENT = 2,
} AppSide;

typedef struct {
    AppSide app_side;
    uint16_t port;  // port
    char* address; // ip address / hostname
    char* file_in; // file to read from (client)
    char* file_out; // file to write to (server)
    uint32_t timeout_sec;
} Args, *ArgsPtr;

#endif // CLIPARSE_H