#include "cgroups.h"

const int pid_buf_size = 32;
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
    // removing snprintf warnings - error checking is done manually with error_and_exit
    // NOLINTNEXTLINE
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
    // NOLINTNEXTLINE
    if (snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", dir) == -1) {
        error_and_exit("Failed to write procs directory name - too long?");
    }
    int procs_fd = open(procs_path, O_WRONLY | O_CLOEXEC);
    if (procs_fd < 0) {
        error_and_exit("Failed to open cgroup.procs");
    }
    (void)fprintf(stderr, "PID joining cgroup: %d\n", getpid()); // DEBUGGING
    // this puts the current process into this cgroup
    // I think lizzie moves the process out of the cgroup at some point into her code (mounts), so we might want to do that
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
        // NOLINTNEXTLINE
        if (snprintf(path, sizeof(path), "%s/%s", dir, (*set)->name) == -1) {
            error_and_exit("Failed to write settings file path - name too long?");
        }
        int setting_fd = open(path, O_WRONLY | O_CLOEXEC);
        if (setting_fd < 0) {
            error_and_exit("opening cgroup setting file failed");
        }
        if (write(setting_fd, (*set)->value, strlen((*set)->value)) == -1) {
            close(setting_fd);
            error_and_exit("Failed to write into settings file");
        }
        close(setting_fd);
    }
    (void)fprintf(stderr, "=> finished creating cgroups...\n");
    return 0;
}

int free_resources(struct child_config* config) {
    char dir[PATH_MAX];
    (void)fprintf(stderr, "=> freeing cgroups...\n");
    // NOLINTNEXTLINE
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s", config->hostname) == -1) {
        error_and_exit("Failed to write cgroup directory");
    }

    (void)fprintf(stderr, "=> moving process out of cgroup...\n");
    // Move process back to root cgroup
    int root_fd = open("/sys/fs/cgroup/cgroup.procs", O_WRONLY | O_CLOEXEC);
    if (root_fd < 0) {
        error_and_exit("Failed to open root cgroup.procs");
    }
    char pid_buf[pid_buf_size];
    // write current process out of container and into root cgroup
    // NOLINTNEXTLINE
    if (snprintf(pid_buf, sizeof(pid_buf), "%d", getpid()) == -1) {
        error_and_exit("Failed to write current process pid");
    }
    if (write(root_fd, pid_buf, strlen(pid_buf)) == -1) {
        close(root_fd);
        error_and_exit("Failed to move process to root cgroup");
    }
    close(root_fd);

    // Now it's safe to remove the cgroup
    (void)fprintf(stderr, "=> remove cgroup directory...\n");
    if (rmdir(dir)) {
        error_and_exit("Failed to remove cgroup directory");
    }
    return 0;
}
