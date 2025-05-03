#include "utils.h"

// function 6: print error message and exit dont rmb what this is for tbh
void error_and_exit(const char* error_msg) {
    perror(error_msg);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
}
