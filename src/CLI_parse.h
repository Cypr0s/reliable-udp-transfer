/** ------------- IPK 2 - RDT ---------------
 * @headerfile  CLI_parse.h
 * @author      Kristian Luptak (xluptak00)
 * @date        26.4.2026
 * @brief       CLI argument parsing module
 */

#ifndef CLIPARSE_H
#define CLIPARSE_H

#include "error.h"  // error codes
#include <stdio.h>  // fprintf
#include <stdint.h>  // types
#include <string.h> // strcmp
#include <limits.h> // type limits
#include <stdlib.h> // strtol <- defined also in error.h
#include "util.h"   // AppSideEnum, 


ExitCode parse_arguments(int argc, char** argv, ArgsPtr args);

ExitCode check_arg(void* arg, int32_t argc_count, uint32_t i, uint8_t arg_size, char* argument);

int64_t convert_str_to_long(const char* str, int64_t max_val);

#endif // CLIPARSE_H