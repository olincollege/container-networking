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

struct child_config {
  int argc;
  uid_t uid;
  int fd;
  char* hostname;
  char** argv;
  char* mount_dir;
};

void kill_and_finish_child(pid_t child_pid);

int finish_child(pid_t child_pid);

int clear_resources();

void usage();

void cleanup();

void error_and_exit();
