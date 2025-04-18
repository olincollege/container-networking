#pragma once

#include <sys/types.h>

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

/**
 * Pivot the root filesystem using the pivot_root syscall.
 *
 * @param new_root Path to the new root.
 * @param put_old Path to directory to place the old root.
 * @return 0 on success, or -1 on failure.
 */
int pivot_root(const char* new_root, const char* put_old);
