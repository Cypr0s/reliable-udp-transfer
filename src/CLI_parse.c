/** ------------- IPK 2 - RDT ---------------
 * @headerfile  CLI_parse.c
 * @author      Kristian Luptak (xluptak00)
 * @date        26.4.2026
 * @brief       CLI argument parsing module
 */

#include "CLI_parse.h"

#define BASE 10

const char* help_message;

/**
 * @brief       parses arguments into ArgsPtr struct, checks for CLI errors
 * @param argc  number of arguments
 * @param argv  array of argument strings
 * @param args  pointer to ArgsPtr struct
 * @return      EXIT_SUCCESS(0) - success
 *              EXIT_PARSE(2) - parsing error
 *              EXIT_HELP(3) - help - will return 0
 */
ExitCode parse_arguments(int argc, char** argv, ArgsPtr args) {
    if(argc <= 1) {
        fprintf(stderr, "No arguments provided, try using --help\n");
        return EXIT_PARSE;
    }

    
    // loop through arguments
    for(int32_t i = 1; i < argc; i++) {
        char* argument = argv[i];

        if(!strcmp(argument, "-help") || !strcmp(argument, "-h")) {
            // help argument, it has highest priority(if provided, all other arguments are ignored)
            fprintf(stdout, "%s", help_message);
            return EXIT_HELP;
        }
        else if (!strcmp(argument, "-s")) {
            // server side argument
            if (args->app_side != NONE) {
                fprintf(stderr, "Multiple uses of arg `%s`, only one side can be specified\n", 
                    argument
                );
                return EXIT_PARSE;
            }
            args->app_side = SERVER;
        }
        else if (!strcmp(argument, "-c")) {
            // client side argument
            if (args->app_side != NONE) {
                fprintf(stderr, "Multiple uses of arg `%s`, only one side can be specified\n", 
                    argument
                );
                return EXIT_PARSE;
            }
            args->app_side = CLIENT;
        }
        else if (!strcmp(argument, "-p")) {
            // port argument (-p)
            if(check_arg(&args->port, argc, i, sizeof(uint16_t), argument)) {
                return EXIT_PARSE;
            }

            // convert value to int
            int64_t val = convert_str_to_long(argv[i + 1], UINT16_MAX);
            if(val == -1 || val == 0) {
                fprintf(stderr, "Invalid argument, `%s` parameter input value `%s` is invalid\n", 
                    argument, 
                    argv[i + 1]
                );
                return EXIT_PARSE;
            }

            args->port = (uint16_t) val;
            i++;
        }
        
        else if(!strcmp(argument, "-w")) {
            // timeout argument (-w)

            if(check_arg(&args->timeout_sec, argc, i, sizeof(uint32_t), argument)) {
                return EXIT_PARSE;
            }

            // convert value to int
            int64_t val = convert_str_to_long(argv[i + 1], UINT32_MAX);
            if(val == -1) {
                fprintf(stderr, "Invalid argument, `%s` parameter input value `%s` is invalid\n", 
                    argument, 
                    argv[i + 1]
                );
                return EXIT_PARSE;
            }

            args->timeout_sec = val;
            i++;
        } // timeout argument
        else if (!strcmp(argument, "-a")) {
            // address argument (-a)
            if(check_arg(&args->address, argc, i, sizeof(char*), argument)) {
                return EXIT_PARSE;
            }

            args->address = argv[i + 1];
            i++;
        } // address argument
        else if (!strcmp(argument, "-i")) {
            // file in argument (-i)
            if(check_arg(&args->file_in, argc, i, sizeof(char*), argument)) {
                return EXIT_PARSE;
            }

            args->file_in = argv[i + 1];
            i++;
        } // file in argument
        else if (!strcmp(argument, "-o")) {
            // file out argument (-o)
            if(check_arg(&args->file_out, argc, i, sizeof(char*), argument)) {
                return EXIT_PARSE;
            }

            args->file_out = argv[i + 1];
            i++;
        }
        else {
            // error arg
            fprintf(stderr, "Invalid argument `%s`, try using parameter -help\n", argument);
            return EXIT_PARSE;
        } // error arg
    } // argument for loop


    // required arguments checking
    if(args->app_side == NONE) {
        fprintf(stderr, "No side specified, try using parameter -help\n");
        return EXIT_PARSE;
    }

    if(args->app_side == CLIENT && args->address == NULL) {
        fprintf(stderr, "No address specified for client side, try using parameter -help\n");
        return EXIT_PARSE;
    }

    if (args->port == 0) {
        fprintf(stderr, "No port specified, try using parameter -help\n");
        return EXIT_PARSE;
    }

    // set default timeout if its not set
    if(args->timeout_sec == 0) {
        // default scanner timeout time 1 second
        args->timeout_sec = 1;
    }

    return EXIT_SUCCESS;
}   // parse_arguments

/**
 * @brief checks if argument is already set and if theres an actual value after argument
 */
ExitCode check_arg(void* arg, int32_t argc_count, int32_t i, uint8_t arg_size, char* argument) {
    char zero_arr[arg_size];
    memset(zero_arr, 0, arg_size);
    // check if argument is already set
    if (memcmp(arg, zero_arr, arg_size) != 0) {
        fprintf(stderr, "Multiple uses of arg `%s`\n", argument);
        return EXIT_PARSE;
    }

    // check if theres an actual value after argument
    if(i + 1 >= argc_count) {
        fprintf(stderr, "Invalid argument, `%s` parameter needs a value\n", argument);
        return EXIT_PARSE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief converts string to long and checks constraint that is the max val eg port max USHORT_MAX
 * @param str string to convert
 * @param max_val max val of converted string
 * @return converted value or -1 on error
 */
int64_t convert_str_to_long(const char* str, int64_t max_val) {
    char* check;
    int64_t val;
    errno = 0;
    val = strtol(str, &check, BASE);
    if (errno != 0 || *check != '\0' || check == str) {
        return -1;
    }
    if (val > max_val || val <= 0) {
        return -1;
    }
    return val;
} // convert_str_to_long