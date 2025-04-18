#include "utils.h"

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

void error_and_exit(const char* error_msg) {
  perror(error_msg);
  cleanup();

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  exit(EXIT_FAILURE);
}

void kill_and_finish_child(pid_t child_pid) {
  if (child_pid) {
    kill(child_pid, SIGKILL);
  }
  finish_child(child_pid);
  clear_resources();
  cleanup();
  error_and_exit("");
}

int finish_child(pid_t child_pid) {
  int child_status = 0;
  waitpid(child_pid, &child_status, 0);
  err |= WEXITSTATUS(child_status);
}


void cleanup() {
  if (sockets[0]) close(sockets[0]);
  if (sockets[1]) close(sockets[1]);
  // return err;
  // exit(status);
}

int clear_resources() {
  free_resources(&config);
  free(stack);

  cleanup();
}

// void usage(char** argv) {
//   (void)fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);
//   error_and_exit("Incorrect usage");
// }

int finish_options(const struct child_config* configptr, int sockets[2], pid_t child_pid) {
  struct child_config config = *configptr;
  if (!config.argc || !config.mount_dir) {
    (void)fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", config.argv[0]);
    error_and_exit("Incorrect usage");
  }

  (void)fprintf(stderr, "=> validating Linux version...");
  struct utsname host = {0};
  if (uname(&host)) {
    (void)fprintf(stderr, "failed: %m\n");
    cleanup();
  }

  int major = -1;
  int minor = -1;
  if (sscanf(host.release, "%u.%u.", &major, &minor) != 2) {
    (void)fprintf(stderr, "weird release format: %s\n", host.release);
    cleanup();
  }
  if (major != 6 || (minor != 8 && minor != 10)) {
    (void)fprintf(stderr, "expected 4.7.x or 4.8.x: %s\n", host.release);
    cleanup();
  }

  if (strcmp("x86_64", host.machine)) {
    (void)fprintf(stderr, "expected x86_64: %s\n", host.machine);
    cleanup();
  }
  (void)fprintf(stderr, "%s on %s.\n", host.release, host.machine);

  char hostname[256] = {0};
  if (choose_hostname(hostname, sizeof(hostname))) {
    error_and_exit("Error choosing hostname");
  }
  config.hostname = hostname;

  if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
    (void)fprintf(stderr, "socketpair failed: %m\n");
    error_and_exit("socketpair failed");
  }
  if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
    (void)fprintf(stderr, "fcntl failed: %m\n");
    error_and_exit("fcntl failed");
  }
  config.fd = sockets[1];
  #define STACK_SIZE (1024 * 1024)

  char* stack = 0;
  if (!(stack = malloc(STACK_SIZE))) {
    (void)fprintf(stderr, "=> malloc failed, out of memory?\n");
    error_and_exit("out of memory");
  }
  if (resources(&config)) {
    clear_resources();
    error_and_exit("");
  }
  int flags = CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWIPC |
              CLONE_NEWNET | CLONE_NEWUTS;
  if ((child_pid = clone(child, stack + STACK_SIZE, flags | SIGCHLD, &config)) == -1) {
    (void)fprintf(stderr, "=> clone failed! %m\n");
    clear_resources();
    error_and_exit("clone failed");
  }

  close(sockets[1]);
  sockets[1] = 0;
  close(sockets[1]);
  sockets[1] = 0;
  if (handle_child_uid_map(child_pid, sockets[0])) {
    kill_and_finish_child(child_pid);
  }

  finish_child(child_pid);
}

// finish_options : 

// if (!config.argc) goto usage;
// if (!config.mount_dir) goto usage;

// fprintf(stderr, "=> validating Linux version...");
// struct utsname host = {0};
// if (uname(&host)) {
//   fprintf(stderr, "failed: %m\n");
//   goto cleanup;
// }
// int major = -1;
// int minor = -1;
// if (sscanf(host.release, "%u.%u.", &major, &minor) != 2) {
//   fprintf(stderr, "weird release format: %s\n", host.release);
//   goto cleanup;
// }
// if (major != 6 || (minor != 8 && minor != 10)) {
//   fprintf(stderr, "expected 4.7.x or 4.8.x: %s\n", host.release);
//   goto cleanup;
// }
// if (strcmp("x86_64", host.machine)) {
//   fprintf(stderr, "expected x86_64: %s\n", host.machine);
//   goto cleanup;
// }
// fprintf(stderr, "%s on %s.\n", host.release, host.machine);

// char hostname[256] = {0};
// if (choose_hostname(hostname, sizeof(hostname))) goto error;
// config.hostname = hostname;

// if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
//   fprintf(stderr, "socketpair failed: %m\n");
//   goto error;
// }
// if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
//   fprintf(stderr, "fcntl failed: %m\n");
//   goto error;
// }
// config.fd = sockets[1];
// #define STACK_SIZE (1024 * 1024)

// char* stack = 0;
// if (!(stack = malloc(STACK_SIZE))) {
//   fprintf(stderr, "=> malloc failed, out of memory?\n");
//   goto error;
// }
// if (resources(&config)) {
//   err = 1;
//   goto clear_resources;
// }
// int flags = CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWIPC |
//             CLONE_NEWNET | CLONE_NEWUTS;
// if ((child_pid = clone(child, stack + STACK_SIZE, flags | SIGCHLD, &config)) ==
//     -1) {
//   fprintf(stderr, "=> clone failed! %m\n");
//   err = 1;
//   goto clear_resources;
// }
// close(sockets[1]);
// sockets[1] = 0;
// close(sockets[1]);
// sockets[1] = 0;
// if (handle_child_uid_map(child_pid, sockets[0])) {
//   // err = 1;
//   // goto kill_and_finish_child;
//   kill_and_finish_child(child_pid);
// }

// goto finish_child;
