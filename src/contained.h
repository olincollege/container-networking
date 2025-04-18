#pragma once

#include "utils.h"

/**
 * Mount namespace setup and pivot_root logic.
 *
 * @param config Pointer to a child_config struct.
 * @return 0 on success, -1 on failure.
 */
int mounts(struct child_config* config);
