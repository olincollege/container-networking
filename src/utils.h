// function 1: choose host name and initialize socketpair and fcntl
// function 2: allocate stack and clone child process
// function 3: handle child uid map
// function 4: user namespace
// function 5: mount namespace  

#pragma once

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * Choose host name and initialize socketpair and fcntl
 *
 *
 * @param buff The buffer to store the generated host name.
 * @param len The length of the buffer.
 * @return 0 on success, -1 on failure.
 */
int choose_hostname(char* buff, size_t len);

/**
 * Allocate stack and clone child process
 *
 * @param sockets The socket pair used for communication.
 * @param config The configuration for the child process.
 * @return 0 on success, -1 on failure.
 */
int allocate_stack_and_clone(int sockets[2], struct child_config* config);

/**
 * Handle child uid map
 *
 * @param child_pid The PID of the child process.
 * @param socket The socket used for communication.
 * @return 0 on success, -1 on failure.
 */
int handle_child_uid_map(pid_t child_pid, int socket);

/**
 * Mount user namespace
 *
 * @param config The configuration for the child process.
 * @return 0 on success, -1 on failure.
 */
int userns(struct child_config* config);
