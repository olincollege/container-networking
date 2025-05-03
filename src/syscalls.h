#pragma once

#include "utils.h"

/**
 * Drop bounding and inheritable capabilities.
 *
 * @return 0 on success, 1 on failure.
 */
int capabilities(void);

/**
 * Apply seccomp syscall filters.
 *
 * @return 0 on success, 1 on failure.
 */
int syscalls(void);

