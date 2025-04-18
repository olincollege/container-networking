#include "cgroups.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

// from: alex's code - lines 264–285
const cgrp_setting add_to_tasks = {.name = "tasks", .value = "0"};

const cgrp_setting mem_limit    = { .name = "memory.limit_in_bytes",      .value = MEMORY };
const cgrp_setting kmem_limit   = { .name = "memory.kmem.limit_in_bytes", .value = MEMORY };
const cgrp_setting cpu_shares   = { .name = "cpu.shares",                 .value = SHARES };
const cgrp_setting pids_max     = { .name = "pids.max",                   .value = PIDS };
const cgrp_setting blkio_weight = { .name = "blkio.weight",               .value = PIDS };

cgrp_control memory_cgroup = {
    .control = "memory",
    .settings = { &mem_limit, &kmem_limit, &add_to_tasks, NULL }
};

cgrp_control cpu_cgroup = {
    .control = "cpu",
    .settings = { &cpu_shares, &add_to_tasks, NULL }
};

cgrp_control pids_cgroup = {
    .control = "pids",
    .settings = { &pids_max, &add_to_tasks, NULL }
};

cgrp_control blkio_cgroup = {
    .control = "blkio",
    .settings = { &blkio_weight, &add_to_tasks, NULL }
};

cgrp_control* cgrps[] = {
    &memory_cgroup,
    &cpu_cgroup,
    &pids_cgroup,
    &blkio_cgroup,
    NULL
};

// from: function resources(struct child_config* config) — lines 287–323
int resources(struct child_config* config) {
    fprintf(stderr, "=> setting cgroups...\n");

    for (cgrp_control** cgrp = cgrps; *cgrp; cgrp++) {
        char dir[PATH_MAX] = {0};
        fprintf(stderr, "%s...", (*cgrp)->control);

        if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s", (*cgrp)->control,
                     config->hostname) == -1) {
            error_and_exit("Failed to write directory name - too long?");
            return -1;
        }

        if (mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR)) {
            error_and_exit("mkdir failed");
            return -1;
        }

        for (cgrp_setting** setting = (*cgrp)->settings; *setting; setting++) {
            char path[PATH_MAX] = {0};
            int file_des = 0;

            if (snprintf(path, sizeof(path), "%s/%s", dir, (*setting)->name) == -1) {
                error_and_exit("Failed to write settings file name - too long?");
                return -1;
            }

            if ((file_des = open(path, O_CLOEXEC)) == -1) {
                error_and_exit("opening settings file failed");
                return -1;
            }

            if (write(file_des, (*setting)->value, strlen((*setting)->value)) == -1) {
                error_and_exit("writing to settings file failed");
                close(file_des);
                return -1;
            }
            close(file_des);
        }
    }

    fprintf(stderr, "done.\n");
    fprintf(stderr, "=> setting rlimit...");

    if (setrlimit(RLIMIT_NOFILE, &(struct rlimit){
        .rlim_max = FD_COUNT,
        .rlim_cur = FD_COUNT })) {
        error_and_exit("Failed to set file limit");
        return 1;
    }

    fprintf(stderr, "done.\n");
    return 0;
}


// from: function free_resources(struct child_config* config) — lines 325–349
int free_resources(struct child_config* config) {
    fprintf(stderr, "=> cleaning cgroups...\n");

    for (cgrp_control** cgrp = cgrps; *cgrp; cgrp++) {
        char dir[PATH_MAX] = {0};
        char task[PATH_MAX] = {0};
        int task_fd = 0;

        if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s", (*cgrp)->control,
                     config->hostname) == -1 ||
            snprintf(task, sizeof(task), "/sys/fs/cgroup/%s/tasks", (*cgrp)->control) == -1) {
            error_and_exit("snprintf failed");
            return -1;
        }

        if ((task_fd = open(task, O_CLOEXEC)) == -1) {
            error_and_exit("Opening task file failed");
            return -1;
        }

        if (write(task_fd, "0", 2) == -1) {
            error_and_exit("Writing to task file failed");
            close(task_fd);
            return -1;
        }

        close(task_fd);

        if (rmdir(dir)) {
            error_and_exit("Failed to remove directory");
            return -1;
        }
    }

    fprintf(stderr, "done.\n");
    return 0;
}
