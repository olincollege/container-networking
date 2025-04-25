#include "../src/utils.h"

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/new/assert.h>
#include <criterion/redirect.h>
#include <string.h>
#include <unistd.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

Test(utils, choose_hostname_format_and_length) {
    char buffer[256];
    int result = choose_hostname(buffer, sizeof(buffer));

    cr_assert_eq(result, 0, "choose_hostname should return 0");
    cr_assert(strlen(buffer) > 0, "Hostname should not be empty");
    cr_assert(strchr(buffer, '-') != NULL, "Hostname should have a dash");
}

// Mockable function for uid/gid map writing
int write_uid_gid_map(pid_t child_pid, const char* file, int uid, int count) {
    cr_log_info("Mock write_uid_gid_map for PID %d, file %s\n", child_pid, file);
    cr_assert(
    strcmp(file, "uid_map") == 0 || strcmp(file, "gid_map") == 0,
    "file should be either uid_map or gid_map, but was %s", file
    );
    cr_expect_eq(uid, 1000);
    cr_expect_eq(count, 1);
    return 0;  // Simulate success
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

// Mocks to replace read/write
ssize_t __wrap_write(int fd, const void* buf, size_t count) {
    (void)fd;
    (void)buf;
    return (ssize_t)count;  // pretend write always succeeds
}

ssize_t __wrap_read(int fd, void* buf, size_t count) {
    (void)fd;
    if (count >= sizeof(int)) {
        *(int*)buf = 0;  // simulate "OK" response
    }
    return (ssize_t)count;
}

Test(utils, userns_success) {
    struct child_config config = {
        .fd = 42,
        .uid = 1000,
        .argc = 0,
        .argv = NULL,
        .hostname = "unit-test",
        .mount_dir = NULL
    };

    int result = userns(&config);
    cr_assert_eq(result, 0);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
