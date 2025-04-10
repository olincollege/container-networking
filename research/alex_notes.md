### Linux containers in 500 lines of code<sup>1</sup>:
- namespaces - used to group kernel objects into different sets that can be accessed by specific process trees
- capabilities - used here to set some coarse limits on what uid 0 can do
  - Done through system calls
- cgroups - a mechanism to limit usage of resources like memory, disk io, and cpu-time
  - Done through file system
- setrlimit - another mechanism for limiting resource usage; older than cgroups, but can do some things cgroups can't
  - Done through system calls
- Error checking is important to understand what is going on
- Uses the `goto` statement a lot, which provides unconditional jump functionality, transferring program control to a labeled statement within the same function
  - Generally discouraged to use because of readability and it’s error prone
  - Example usage:
```c
#include <stdio.h>

int main() {
    int num;
    printf("Enter a number: ");
    scanf("%d", &num);

    if (num % 2 == 0) {
        goto even;
    } else {
        goto odd;
    }

even:
    printf("%d is even.\n", num);
    return 0;

odd:
    printf("%d is odd.\n", num);
    return 0;
}
```
- `man 7 capabilities` has details on dropping capabilities
- There’s a list of dropped capabilities which might be interesting to add to our containers? The reason they’re dropped is because they’re not namespaced in this code
- There’s definitely a lot of security stuff related to capabilities that I don’t really understand
- The person who wrote this has also blacklisted various “dangerous” system calls - more information about this is linked to a Docker source
- Cgroup vs cgroup2 - this article uses cgroup2; this could be a potential difference to look into

### Basic Container Notes
- A container is a standard unit of software that packages up code and all its dependencies so the application runs quickly and reliably from one computing environment to another<sup>2</sup>
- Namespaces isolate processes from each other<sup>3, here on</sup>
- There are different kinds of namespaces
  - User namespace
    - has its own set of user IDs and group IDs for assignment to processes. In particular, this means that a process can have root privilege within its user namespace without having it in other user namespaces
  - Process ID (PID) namespace
    - Provides isolation from processes with different namespaces
  - Network namespace
    - has an independent network stack
  - Mount namespace
    - has an independent list of mount points seen by the processes in the namespace. This means that you can mount and unmount filesystems in a mount namespace without affecting the host filesystem
  - Interprocess Communication (IPC) namespace
  - UNIX Time‑Sharing (UTS) namespace
- A control group (cgroup) is a Linux kernel feature that limits, accounts for, and isolates the resource usage (CPU, memory, disk I/O, network, and so on) of a collection of processes
- The following lines of code create a v1 cgroup called `foo` and sets the memory limit for it to 50,000,000 bytes (50 MB)
  - `root # mkdir -p /sys/fs/cgroup/memory/foo`
  - `root # echo 50000000 > /sys/fs/cgroup/memory/foo/memory.limit_in_bytes`
- This resource<sup>3</sup> has good examples of using cgroups

### cgroup Notes<sup>4</sup>
- A *hierarchy* is a set of cgroups arranged in a tree - every task in the system is in exactly one of the cgroups in the hierarchy
- Process aggregations require the basic notion of a grouping/partitioning of processes, with newly forked processes ending up in the same group (cgroup) as their parent process
- Each cgroup is represented by a directory in the cgroup file system containing the following files describing that cgroup:
  - tasks: list of tasks (by PID) attached to that cgroup.  This list is not guaranteed to be sorted.  Writing a thread ID into this file moves the thread into this cgroup.
  - cgroup.procs: list of thread group IDs in the cgroup.  This list is not guaranteed to be sorted or free of duplicate TGIDs, and userspace should sort/uniquify the list if this property is required. Writing a thread group ID into this file moves all threads in that group into this cgroup.
  - notify_on_release flag: run the release agent on exit?
  - release_agent: the path to use for release notifications (this file exists in the top cgroup only)
Additional files can be added

### Linux Containers in 500 lines of code (again)
- The article has something along the lines of `child_pid = clone(child_func, stack + STACK_SIZE, flags, argv + 1);`
  - `clone` is like `fork`, but has some more parameters
    - Sets up the container (new namespaces)
  - `child_func` sets up the container process and runs a command (`argv+1`)
    - That command can come from terminal (`ls`) for example
  - `stack + STACK_SIZE` gives the child its own stack space
  The `handle_child_uid_map` function has the child tell its parent if it is using a namespace
  - If the child is using a namespace, the parent writes to uid and gid maps in the child
  - I believe this means: the parent has its own UIDs that it associates with different processes, which are different from the processes the child associates with it because the child has created its own container/ecosystem
  - This function creates a map between those discrepancies so everything is on the same page of what is happening
  - This allows the child to pretend to be root within the container
- The `usern` function is the child-side function that writes if it is using a namespace
  - I believe it also reads from the parent the new uid/gid mappings
- `mount` is used to attach (or mount) a file system to a specified directory<sup>5</sup>
- cgroups have their own namespaces, as do mounts
- We need to create the cgroup before we enter a cgroup namespace
- Once we do, that cgroup will behave like the root cgroup inside of the namespace



### Resources:
- https://blog.lizzie.io/linux-containers-in-500-loc.html#org65bbba4 
- https://www.docker.com/resources/what-container/ 
- https://blog.nginx.org/blog/what-are-namespaces-cgroups-how-do-they-work 
- https://www.kernel.org/doc/Documentation/cgroup-v1/cgroups.txt 
- https://medium.com/javarevisited/what-is-mount-and-how-does-it-work-6fdd5e19967d 
