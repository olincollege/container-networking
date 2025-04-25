//NOLINTNEXTLINE
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <libgen.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h> 
#include <unistd.h>

#include <linux/limits.h>

#include "utils.h"

#define MEMORY "1073741824"   // 1GB
#define CPU_MAX "100000 100000" // 100% of 1 core
#define PIDS "64"
#define IO_WEIGHT "10"

// a struct for the different settings/procs in a cgroup
typedef struct cgrp_setting {
    char* name;
    char* value;
  } cgrp_setting;

/**
 * Create cgroup file directories with appropriate settings
 *
 * @param config The configuration/values this container will have
 * @return 0 on success, exits on failure.
 */
int resources(struct child_config* config);

/**
 * Safely delete the container made for the given child configuration
 *
 * @param config The configuration/values this container will have
 * @return 0 on success, exits on failure.
 */
int free_resources(struct child_config* config);

/**
 * Bind the mount directory in config and make it the new root directory of 
 * the container
 * 
 * @param config Pointer to a struct containing the path to the desired new 
 * root and other container info
 * @return 0 on success, exits on failure.
 */
int mounts(struct child_config* config);
