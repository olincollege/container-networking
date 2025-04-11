/* -*- compile-command: "gcc -Wall -Werror -lcap -lseccomp contained.c -o
 * contained" -*- */
/* This code is licensed under the GPLv3. You can find its text here:
   https://www.gnu.org/licenses/gpl-3.0.en.html */

#include "cgroups.c"
#include "syscalls.c"
#include "utils.c"

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



int mounts(struct child_config* config) {
  fprintf(stderr, "=> remounting everything with MS_PRIVATE...");
  if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)) {
    fprintf(stderr, "failed! %m\n");
    return -1;
  }
  fprintf(stderr, "remounted.\n");

  fprintf(stderr, "=> making a temp directory and a bind mount there...");
  char mount_dir[] = "/tmp/tmp.XXXXXX";
  if (!mkdtemp(mount_dir)) {
    fprintf(stderr, "failed making a directory!\n");
    return -1;
  }

  if (mount(config->mount_dir, mount_dir, NULL, MS_BIND | MS_PRIVATE, NULL)) {
    fprintf(stderr, "bind mount failed!\n");
    return -1;
  }

  char inner_mount_dir[] = "/tmp/tmp.XXXXXX/oldroot.XXXXXX";
  memcpy(inner_mount_dir, mount_dir, sizeof(mount_dir) - 1);
  if (!mkdtemp(inner_mount_dir)) {
    fprintf(stderr, "failed making the inner directory!\n");
    return -1;
  }
  fprintf(stderr, "done.\n");

  fprintf(stderr, "=> pivoting root...");
  if (pivot_root(mount_dir, inner_mount_dir)) {
    fprintf(stderr, "failed!\n");
    return -1;
  }
  fprintf(stderr, "done.\n");

  char* old_root_dir = basename(inner_mount_dir);
  char old_root[sizeof(inner_mount_dir) + 1] = {"/"};
  strcpy(&old_root[1], old_root_dir);

  fprintf(stderr, "=> unmounting %s...", old_root);
  if (chdir("/")) {
    fprintf(stderr, "chdir failed! %m\n");
    return -1;
  }
  if (umount2(old_root, MNT_DETACH)) {
    fprintf(stderr, "umount failed! %m\n");
    return -1;
  }
  if (rmdir(old_root)) {
    fprintf(stderr, "rmdir failed! %m\n");
    return -1;
  }
  fprintf(stderr, "done.\n");
  return 0;
}

#define USERNS_OFFSET 10000
#define USERNS_COUNT 2000

int handle_child_uid_map(pid_t child_pid, int fd) {
  int uid_map = 0;
  int has_userns = -1;
  if (read(fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns)) {
    fprintf(stderr, "couldn't read from child!\n");
    return -1;
  }
  if (has_userns) {
    char path[PATH_MAX] = {0};
    for (char** file = (char*[]){"uid_map", "gid_map", 0}; *file; file++) {
      if (snprintf(path, sizeof(path), "/proc/%d/%s", child_pid, *file) >
          sizeof(path)) {
        fprintf(stderr, "snprintf too big? %m\n");
        return -1;
      }
      fprintf(stderr, "writing %s...", path);
      if ((uid_map = open(path, O_WRONLY)) == -1) {
        fprintf(stderr, "open failed: %m\n");
        return -1;
      }
      if (dprintf(uid_map, "0 %d %d\n", USERNS_OFFSET, USERNS_COUNT) == -1) {
        fprintf(stderr, "dprintf failed: %m\n");
        close(uid_map);
        return -1;
      }
      close(uid_map);
    }
  }
  if (write(fd, &(int){0}, sizeof(int)) != sizeof(int)) {
    fprintf(stderr, "couldn't write: %m\n");
    return -1;
  }
  return 0;
}
int userns(struct child_config* config) {
  fprintf(stderr, "=> trying a user namespace...");
  int has_userns = !unshare(CLONE_NEWUSER);
  if (write(config->fd, &has_userns, sizeof(has_userns)) !=
      sizeof(has_userns)) {
    fprintf(stderr, "couldn't write: %m\n");
    return -1;
  }
  int result = 0;
  if (read(config->fd, &result, sizeof(result)) != sizeof(result)) {
    fprintf(stderr, "couldn't read: %m\n");
    return -1;
  }
  if (result) return -1;
  if (has_userns) {
    fprintf(stderr, "done.\n");
  } else {
    fprintf(stderr, "unsupported? continuing.\n");
  }
  fprintf(stderr, "=> switching to uid %d / gid %d...", config->uid,
          config->uid);
  if (setgroups(1, &(gid_t){config->uid}) ||
      setresgid(config->uid, config->uid, config->uid) ||
      setresuid(config->uid, config->uid, config->uid)) {
    fprintf(stderr, "%m\n");
    return -1;
  }
  fprintf(stderr, "done.\n");
  return 0;
}
int child(void* arg) {
  struct child_config* config = arg;
  if (sethostname(config->hostname, strlen(config->hostname)) ||
      mounts(config) || userns(config) || capabilities() || syscalls()) {
    close(config->fd);
    return -1;
  }
  if (close(config->fd)) {
    fprintf(stderr, "close failed: %m\n");
    return -1;
  }
  if (execve(config->argv[0], config->argv, NULL)) {
    fprintf(stderr, "execve failed! %m.\n");
    return -1;
  }
  return 0;
}

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
          fprintf(stderr, "badly-formatted uid: %s\n", optarg);
          goto usage;
        }
        break;
      default:
        goto usage;
    }
    last_optind = optind;
  }
