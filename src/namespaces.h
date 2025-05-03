#pragma once

#include "cgroups.h"   // for resources()
#include "syscalls.h"  // for syscalls() and capabilities()

#define STACK_SIZE ((size_t)(1024 * 1024))
#define USERNS_OFFSET 10000
#define USERNS_COUNT 2000

/**
 * This function generates a pseudo-random container hostname by combining a
 * timestamp with a randomly selected name. This is not necessary but a fun
 * feature the original author wrote that we decided to keep.
 *
 * @param buff The buffer to store the generated host name.
 * @param len The length of the buffer.
 * @return 0 on success, -1 on failure.
 */
int choose_hostname(char* buff, size_t len);

/**
 * This function sets up a new stack for the child process and uses `clone()`
 * with Linux namespace flags (PID, NET, IPC, MNT, CGROUP, UTS) to isolate the
 * container. The child process will later execute the `child()` function.
 * - PID namespace: Isolates process IDs.
 * - NET namespace: Isolates network stack.
 * - IPC namespace: Isolates inter-process communication.
 * - MNT namespace: Isolates filesystem mount points.
 * - CGROUP namespace: Isolates control groups.
 * - UTS namespace: Isolates hostname and domain name.
 *
 * @param sockets The socket pair used for communication.
 * @param config The configuration for the child process.
 * @return 0 on success, -1 on failure.
 */
int allocate_stack_and_clone(int sockets[2], struct child_config* config);

/**
 * This function writes appropriate mappings to `/proc/[pid]/uid_map` and
 * `/proc/[pid]/gid_map` files so the containerized process can drop privileges
 * safely while appearing as root (uid 0) inside the container.
 *
 * @param child_pid The PID of the child process.
 * @param socket The socket used for communication.
 * @return 0 on success, -1 on failure.
 */
int handle_child_uid_map(pid_t child_pid, int socket);

/**
 * This function calls `unshare()` to create a new user namespace. It also
 * notifies the parent process to write UID/GID mappings and then switches to
 * the mapped user ID. This lets the container to operate with root-like
 * privileges inside the container while being unprivileged outside.
 *
 * @param config The configuration for the child process.
 * @return 0 on success, -1 on failure.
 */
int userns(struct child_config* config);

/**
 * This function runs inside the new namespaces and applies the container configuration:
 * - Sets hostname (UTS namespace)
 * - Mounts isolated filesystem (MNT namespace)
 * - Initializes user namespace (USER)
 * - Drops capabilities and installs seccomp filters
 * - Finally executes the target binary via `execve`
 *
 * @param arg Pointer to a child_config struct.
 * @return 0 on success, -1 on failure.
 */
int child(void* arg);
