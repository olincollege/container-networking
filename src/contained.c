/* -*- compile-command: "gcc -Wall -Werror -lcap -lseccomp contained.c -o
 * contained" -*- */
/* This code is licensed under the GPLv3. You can find its text here:
   https://www.gnu.org/licenses/gpl-3.0.en.html */

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
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  exit(EXIT_FAILURE);
}

struct child_config {
  int argc;
  uid_t uid;
  int fd;
  char* hostname;
  char** argv;
  char* mount_dir;
};

int capabilities() {
  fprintf(stderr, "=> dropping capabilities...");
  int drop_caps[] = {
      CAP_AUDIT_CONTROL,   CAP_AUDIT_READ,   CAP_AUDIT_WRITE, CAP_BLOCK_SUSPEND,
      CAP_DAC_READ_SEARCH, CAP_FSETID,       CAP_IPC_LOCK,    CAP_MAC_ADMIN,
      CAP_MAC_OVERRIDE,    CAP_MKNOD,        CAP_SETFCAP,     CAP_SYSLOG,
      CAP_SYS_ADMIN,       CAP_SYS_BOOT,     CAP_SYS_MODULE,  CAP_SYS_NICE,
      CAP_SYS_RAWIO,       CAP_SYS_RESOURCE, CAP_SYS_TIME,    CAP_WAKE_ALARM};
  size_t num_caps = sizeof(drop_caps) / sizeof(*drop_caps);
  fprintf(stderr, "bounding...");
  for (size_t i = 0; i < num_caps; i++) {
    if (prctl(PR_CAPBSET_DROP, drop_caps[i], 0, 0, 0)) {
      fprintf(stderr, "prctl failed: %m\n");
      return 1;
    }
  }
  fprintf(stderr, "inheritable...");
  cap_t caps = NULL;
  if (!(caps = cap_get_proc()) ||
      cap_set_flag(caps, CAP_INHERITABLE, num_caps, drop_caps, CAP_CLEAR) ||
      cap_set_proc(caps)) {
    fprintf(stderr, "failed: %m\n");
    if (caps) cap_free(caps);
    return 1;
  }
  cap_free(caps);
  fprintf(stderr, "done.\n");
  return 0;
}

int pivot_root(const char* new_root, const char* put_old) {
  return syscall(SYS_pivot_root, new_root, put_old);
}

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

#define SCMP_FAIL SCMP_ACT_ERRNO(EPERM)

int syscalls() {
  scmp_filter_ctx ctx = NULL;
  fprintf(stderr, "=> filtering syscalls...");
  if (!(ctx = seccomp_init(SCMP_ACT_ALLOW)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
                       SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
                       SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(
          ctx, SCMP_FAIL, SCMP_SYS(unshare), 1,
          SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)) ||
      seccomp_rule_add(
          ctx, SCMP_FAIL, SCMP_SYS(clone), 1,
          SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ioctl), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, TIOCSTI, TIOCSTI)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(keyctl), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(add_key), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(request_key), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ptrace), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(mbind), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(migrate_pages), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(move_pages), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(set_mempolicy), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(userfaultfd), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(perf_event_open), 0) ||
      seccomp_attr_set(ctx, SCMP_FLTATR_CTL_NNP, 0) || seccomp_load(ctx)) {
    if (ctx) seccomp_release(ctx);
    fprintf(stderr, "failed: %m\n");
    return 1;
  }
  seccomp_release(ctx);
  fprintf(stderr, "done.\n");
  return 0;
}

#define MEMORY "1073741824"
#define SHARES "256"
#define PIDS "64"
#define WEIGHT "10"
#define FD_COUNT 64

typedef struct cgrp_setting {
  char* name;
  char* value;
} cgrp_setting;

// represents a cgroup controller for things like memory and cpu
typedef struct cgrp_control {
  char* control;
  cgrp_setting* settings[]; 
} cgrp_control;

// shread setting for all cgroups
const cgrp_setting add_to_tasks = {.name = "tasks", .value = "0"};

// individual settings for cgroups 
// values are macros defined up above - come back to why these values later
// settings will limit how much resources the container gets for these processes
const cgrp_setting mem_limit       = { .name = "memory.limit_in_bytes",     .value = MEMORY };
const cgrp_setting kmem_limit      = { .name = "memory.kmem.limit_in_bytes",.value = MEMORY };
const cgrp_setting cpu_shares      = { .name = "cpu.shares",                .value = SHARES };
const cgrp_setting pids_max        = { .name = "pids.max",                  .value = PIDS };
const cgrp_setting blkio_weight    = { .name = "blkio.weight",              .value = PIDS };

// define control groups with respective settings
const cgrp_control memory_cgroup = {
  .control = "memory",
  .settings = { &mem_limit, &kmem_limit, &add_to_tasks, NULL }
};

const cgrp_control cpu_cgroup = {
  .control = "cpu",
  .settings = { &cpu_shares, &add_to_tasks, NULL }
};

const cgrp_control pids_cgroup = {
  .control = "pids",
  .settings = { &pids_max, &add_to_tasks, NULL }
};

const cgrp_control blkio_cgroup = {
  .control = "blkio",
  .settings = { &blkio_weight, &add_to_tasks, NULL }
};

// shouldn't be changed by the code later on
const cgrp_control* const cgrps[] = {
  &memory_cgroup,
  &cpu_cgroup,
  &pids_cgroup,
  &blkio_cgroup,
  NULL
};

