#include "utils.h"

void error_and_exit(const char* error_msg) {
    perror(error_msg);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
}
