// NOLINTNEXTLINE
#define _GNU_SOURCE
#pragma once

// Standard C headers
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// POSIX headers
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

// Linux-specific headers
#include <linux/capability.h>
#include <linux/limits.h>

// Third-party headers
#include <seccomp.h>
#include <sys/capability.h>


struct child_config {
  int argc;
  uid_t uid;
  int fd;
  char* hostname;
  char** argv;
  char* mount_dir;
};

/**
 * Print an error message and exit the program.
 *
 * This function wraps a call to `perror()` followed by `exit(EXIT_FAILURE)`.
 * It is intended for use when a system call fails and the program cannot
 * recover. The provided message is used as the prefix for the error output,
 * followed by a description of the current `errno`. After printing the error
 * message, the function will terminate the program immediately with a failure
 * status. This function does not return.  It is useful for handling
 * unrecoverable system call failures (e.g., `clone`, `mount`, `execve`) and
 * also helps satisfy linting rules by avoiding duplicated logic.
 *
 * @param error_msg The message to display before exiting.
 */
void error_and_exit(const char* error_msg);
