/* -*- compile-command: "gcc -Wall -Werror -lcap -lseccomp contained.c -o
 * contained" -*- */
/* This code is licensed under the GPLv3. You can find its text here:
   https://www.gnu.org/licenses/gpl-3.0.en.html */

#include "contained.h"


int main(int argc, char** argv) {
  struct child_config config = {0};
  int err = 0;
  int option = 0;
  int sockets[2] = {0};
  pid_t child_pid = 0;
  int last_optind = 0;
  while ((option = getopt(argc, argv, "c:m:u:"))) {
    switch (option) {
      case 'c':
        config.argc = argc - last_optind - 1;
        config.argv = &argv[argc - config.argc];
        goto finish_options;
      case 'm':
        config.mount_dir = optarg;
        break;
      case 'u':
        if (sscanf(optarg, "%d", &config.uid) != 1) {
          (void)fprintf(stderr, "badly-formatted uid: %s\n", optarg);
          goto usage;
        }
        break;
      default:
        goto usage;
    }
    last_optind = optind;
  }
finish_options:
  if (!config.argc) goto usage;
  if (!config.mount_dir) goto usage;

  (void)fprintf(stderr, "=> validating Linux version...");
  struct utsname host = {0};
  if (uname(&host)) {
    (void)fprintf(stderr, "failed: %m\n");
    goto cleanup;
  }
  int major = -1;
  int minor = -1;
  if (sscanf(host.release, "%u.%u.", &major, &minor) != 2) {
    (void)fprintf(stderr, "weird release format: %s\n", host.release);
    goto cleanup;
  }
  if (major != 6 || (minor != 7 && minor != 8)) {
    (void)fprintf(stderr, "expected 4.7.x or 4.8.x: %s\n", host.release);
    goto cleanup;
  }
  if (strcmp("x86_64", host.machine)) {
    (void)fprintf(stderr, "expected x86_64: %s\n", host.machine);
    goto cleanup;
  }
  (void)fprintf(stderr, "%s on %s.\n", host.release, host.machine);

  char hostname[256] = {0};
  if (choose_hostname(hostname, sizeof(hostname))) goto error;
  config.hostname = hostname;

  if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
    (void)fprintf(stderr, "socketpair failed: %m\n");
    goto error;
  }
  if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
    (void)fprintf(stderr, "fcntl failed: %m\n");
    goto error;
  }
  config.fd = sockets[1];
#define STACK_SIZE (1024 * 1024)

  char* stack = 0;
  if (!(stack = malloc(STACK_SIZE))) {
    (void)fprintf(stderr, "=> malloc failed, out of memory?\n");
    goto error;
  }
  if (resources(&config)) {
    err = 1;
    goto clear_resources;
  }
  int flags = CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWIPC |
              CLONE_NEWNET | CLONE_NEWUTS;
  if ((child_pid =
           clone(child, stack + STACK_SIZE, flags | SIGCHLD, &config)) == -1) {
    (void)fprintf(stderr, "=> clone failed! %m\n");
    err = 1;
    goto clear_resources;
  }
  close(sockets[1]);
  sockets[1] = 0;
  close(sockets[1]);
  sockets[1] = 0;
  if (handle_child_uid_map(child_pid, sockets[0])) {
    err = 1;
    goto kill_and_finish_child;
  }

  goto finish_child;
kill_and_finish_child:
  if (child_pid) kill(child_pid, SIGKILL);
finish_child:;
  int child_status = 0;
  waitpid(child_pid, &child_status, 0);
  // free(child_pid);
  err |= WEXITSTATUS(child_status);
clear_resources:
  free_resources(&config);
  free(stack);

  goto cleanup;
usage:
  (void)fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);
error:
  err = 1;
cleanup:
  if (sockets[0]) close(sockets[0]);
  if (sockets[1]) close(sockets[1]);
  return err;
}
