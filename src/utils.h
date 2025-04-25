#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE (1024 * 1024)
#define USERNS_OFFSET 10000
#define USERNS_COUNT 2000

/**
 * Choose host name and initialize socketpair and fcntl.
 *
 * @param buff The buffer to store the generated host name.
 * @param len The length of the buffer.
 * @return 0 on success, -1 on failure.
 */
int choose_hostname(char* buff, size_t len);

/**
 * Allocate stack and clone child process.
 *
 * @param sockets The socket pair used for communication.
 * @param config The configuration for the child process.
 * @return 0 on success, -1 on failure.
 */
int allocate_stack_and_clone(int sockets[2], struct child_config* config);

/**
 * Handle child UID map.
 *
 * @param child_pid The PID of the child process.
 * @param socket The socket used for communication.
 * @return 0 on success, -1 on failure.
 */
int handle_child_uid_map(pid_t child_pid, int socket);

/**
 * Mount user namespace.
 *
 * @param config The configuration for the child process.
 * @return 0 on success, -1 on failure.
 */
int userns(struct child_config* config);

/**
 * Child function executed in new namespaces.
 *
 * @param arg Pointer to a child_config struct.
 * @return 0 on success, -1 on failure.
 */
int child(void* arg);
