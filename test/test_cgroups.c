#include "../src/cgroups.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/new/assert.h>
#include <criterion/redirect.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

Test(cgroups, file_existence) {
    struct child_config test_config = {
        .argc = 0,
        .uid = 0,
        .fd = 0,
        .hostname = "test-container",
        .argv = NULL,
        .mount_dir = NULL
    };
    const char* files[4] = {"memory.max", "cpu.max", "pids.max", "io.weight"};
    char file_path[PATH_MAX] = {0};
    resources(&test_config);
    for (int i = 0; i < 4; i++) {
        // assume if snprintf doesn't fail in resources(), it won't fail here
        //NOLINTNEXTLINE
        (void)snprintf(file_path, sizeof(file_path), "/sys/fs/cgroup/test-container/%s", files[i]);
        cr_assert(eq(int, access(file_path, F_OK), 0));
    } 
    free_resources(&test_config);
}

Test(cgroups, file_values) {
    struct child_config test_config = {
        .argc = 0,
        .uid = 0,
        .fd = 0,
        .hostname = "test-container",
        .argv = NULL,
        .mount_dir = NULL
    };
    const char* files[4] = {"memory.max", "cpu.max", "pids.max", "io.weight"};
    // defined by macros
    char* correct_vals[4] = {"1073741824\n", "100000 100000\n", "64\n", "default 10\n"};
    char file_path[PATH_MAX] = {0};
    resources(&test_config);
    for (int i = 0; i < 4; i++) {
        // assume if snprintf doesn't fail in resources(), it won't fail here
        //NOLINTNEXTLINE
        (void)snprintf(file_path, sizeof(file_path), "/sys/fs/cgroup/test-container/%s", files[i]);
        char value[64] = {0};
        int file_des = open(file_path, O_RDONLY | O_CLOEXEC);
        read(file_des, value, sizeof(value));
        close(file_des);
        cr_assert(eq(str, value, correct_vals[i]));
    } 
    free_resources(&test_config);
}

Test(cgroups, remove_cgroup_dir) {
    struct child_config test_config = {
        .argc = 0,
        .uid = 0,
        .fd = 0,
        .hostname = "test-container",
        .argv = NULL,
        .mount_dir = NULL
    };
    resources(&test_config);
    free_resources(&test_config);
    cr_assert(ne(int, access("/sys/fs/cgroup/test-container", F_OK), 0));
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
