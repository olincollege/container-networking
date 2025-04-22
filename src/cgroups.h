#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

#define MEMORY "1073741824"   // 1GB
#define CPU_MAX "100000 100000" // 100% of 1 core
#define PIDS "64"
#define IO_WEIGHT "10"

// a struct for the different settings/procs in a cgroup
typedef struct cgrp_setting {
    char* name;
    char* value;
  } cgrp_setting;

// shouldn't be here but will be for now
  struct child_config {
    int argc;
    uid_t uid;
    int fd;
    char* hostname;
    char** argv;
    char* mount_dir;
  };

/**
 * Create cgroup file directories with appropriate settings
 *
 * @param config The configuration/values this container will have
 * @return 0 on success, -1 on failure.
 */
int resources(struct child_config* config);

/**
 * Safely delete the container made for the given child configuration
 *
 * @param config The configuration/values this container will have
 * @return 0 on success, -1 on failure.
 */
int free_resources(struct child_config* config);
