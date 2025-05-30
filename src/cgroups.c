#include "cgroups.h"

const int pid_buf_size = 32;

int resources(struct child_config* config) {
    // create relevant settings
    cgrp_setting mem_limit = { .name = "memory.max", .value = MEMORY };
    cgrp_setting cpu_limit = { .name = "cpu.max", .value = CPU_MAX };
    cgrp_setting pids_max  = { .name = "pids.max", .value = PIDS };
    cgrp_setting io_weight = { .name = "io.weight", .value = IO_WEIGHT };

    (void)fprintf(stderr, "=> creating cgroup directory...\n");
    char dir[PATH_MAX] = {0};
    // removing snprintf warnings - error checking is done manually with error handling
    // NOLINTNEXTLINE
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s", config->hostname) == -1) {
        perror("Failed to write cgroup directory name - too long?");
        return -1;
    }
    // rwx for owner, r-x for groups and others
    const mode_t perms = 0755;
    if (mkdir(dir, perms) && errno != EEXIST) { // make directory - doesn't error if it already exists
        perror("mkdir failed");
        return -1;
    } 

    // join current process to the new cgroup
    (void)fprintf(stderr, "=> creating process directory...\n");
    char procs_path[PATH_MAX];
    // NOLINTNEXTLINE
    if (snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", dir) == -1) {
        perror("Failed to write procs directory name - too long?");
        return -1;
    }
    int procs_fd = open(procs_path, O_WRONLY | O_CLOEXEC);
    if (procs_fd < 0) {
        perror("Failed to open cgroup.procs");
        return -1;
    }
    (void)fprintf(stderr, "=> PID joining cgroup: %d\n", getpid()); // DEBUGGING
    // this puts the current process into this cgroup
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
            perror("Failed to write settings file path - name too long?");
            return -1;
        }
        int setting_fd = open(path, O_WRONLY | O_CLOEXEC);
        if (setting_fd < 0) {
            perror("opening cgroup setting file failed");
            return -1;
        }
        if (write(setting_fd, (*set)->value, strlen((*set)->value)) == -1) {
            close(setting_fd);
            perror("Failed to write into settings file");
            return -1;
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
        perror("Failed to write cgroup directory");
        return -1;
    }

    (void)fprintf(stderr, "=> moving process out of cgroup...\n");
    // Move process back to root cgroup
    int root_fd = open("/sys/fs/cgroup/cgroup.procs", O_WRONLY | O_CLOEXEC);
    if (root_fd < 0) {
        perror("Failed to open root cgroup.procs");
        return -1;
    }
    char pid_buf[pid_buf_size];
    // write current process out of container and into root cgroup
    // NOLINTNEXTLINE
    if (snprintf(pid_buf, sizeof(pid_buf), "%d", getpid()) == -1) {
        perror("Failed to write current process pid");
        return -1;
    }
    if (write(root_fd, pid_buf, strlen(pid_buf)) == -1) {
        close(root_fd);
        perror("Failed to move process to root cgroup");
        return -1;
    }
    close(root_fd);

    // remove cgroup now that it's empty
    (void)fprintf(stderr, "=> remove cgroup directory...\n");
    if (rmdir(dir)) {
        perror("Failed to remove cgroup directory");
        return -1;
    }
    (void)fprintf(stderr, "=> finished removing cgroup...\n");
    return 0;
}


int mounts(struct child_config* config) {
    (void)fprintf(stderr, "=> Making / MS_PRIVATE recursively...\n");
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) {
        perror("mount MS_PRIVATE");
        return -1;
    }

    // Create a temporary mount point
    char new_root_template[] = "/tmp/container_root.XXXXXX";
    char* new_root = mkdtemp(new_root_template);
    if (!new_root) {
        perror("mkdtemp new_root");
        return -1;
    }

    // Bind mount the desired new root to this temp location
    if (mount(config->mount_dir, new_root, NULL, MS_BIND, NULL) < 0) {
        perror("bind mount config->mount_dir to new_root");
        return -1;
    }

    // Create a directory inside new_root for the old root to live during pivot_root
    char old_root[PATH_MAX];
    // disable lint because we manually error check
    //NOLINTNEXTLINE
    if(snprintf(old_root, sizeof(old_root), "%s/old_root", new_root) == -1) {
        perror("couldn't write file name - too long?");
        return -1;
    }
    const mode_t perms = 0777;
    if (mkdir(old_root, perms) < 0) {
        perror("mkdir old_root inside new_root");
        return -1;
    }

    (void)fprintf(stderr, "=> pivot_root(%s, %s)...\n", new_root, old_root);
    if (syscall(SYS_pivot_root, new_root, old_root) < 0) {
        perror("pivot_root failed");
        return -1;
    }

    // Change to new root
    if (chdir("/") < 0) {
        perror("chdir to new root");
        return -1;
    }

    // Unmount and remove the old root
    if (umount2("/old_root", MNT_DETACH) < 0) {
        perror("umount2 old_root");
        return -1;
    }

    if (rmdir("/old_root") < 0) {
        perror("rmdir old_root");
        return -1;
    }

    (void)fprintf(stderr, "=> Mount setup complete.\n");
    return 0;
}
