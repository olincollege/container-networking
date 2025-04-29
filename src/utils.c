
#include "utils.h"

// function 1: choose host name and initialize socketpair and fcntl
int choose_hostname(char* buff, size_t len) {
  static const char* suits[] = {"swords", "wands", "pentacles", "cups"};
  static const char* minor[] = {"ace",  "two",    "three", "four", "five",
                                "six",  "seven",  "eight", "nine", "ten",
                                "page", "knight", "queen", "king"};
  static const char* major[] = {
      "fool",       "magician", "high-priestess", "empress",  "emperor",
      "hierophant", "lovers",   "chariot",        "strength", "hermit",
      "wheel",      "justice",  "hanged-man",     "death",    "temperance",
      "devil",      "tower",    "star",           "moon",     "sun",
      "judgment",   "world"};

  struct timespec now = {0};
  clock_gettime(CLOCK_MONOTONIC, &now);
  size_t ix = now.tv_nsec % 78;

  if (ix < sizeof(major) / sizeof(*major)) {
    snprintf(buff, len, "%05lx-%s", now.tv_sec, major[ix]);
  } else {
    ix -= sizeof(major) / sizeof(*major);
    snprintf(buff, len, "%05lxc-%s-of-%s", now.tv_sec,
             minor[ix % (sizeof(minor) / sizeof(*minor))],
             suits[ix / (sizeof(minor) / sizeof(*minor))]);
  }

  return 0;
}

// function 2: allocate stack and clone child process
int allocate_stack_and_clone(int sockets[2], struct child_config* config) {
  char* stack = NULL;
  pid_t child_pid = 0;

  stack = malloc((size_t)STACK_SIZE);
  if (!stack) {
    error_and_exit("=> malloc failed, out of memory?");
    // return -1;
  }

  if (resources(config)) {
    free(stack);
    return -1;
  }

  int flags = CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWIPC |
              CLONE_NEWNET | CLONE_NEWUTS;

  child_pid = clone(child, stack + STACK_SIZE, flags | SIGCHLD, config);
  if (child_pid == -1) {
    // (void)fprintf(stderr, "=> clone failed! %m\n");
    free(stack);
    perror("=> clone failed!");
    return -1;
  }

  close(sockets[1]);
  sockets[1] = 0;

  // You’ll want to free `stack` after waitpid in main.c
  return child_pid;
}

// function 3: handle child uid map
int handle_child_uid_map(pid_t child_pid, int fd) {
  int uid_map = 0;
  int has_userns = -1;
  if (read(fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns)) {
    perror("couldn't read from child");
    return -1;
  }

  if (has_userns) {
    char path[PATH_MAX] = {0};
    for (char** file = (char*[]){"uid_map", "gid_map", 0}; *file; file++) {
      if (snprintf(path, sizeof(path), "/proc/%d/%s", child_pid, *file) >
          sizeof(path)) {
        perror("snprintf too big?");
        return -1;
      }
      (void)fprintf(stderr, "writing %s...\n", path);
      if ((uid_map = open(path, O_CLOEXEC)) == -1) {
        perror("failed to open uid_map");
        return -1;
      }
      if (dprintf(uid_map, "0 %d %d\n", USERNS_OFFSET, USERNS_COUNT) == -1) {
        close(uid_map);
        perror("failed to write to uid_map");
        return -1;
      }
      close(uid_map);
    }
  }

  if (write(fd, &(int){0}, sizeof(int)) != sizeof(int)) {
    perror("couldn't write to child");
    return -1;
  }

  return 0;
}

// function 4: user namespace
int userns(struct child_config* config) {
  (void)fprintf(stderr, "=> trying a user namespace...\n");
  int has_userns = !unshare(CLONE_NEWUSER);

  if (write(config->fd, &has_userns, sizeof(has_userns)) !=
      sizeof(has_userns)) {
    perror("couldn't write to parent");
    return -1;
  }

  int result = 0;
  if (read(config->fd, &result, sizeof(result)) != sizeof(result)) {
    perror("couldn't read from parent");
    return -1;
  }

  if (result) {
    return -1;
  }

  if (has_userns) {
    (void)fprintf(stderr, "done.\n");
  } else {
    (void)fprintf(stderr, "unsupported? continuing.\n");
  }

  (void)fprintf(stderr, "=> switching to uid %d / gid %d...\n", config->uid,
                config->uid);
  if (setgroups(1, &(gid_t){config->uid}) ||
      setresgid(config->uid, config->uid, config->uid) ||
      setresuid(config->uid, config->uid, config->uid)) {
    perror("switch failed");
    return -1;
  }

  (void)fprintf(stderr, "done.\n");
  return 0;
}

// function 5: child function executed in new namespaces
int child(void* arg) {
  struct child_config* config = arg;

  if (sethostname(config->hostname, strlen(config->hostname)) ||
      mounts(config) || userns(config) || capabilities() || syscalls()) {
    close(config->fd);
    perror("child failed");
    return -1;
  }

  close(config->fd);
  return 0;
}
