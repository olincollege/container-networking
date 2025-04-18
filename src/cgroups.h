#pragma once

#include "utils.h"

#define MEMORY "1073741824"
#define SHARES "256"
#define PIDS "64"
#define WEIGHT "10"
#define FD_COUNT 64

// From alex's code lines 187-213

/**
 * Represents a single cgroup setting (name-value pair).
 */
typedef struct cgrp_setting {
  char* name;
  char* value;
} cgrp_setting;

/**
 * Represents a cgroup controller (e.g., memory, cpu) with its settings.
 */
typedef struct cgrp_control {
  char* control;
  cgrp_setting* settings[];
} cgrp_control;

// --- EXTERNAL CONSTANTS ---

extern const cgrp_setting add_to_tasks;
extern const cgrp_setting mem_limit;
extern const cgrp_setting kmem_limit;
extern const cgrp_setting cpu_shares;
extern const cgrp_setting pids_max;
extern const cgrp_setting blkio_weight;

extern cgrp_control memory_cgroup;
extern cgrp_control cpu_cgroup;
extern cgrp_control pids_cgroup;
extern cgrp_control blkio_cgroup;
extern cgrp_control* cgrps[];

// functions

/**
 * Set up resource cgroups and file descriptor limits.
 *
 * @param config Pointer to a child_config struct.
 * @return 0 on success, 1 on failure.
 */
int resources(struct child_config* config);

/**
 * Clean up and remove all created cgroup directories.
 *
 * @param config Pointer to a child_config struct.
 * @return 0 on success, -1 on failure.
 */
int free_resources(struct child_config* config);
