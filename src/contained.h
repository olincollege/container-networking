#pragma once

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <linux/capability.h>
#include <linux/limits.h>
#include <pwd.h>
#include <sched.h>
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "cgroups.h"
#include "syscalls.h"
#include "utils.h"

#define MAJOR_VERSION 6
#define MINOR_VERSION 8

void cleanup(int status, void* arg);

void finish_child(int child_pid, int* err);

void clear_resources(struct child_config* config, char* stack);

void usage(char* path);

int main(int argc, char** argv);
