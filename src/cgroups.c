#include "cgroups.h"

void error_and_exit(const char* error_msg) {
    perror(error_msg);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

int resources(struct child_config* config) {
    // create relevant settings
    cgrp_setting mem_limit = { .name = "memory.max", .value = MEMORY };
    cgrp_setting cpu_limit = { .name = "cpu.max", .value = CPU_MAX };
    cgrp_setting pids_max  = { .name = "pids.max", .value = PIDS };
    cgrp_setting io_weight = { .name = "io.weight", .value = IO_WEIGHT };

    (void)fprintf(stderr, "=> creating cgroup directory...\n");
    char dir[PATH_MAX] = {0};
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s", config->hostname) == -1) {
        error_and_exit("Failed to write cgroup directory name - too long?");
    }
    // rwx for owner, r-x for groups and others
    const mode_t perms = 0755;
    if (mkdir(dir, perms) && errno != EEXIST) { // make directory - doesn't error if it already exists
        error_and_exit("mkdir failed");
    } 

    // join current process to the new cgroup
    (void)fprintf(stderr, "=> creating process directory...\n");
    char procs_path[PATH_MAX];
    if (snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", dir) == -1) {
        error_and_exit("Failed to write procs directory name - too long?");
    }
    int procs_fd = open(procs_path, O_WRONLY | O_CLOEXEC);
    if (procs_fd < 0) {
        error_and_exit("Failed to open cgroup.procs");
    }
    dprintf(procs_fd, "%d", getpid());
    close(procs_fd);

    (void)fprintf(stderr, "=> creating setting files...\n");
    cgrp_setting* settings[] = {
        &mem_limit,
        &cpu_limit,
        &pids_max,
        &io_weight,
        NULL
    };

    // for each setting, write value to corresponding file
    for (cgrp_setting** set = settings; *set; set++) {
        char path[PATH_MAX];
        if (snprintf(path, sizeof(path), "%s/%s", dir, (*set)->name) == -1) {
        error_and_exit("Failed to write settings file path - name too long?");
        }
        int setting_fd = open(path, O_WRONLY | O_CLOEXEC);
        if (setting_fd < 0) {
        error_and_exit("opening cgroup setting file failed");
        }
        write(setting_fd, (*set)->value, strlen((*set)->value));
        close(setting_fd);
    }
    (void)fprintf(stderr, "=> finished creating cgroups...\n");
    return 0;
}

int free_resources(struct child_config* config) {
    char dir[PATH_MAX];
    char procs[PATH_MAX];
    (void)fprintf(stderr, "=> freeing cgroups...\n");
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s", config->hostname) == -1 ||
        snprintf(procs, sizeof(procs), "%s/cgroup.procs", dir) == -1) {
        error_and_exit("Failed to write cgroup directory or procs path");
    }

    (void)fprintf(stderr, "=> clean up cgroup.procs...\n");
    // Move all processes out of the cgroup
    int procs_fd = open(procs, O_WRONLY | O_CLOEXEC);
    if (procs_fd < 0) {
        error_and_exit("Failed to open cgroup.procs for cleanup");
    }
    // writing 0 moves processes to root/parent directory
    if (write(procs_fd, "0", 1) == -1) {
        close(procs_fd);
        error_and_exit("Failed to write '0' to cgroup.procs to remove processes");
    }
    close(procs_fd);

    (void)fprintf(stderr, "=> remove cgroup directory...\n");
    // Now we can safely remove the cgroup directory
    if (rmdir(dir)) {
        error_and_exit("Failed to remove cgroup directory");
    }

    return 0;
}  
