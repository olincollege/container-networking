//NOLINTNEXTLINE
#define _GNU_SOURCE

#pragma once

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

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
