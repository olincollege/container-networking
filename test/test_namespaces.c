// Intentional name to enable GNU-specific features 
// NOLINTNEXTLINE
#define _GNU_SOURCE
#include <criterion/criterion.h>
#include <criterion/logging.h>  
#include <criterion/new/assert.h>
#include <fcntl.h>               
#include <string.h>              
#include <sys/types.h>            
#include <unistd.h>

#include "../src/namespaces.h"

enum { DEFAULT_TEST_UID = 1000 };
enum { HOSTNAME_BUFFER_SIZE = 256 };


// mocks -- might cause issues if compiled on a diff computer, I can fix it when
// we try again, its a fix in cmake

// Intentional names flagged for wrapping
// NOLINTNEXTLINE
int __wrap_open(const char* pathname, int flags, ...) {
  (void)pathname;
  (void)flags;
  return 3;
}

// NOLINTNEXTLINE
int __wrap_dprintf(int file_descriptor, const char* format, ...) {
  (void)file_descriptor;
  (void)format;
  return 0;
}

// NOLINTNEXTLINE
ssize_t __wrap_write(int file_descriptor, const void* buf, size_t count) {
  (void)file_descriptor;
  (void)buf;
  return (ssize_t)count;
}

// NOLINTNEXTLINE
ssize_t __wrap_read(int file_descriptor, void* buf, size_t count) {
  (void)file_descriptor;
  if (count >= sizeof(int)) {
    *(int*)buf = 0;
  }
  return (ssize_t)count;
}

// Tests

Test(namespaces, choose_hostname_format_and_length) {
  char buffer[HOSTNAME_BUFFER_SIZE];
  int result = choose_hostname(buffer, sizeof(buffer));

  cr_assert_eq(result, 0, "choose_hostname should return 0");
  cr_assert(strlen(buffer) > 0, "Hostname should not be empty");
  cr_assert(strchr(buffer, '-') != NULL, "Hostname should have a dash");
}

Test(namespaces, handle_child_uid_map_success) {
  int pipefd[2];
  cr_assert(pipe2(pipefd, O_CLOEXEC) == 0, "pipe creation failed");

  int has_userns = 1;
  cr_assert(write(pipefd[1], &has_userns, sizeof(int)) == sizeof(int),
            "write to pipe failed");
  lseek(pipefd[0], 0, SEEK_SET);  // rewind read pipe

  int result = handle_child_uid_map(getpid(), pipefd[0]);
  cr_assert_eq(result, 0, "handle_child_uid_map should succeed");

  close(pipefd[0]);
  close(pipefd[1]);
}

Test(namespaces, userns_success) {
  int pipefd[2];
  cr_assert(pipe2(pipefd, O_CLOEXEC) == 0, "pipe creation failed");

  struct child_config config = {
    .fd = pipefd[1],
    .uid = DEFAULT_TEST_UID,
    .argc = 0,
    .argv = NULL,
    .hostname = "unit-test",
    .mount_dir = NULL
  };

  int result = userns(&config);
  cr_assert_eq(result, 0, "userns should succeed");

  close(pipefd[0]);
  close(pipefd[1]);
}
