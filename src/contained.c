/* Compile it with CMake!
 * sudo ./build/src/contained -u 1000 -m . -c /bin/sh ~ */
/* This code is licensed under the GPLv3. You can find its text here:
   https://www.gnu.org/licenses/gpl-3.0.en.html */

#include "contained.h"

void cleanup(int status, void* arg) {
  int* sockets = (int*)arg;
  if (sockets[0]) {
    close(sockets[0]);
  }
  if (sockets[1]) {
    close(sockets[1]);
  }

  (void)fprintf(stderr, "Cleanup function called with status: %d\n", status);
}

void finish_child(int child_pid, int* err) {
  int child_status = 0;
  waitpid(child_pid, &child_status, 0);
  // free(child_pid);
  *err |= WEXITSTATUS(child_status);
}

void clear_resources(struct child_config* config, char* stack) {
  free_resources(config);
  free(stack);
}

void usage(char* path) {
  (void)fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", path);
  exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
  struct child_config config = {0};
  int err = 0;
  int option = 0;
  int sockets[2] = {0};
  pid_t child_pid = 0;
  int last_optind = 0;

  if (on_exit(cleanup, (void*)sockets) != 0) {
    (void)fprintf(stderr, "Failed to register cleanup function\n");
    return EXIT_FAILURE;
  }

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
          usage(argv[0]);
        }
        break;
      default:
        usage(argv[0]);
    }
    last_optind = optind;
  }

finish_options:
  if (!config.argc) {
    usage(argv[0]);
  }
  if (!config.mount_dir) {
    usage(argv[0]);
  }

  (void)fprintf(stderr, "=> validating Linux version...");
  struct utsname host = {0};
  if (uname(&host)) {
    error_and_exit("failed: ");
  }
  int major = -1;
  int minor = -1;
  if (sscanf(host.release, "%u.%u.", &major, &minor) != 2) {
    (void)fprintf(stderr, "weird release format: %s\n", host.release);
    error_and_exit("weird release format");
  }
  if (major != MAJOR_VERSION || minor != MINOR_VERSION) {
    (void)fprintf(stderr, "expected 4.7.x or 4.8.x: %s\n", host.release);
    error_and_exit("version mismatch");
  }
  if (strcmp("x86_64", host.machine)) {
    (void)fprintf(stderr, "expected x86_64: %s\n", host.machine);
    error_and_exit("architecture mismatch");
  }
  (void)fprintf(stderr, "%s on %s.\n", host.release, host.machine);

  char hostname[256] = {0};
  if (choose_hostname(hostname, sizeof(hostname))) {
    error_and_exit("Choosing hostname failed");
  }
  config.hostname = hostname;

  if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
    error_and_exit("socketpair failed:");
  }
  if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
    error_and_exit("fcntl failed: ");
  }
  config.fd = sockets[1];
#define STACK_SIZE (1024 * 1024)

  char* stack = 0;
  if (!(stack = malloc(STACK_SIZE))) {
    error_and_exit("=> malloc failed, out of memory?\n");
  }
  if (resources(&config)) {
    clear_resources(&config, stack);
    exit(EXIT_FAILURE);
  }
  int flags = CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWIPC |
              CLONE_NEWNET | CLONE_NEWUTS;
  if ((child_pid =
           clone(child, stack + STACK_SIZE, flags | SIGCHLD, &config)) == -1) {
    error_and_exit("=> clone failed!");
    clear_resources(&config, stack);
    exit(EXIT_FAILURE);
  }
  close(sockets[1]);
  sockets[1] = 0;
  close(sockets[1]);
  sockets[1] = 0;
  if (handle_child_uid_map(child_pid, sockets[0])) {
    if (child_pid) {
      kill(child_pid, SIGKILL);
    }
    finish_child(child_pid, &err);
    clear_resources(&config, stack);
    exit(EXIT_FAILURE);
  }

  finish_child(child_pid, &err);
  clear_resources(&config, stack);
  exit(err);
  // kill_and_finish_child:
  // if (child_pid) {
  //   kill(child_pid, SIGKILL);
  // }
  // finish_child:
  //   int child_status = 0;
  //   waitpid(child_pid, &child_status, 0);
  //   // free(child_pid);
  //   err |= WEXITSTATUS(child_status);
  // clear_resources:
  //   free_resources(&config);
  //   free(stack);

  //   goto cleanup;
  // usage:
  //   (void)fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);
  // error:
  //   err = 1;
  // cleanup:
  //   if (sockets[0]) {
  //     close(sockets[0]);
  //   }
  //   if (sockets[1]) {
  //     close(sockets[1]);
  //   }
  //   return err;
  // exit(err);
}
