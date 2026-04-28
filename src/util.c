
#include "util.h"

int32_t open_file(const char* path, AppSideEnum app_side) {
    if(path == NULL || !strcmp(path, "-")) {
        if(app_side == SERVER) {
            return STDOUT_FILENO; // stdout for server
        }
        return STDIN_FILENO; // stdin for client
    }

    // r or w per side
    uint32_t flag = app_side == CLIENT ? O_RDONLY : (O_WRONLY | O_TRUNC | O_CREAT);
    // open or creade -rw-rw-r--
    int32_t fd = open(path, flag, 0664);
    if(fd == -1) {
        perror("open");
        return -1;
    }
    return fd;
} // open_file


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