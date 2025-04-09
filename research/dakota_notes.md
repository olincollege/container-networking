### Imports

| Header | Purpose | Relevance in Containers |
| :---- | :---- | :---- |
| `#define _GNU_SOURCE` | Enables GNU extensions for broader syscall access | Required for functions like `clone()` and advanced Linux features |
| `<stdio.h>` | Standard I/O functions | General-purpose C programming |
| `<stdlib.h>` | Memory allocation, process control | Used in most C programs (e.g., `malloc`, `exit`, `system`) |
| `<string.h>` | String manipulation | Common for parsing, command-line building, etc. |
| `<errno.h>` | Error handling | Helpful for debugging syscall errors |
| `<time.h>` | Time-related functions | For logging, timeout handling |
| `<unistd.h>` | POSIX system calls | Used for `fork()`, `exec()`, `getuid()`, `setuid()`, `chroot()` |
| `<pwd.h>` | Access user info (`getpwnam`, etc.) | Switching to unprivileged users inside containers |
| `<grp.h>` | Access group info (`getgrnam`, etc.) | Same as above, for group management |
| `<fcntl.h>` | File control options (`open`, `O_*` flags) | Managing file descriptors in isolated FS environments |
| `<sys/stat.h>` | File status info (`stat`, `chmod`) | Setting file permissions in container FS |
| `<linux/limits.h>` | System limits (e.g., `PATH_MAX`) | Useful when creating or checking paths inside container |
| `<sys/mount.h>` | Filesystem mounting (`mount`, `umount`) | For setting up container root FS, bind mounts, etc. |
| `<sched.h>` | Process/thread scheduling and `clone()` | Used for creating namespaces (e.g., `CLONE_NEWPID`, `CLONE_NEWNS`) |
| `<sys/utsname.h>` | Hostname/system info | Set container-specific hostname |
| `<sys/prctl.h>` | Process-level controls | Used for enabling seccomp, setting dumpable flags, etc. |
| `<seccomp.h>` | Define and apply syscall filters | Key to reducing attack surface by restricting syscalls |
| `<sys/capability.h>` | Manage POSIX capabilities | Drop/retain specific privileges instead of all-or-nothing `root` |
| `<linux/capability.h>` | Linux-specific capabilities definitions | Often used with `<sys/capability.h>` |
| `<sys/resource.h>` | Resource limits (`setrlimit`) | Restrict CPU, memory, file usage |
| `<sys/socket.h>` | Socket APIs | For custom networking or IPC |
| `<sys/syscall.h>` | Make raw syscalls | Needed for unwrapped syscalls, especially in seccomp profiles |
| `<sys/wait.h>` | Wait for process state changes | Used when managing child processes (e.g., PID 1 in container) |

### General  
Name spaces can be separated by content or user

Content

- Video, Documents, Etc.  
- Find by type

Users

- Allow for multi-tenancy  
- Keeps user data separated

Name spaces are like folders in a file system \- they let you separate your contents in a database

### Actual Process

```{shell} 
	if ([socketpair](https://man7.org/linux/man-pages/man2/socketpair.2.html)(AF\_LOCAL, SOCK\_SEQPACKET, 0, sockets)) {  
		fprintf(stderr, "socketpair failed: %m\\n");  
		goto error;  
	}  
	if ([fcntl](https://man7.org/linux/man-pages/man2/fcntl.2.html)(sockets\[0\], F\_SETFD, FD\_CLOEXEC)) {  
		fprintf(stderr, "fcntl failed: %m\\n");  
		goto error;  
	}  
	config.fd \= sockets\[1\];
```

socketpair \- create a pair of connected sockets  
fcntl \- manipulate file descriptor

```{shell}
	\#define STACK\_SIZE (1024 \* 1024\)

	char \*stack \= 0;  
	if (\!(stack \= malloc(STACK\_SIZE))) {  
		fprintf(stderr, "=\> malloc failed, out of memory?\\n");  
		goto error;  
	}
```

Assign temporary stack space for the namespace to store things in when cloning.

```{shell}

	int flags \= CLONE\_NEWNS  
		| CLONE\_NEWCGROUP  
		| CLONE\_NEWPID  
		| CLONE\_NEWIPC  
		| CLONE\_NEWNET  
		| CLONE\_NEWUTS;
```

Define which Linux namespaces you want the child process (i.e., the container) to be isolated into

```{shell{
	if ((child\_pid \= clone(child, stack \+ STACK\_SIZE, flags | SIGCHLD, \&config)) \== \-1) {  
		fprintf(stderr, "=\> clone failed\! %m\\n");  
		err \= 1;  
		goto clear\_resources;  
	}
``` 
Clones it
