/** ------------- IPK 2 - RDT ---------------
 * @file        util.c
 * @author      Kristian Luptak (xluptak00)
 * @date        28.4.2026
 * @brief       Implements utility functions used throughout program
 */

#include "util.h"

/**
 * @brief           opens a file with open function, returns file descriptor or -1 on error
 * @param path      file path opens file, NULL or - returns default values based on app_side
 * @param app_side  SERVER or CLIENT
 * @return          file descriptor or -1 on error
 */
int32_t open_file(const char* path, AppSideEnum app_side) {
    if(path == NULL || !strcmp(path, "-")) {
        if(app_side == SERVER) {
            return STDOUT_FILENO; // stdout for server
        }
        return STDIN_FILENO; // stdin for client
    }

    // Client : read
    // Server : write and/or create, rewrite
    uint32_t flag = app_side == CLIENT ? O_RDONLY : (O_WRONLY | O_TRUNC | O_CREAT);
    // open or creade -rw-rw-r--
    int32_t fd = open(path, flag, 0664);
    if(fd == -1) {
        perror("open");
        return -1;
    }
    return fd;
} // open_file


/**
 * @brief                   check if time from last_sent into present exceeds max_timout_ms
 * @param last_sent         timespec struct of last timestamp
 * @param max_timeout_mss   maximum timeout in milliseconds
 * @return EXIT_SUCCESS     timeout didnt exceed
 *         EXIT_TIMEOUT     timeout passed
 *         EXIT_CLOCK       clock_gettime error
 */
ExitCode resolve_timeout(struct timespec last_sent, uint32_t max_timeout_ms) {
    struct timespec curr_time;
    if(clock_gettime(CLOCK_MONOTONIC, &curr_time) != 0) {
        perror("clock_gettime");
        return EXIT_CLOCK;
    }
    if(((curr_time.tv_sec - last_sent.tv_sec) * S_TO_MS) >= max_timeout_ms) {
        return EXIT_TIMEOUT;
    }
    return EXIT_SUCCESS;
} // resolve_timeout