#include <stdlib.h>

struct child_config {
    int argc;
    uid_t uid;
    int fd;
    char* hostname;
    char** argv;
    char* mount_dir;
};

/**
 * Print error message and exit.
 *
 * @param error_msg The message to display before exiting.
 */
void error_and_exit(const char* error_msg);
