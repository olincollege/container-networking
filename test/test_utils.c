// #include <criterion/criterion.h>
// #include <criterion/logging.h>
#include <string.h>
#include <unistd.h>

#include "../src/utils.h"

Test(utils, choose_hostname_format_and_length) {
  char buffer[256];
  int result = choose_hostname(buffer, sizeof(buffer));

  cr_assert_eq(result, 0, "choose_hostname should return 0");
  cr_assert(strlen(buffer) > 0, "Hostname should not be empty");
  cr_assert(strchr(buffer, '-') != NULL, "Hostnam should have a dash");
}

int write_uid_gid_map(pid_t child_pid, const char* file, int uid, int count) {
  cr_log_info("Mock write_uid_gid_map for PID %d, file %s\n", child_pid,
              file);
  cr_expect_str_one_of(file, (const char*[]){"uid_map", "gid_map"});
  cr_expect_eq(uid, 1000);
  cr_expect_eq(count, 1);
  return 0;  // simulate success
}

Test(utils, handle_child_uid_map_success) {
  int pipefd[2];
  cr_assert(pipe(pipefd) == 0);

  // Simulate child wrote 1 to indicate has_userns = true
  int has_userns = 1;
  cr_assert(write(pipefd[1], &has_userns, sizeof(int)) == sizeof(int));
  lseek(pipefd[0], 0, SEEK_SET);  // rewind read pipe

  int result = handle_child_uid_map(4242, pipefd[0]);
  cr_assert_eq(result, 0);

  close(pipefd[0]);
  close(pipefd[1]);
}

// Mocks
ssize_t __wrap_write(int fd, const void* buf, size_t count) {
  return count;  // pretend write always succeeds
}

ssize_t __wrap_read(int fd, void* buf, size_t count) {
  *(int*)buf = 0;  // sim "OK" response from parent
  return count;
}

Test(utils, userns_success) {
  struct child_config config = {.fd = 42, .uid = 1000};

  int result = userns(&config);
  cr_assert_eq(result, 0);
}
