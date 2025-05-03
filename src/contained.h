#pragma once

#include "cgroups.h"
#include "namespaces.h"
#include "syscalls.h"


#define MAJOR_VERSION 6
#define MINOR_VERSION 8

/**
 * Cleanup function is automatically called when `on_exit()` is called.
 *
 * This function closes open sockets and prints a debug message.
 * It is used to ensure that socket descriptors are properly released
 * when the program exits, even if an error occurs.
 *
 * @param status The exit status of the program.
 * @param arg A pointer to the socket array used for communication.
 */
void cleanup(int status, void* arg);

/**
 * Wait for the child container process to finish and update error status.
 *
 * This function calls `waitpid()` on the given child PID and writes its
 * exit status to the `err` flag provided by the caller.
 *
 * @param child_pid The PID of the child process to wait for.
 * @param err Pointer to an error code flag updated with the child’s exit
 * status.
 */
void finish_child(int child_pid, unsigned int* err);

/**
 * Free container resources after the child process exits.
 *
 * @param config Pointer to the child configuration struct.
 * @param stack Pointer to the dynamically allocated stack memory.
 */
void clear_resources(struct child_config* config, char* stack);

/**
 * Print usage instructions and exit the program.
 *
 * @param path The name of the binary (usually `argv[0]`).
 */
void usage(char* path);

/**
 * Main entry point for the container launcher.
 *
 * This function parses command-line arguments, validates the host system,
 * generates a random hostname, allocates container resources, and uses
 * `clone()` to start a containerized process with namespace isolation.
 *
 * On failure, it prints errors and ensures all resources are properly freed.
 *
 * @param argc The number of arguments.
 * @param argv The argument vector.
 * @return Exit status code: 0 on success, non-zero on failure.
 */
int main(int argc, char** argv);