int resources(struct child_config* config) {
  (void)fprintf(stderr, "=> setting cgroups...");
  // iterate through our cgroup controllers
  // Create a cgroup directory for each controller under /sys/fs/cgroup/<controller>/<hostname>
  for (cgrp_control** cgrp = cgrps; *cgrp; cgrp++) {
    char dir[PATH_MAX] = {0};
    (void)fprintf(stderr, "%s...", (*cgrp)->control);
    // write formatted string into char array dir
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s", (*cgrp)->control,
                config->hostname) == -1) {
                  error_and_exit("Failed to write directory name - too long?")
                  return -1;
    }
    // create folder with permissions
    // S_IRUSR - read permissions for owner
    // S_IRUSR - write permissions for owner
    // S_IXUSR - execute permissions for owner
    if (mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR)) {
      error_and_exit("mkdir failed")
      return -1;
    }
    // for each setting associated with the current cgrp
    for (cgrp_setting** setting = (*cgrp)->settings; *setting;
         setting++) {
      char path[PATH_MAX] = {0};
      int file_des = 0;
      if (snprintf(path, sizeof(path), "%s/%s", dir, (*setting)->name) == -1) {
        error_and_exit("Failed to write settings file name  - too long?");
        return -1;
      }
      // open settings file with write only flag (originally used O_WRONLY)
      if ((file_des = open(path, O_CLOEXEC)) == -1) {
        error_and_exit("opening settings file failed");
        return -1;
      }
      // write into new settings file
      if (write(file_des, (*setting)->value, strlen((*setting)->value)) == -1) {
        error_and_exit("writing to settings file failed");
        close(file_des);
        return -1;
      }
      close(file_des);
    }
  }
  (void)fprintf(stderr, "done.\n");
  (void)fprintf(stderr, "=> setting rlimit...");
  // setrlimit - set resource limits
  // Each resource has an associated soft and hard limit, as  defined by the rlimit structure
  // A container can only open FD_COUNT files/sockets at a time
  if (setrlimit(RLIMIT_NOFILE, &(struct rlimit){
                                   .rlim_max = FD_COUNT,  // limit the number of open file descriptors (hard limit)
                                   .rlim_cur = FD_COUNT,  // limit the number of open file descriptors (hard limit)
                               })) {
    error_and_exit("Failed to set file limit");
    return 1;
  }
  (void)fprintf(stderr, "done.\n");
  return 0;
}

int free_resources(struct child_config* config) {
  (void)fprintf(stderr, "=> cleaning cgroups...");
  // for each cgroup controller
  for (cgrp_control** cgrp = cgrps; *cgrp; cgrp++) {
    char dir[PATH_MAX] = {0};
    char task[PATH_MAX] = {0};
    int task_fd = 0;
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s", (*cgrp)->control,
                 config->hostname) == -1 ||
        snprintf(task, sizeof(task), "/sys/fs/cgroup/%s/tasks",
                 (*cgrp)->control) == -1) {
      error_and_exit("snprintf failed")
      return -1;
    }
    // originally used O_WRONLY flag
    if ((task_fd = open(task, O_CLOEXEC)) == -1) {
      error_and_exit("Opening task file failed")
      return -1;
    }
    // empties tasks from the cgroup so we can remove the directory
    if (write(task_fd, "0", 2) == -1) {
      error_and_exit("Writing to task file failed")
      close(task_fd);
      return -1;
    }
    close(task_fd);
    // remove directory
    if (rmdir(dir)) {
      error_and_exit("Failed to remove directory");
      return -1;
    }
  }
  (void)fprintf(stderr, "done.\n");
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
finish_options:
  if (!config.argc) goto usage;
  if (!config.mount_dir) goto usage;

  fprintf(stderr, "=> validating Linux version...");
  struct utsname host = {0};
  if (uname(&host)) {
    fprintf(stderr, "failed: %m\n");
    goto cleanup;
  }
  int major = -1;
  int minor = -1;
  if (sscanf(host.release, "%u.%u.", &major, &minor) != 2) {
    fprintf(stderr, "weird release format: %s\n", host.release);
    goto cleanup;
  }
  if (major != 4 || (minor != 7 && minor != 8)) {
    fprintf(stderr, "expected 4.7.x or 4.8.x: %s\n", host.release);
    goto cleanup;
  }
  if (strcmp("x86_64", host.machine)) {
    fprintf(stderr, "expected x86_64: %s\n", host.machine);
    goto cleanup;
  }
  fprintf(stderr, "%s on %s.\n", host.release, host.machine);

  char hostname[256] = {0};
  if (choose_hostname(hostname, sizeof(hostname))) goto error;
  config.hostname = hostname;

  if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
    fprintf(stderr, "socketpair failed: %m\n");
    goto error;
  }
  if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
    fprintf(stderr, "fcntl failed: %m\n");
    goto error;
  }
  config.fd = sockets[1];
#define STACK_SIZE (1024 * 1024)

  char* stack = 0;
  if (!(stack = malloc(STACK_SIZE))) {
    fprintf(stderr, "=> malloc failed, out of memory?\n");
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
    fprintf(stderr, "=> clone failed! %m\n");
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
  err |= WEXITSTATUS(child_status);
clear_resources:
  free_resources(&config);
  free(stack);

  goto cleanup;
usage:
  fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);
error:
  err = 1;
cleanup:
  if (sockets[0]) close(sockets[0]);
  if (sockets[1]) close(sockets[1]);
  return err;
}
