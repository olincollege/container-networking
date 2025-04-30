#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/new/assert.h>
#include <criterion/redirect.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/utils.h"

// mocks -- might cause issues if compiled on a diff computer, I can fix it when
// we try again, its a fix in cmake

int __wrap_open(const char* pathname, int flags, ...) {
  (void)pathname;
  (void)flags;
  return 3;
}

int __wrap_dprintf(int fd, const char* format, ...) {
  (void)fd;
  (void)format;
  return 0;
}

ssize_t __wrap_write(int fd, const void* buf, size_t count) {
  (void)fd;
  (void)buf;
  return (ssize_t)count;
}

ssize_t __wrap_read(int fd, void* buf, size_t count) {
  (void)fd;
  if (count >= sizeof(int)) {
    *(int*)buf = 0;
  }
  return (ssize_t)count;
}

// Tests

Test(utils, choose_hostname_format_and_length) {
  char buffer[256];
  int result = choose_hostname(buffer, sizeof(buffer));

  cr_assert_eq(result, 0, "choose_hostname should return 0");
  cr_assert(strlen(buffer) > 0, "Hostname should not be empty");
  cr_assert(strchr(buffer, '-') != NULL, "Hostname should have a dash");
}

Test(utils, handle_child_uid_map_success) {
  int pipefd[2];
  cr_assert(pipe(pipefd) == 0, "pipe creation failed");

  int has_userns = 1;
  cr_assert(write(pipefd[1], &has_userns, sizeof(int)) == sizeof(int),
            "write to pipe failed");
  lseek(pipefd[0], 0, SEEK_SET);  // rewind read pipe

  int result = handle_child_uid_map(getpid(), pipefd[0]);
  cr_assert_eq(result, 0, "handle_child_uid_map should succeed");

  close(pipefd[0]);
  close(pipefd[1]);
}

Test(utils, userns_success) {
  int pipefd[2];
  cr_assert(pipe(pipefd) == 0, "pipe creation failed");

  struct child_config config = {.fd = pipefd[1],
                                .uid = 1000,
                                .argc = 0,
                                .argv = NULL,
                                .hostname = "unit-test",
                                .mount_dir = NULL};

  int result = userns(&config);
  cr_assert_eq(result, 0, "userns should succeed");

  close(pipefd[0]);
  close(pipefd[1]);
}
