#include "cgroups.h"

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





struct cgrp_setting add_to_tasks = {.name = "tasks", .value = "0"};

struct cgrp_control* cgrps[] = {
    &(struct cgrp_control){
        .control = "memory",
        .settings =
            (struct cgrp_setting*[]){
                &(struct cgrp_setting){.name = "memory.limit_in_bytes",
                                       .value = MEMORY},
                &(struct cgrp_setting){.name = "memory.kmem.limit_in_bytes",
                                       .value = MEMORY},
                &add_to_tasks, NULL}},
    &(struct cgrp_control){
        .control = "cpu",
        .settings =
            (struct cgrp_setting*[]){
                &(struct cgrp_setting){.name = "cpu.shares", .value = SHARES},
                &add_to_tasks, NULL}},
    &(struct cgrp_control){
        .control = "pids",
        .settings =
            (struct cgrp_setting*[]){
                &(struct cgrp_setting){.name = "pids.max", .value = PIDS},
                &add_to_tasks, NULL}},
    &(struct cgrp_control){
        .control = "blkio",
        .settings =
            (struct cgrp_setting*[]){
                &(struct cgrp_setting){.name = "blkio.weight", .value = PIDS},
                &add_to_tasks, NULL}},
    NULL};
int resources(struct child_config* config) {
  fprintf(stderr, "=> setting cgroups...");
  for (struct cgrp_control** cgrp = cgrps; *cgrp; cgrp++) {
    char dir[PATH_MAX] = {0};
    fprintf(stderr, "%s...", (*cgrp)->control);
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s", (*cgrp)->control,
                 config->hostname) == -1) {
      return -1;
    }
    if (mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR)) {
      fprintf(stderr, "mkdir %s failed: %m\n", dir);
      return -1;
    }
    for (struct cgrp_setting** setting = (*cgrp)->settings; *setting;
         setting++) {
      char path[PATH_MAX] = {0};
      int fd = 0;
      if (snprintf(path, sizeof(path), "%s/%s", dir, (*setting)->name) == -1) {
        fprintf(stderr, "snprintf failed: %m\n");
        return -1;
      }
      if ((fd = open(path, O_WRONLY)) == -1) {
        fprintf(stderr, "opening %s failed: %m\n", path);
        return -1;
      }
      if (write(fd, (*setting)->value, strlen((*setting)->value)) == -1) {
        fprintf(stderr, "writing to %s failed: %m\n", path);
        close(fd);
        return -1;
      }
      close(fd);
    }
  }
  fprintf(stderr, "done.\n");
  fprintf(stderr, "=> setting rlimit...");
  if (setrlimit(RLIMIT_NOFILE, &(struct rlimit){
                                   .rlim_max = FD_COUNT,
                                   .rlim_cur = FD_COUNT,
                               })) {
    fprintf(stderr, "failed: %m\n");
    return 1;
  }
  fprintf(stderr, "done.\n");
  return 0;
}

int free_resources(struct child_config* config) {
  fprintf(stderr, "=> cleaning cgroups...");
  for (struct cgrp_control** cgrp = cgrps; *cgrp; cgrp++) {
    char dir[PATH_MAX] = {0};
    char task[PATH_MAX] = {0};
    int task_fd = 0;
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s", (*cgrp)->control,
                 config->hostname) == -1 ||
        snprintf(task, sizeof(task), "/sys/fs/cgroup/%s/tasks",
                 (*cgrp)->control) == -1) {
      fprintf(stderr, "snprintf failed: %m\n");
      return -1;
    }
    if ((task_fd = open(task, O_WRONLY)) == -1) {
      fprintf(stderr, "opening %s failed: %m\n", task);
      return -1;
    }
    if (write(task_fd, "0", 2) == -1) {
      fprintf(stderr, "writing to %s failed: %m\n", task);
      close(task_fd);
      return -1;
    }
    close(task_fd);
    if (rmdir(dir)) {
      fprintf(stderr, "rmdir %s failed: %m", dir);
      return -1;
    }
  }
  fprintf(stderr, "done.\n");
  return 0;
}
