# Container Networking

Container Networking is a class project for Software Systems (Spring 2025). In this project, we built minimal containers in C that support basic networking functionality. In the current configuration, we load an Alpine image (included in the repo—feel free to swap in others), run a setup bash script to configure networking permissions, and enable communication between the created containers.

This repo contains the code to create two lightweight containers that can exchange messages using a simple echo function, based on assignment 08 and a [low-level container tutorial](https://blog.lizzie.io/linux-containers-in-500-loc.html#org65bbba4).

Our learning goals are to deepen our understanding of Linux containers, networking, and C systems programming, with a strong emphasis on documentation and testing.


## Installation

1. **Clone the repo**

   ```bash
   git clone https://github.com/olincollege/container-networking.git
   cd container-networking
   ```

2. **Build the project**
   We use CMake for building:

   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

## Usage

TODO


## Repo Structure

```{bash}
container-networking/
├── alpine/                  # Alpine-related files or image config
├── research/                # Research notes, background materials
├── src/                     # Core container code
│   ├── cgroups.c/h          # Handles cgroup setup, configuration, and cleanup
│   ├── contained.c/h        # Main logic for container lifecycle and setup
│   ├── error_handling.c/h   # Simple error reporting (`error_and_exit`)
│   ├── syscalls.c/h         # Wrappers for key system calls (mount, setuid)
│   ├── utils.c/h            # Sets up namespaces
│   └── CMakeLists.txt       # Build file for source
├── test/                    # Unit and integration tests
│   ├── test_cgroups.c       # Tests for cgroup module
│   ├── test_utils.c         # Tests for utility and namespace logic
│   └── CMakeLists.txt       # Build file for tests
├── .clang-format            # Code style configuration
├── .clang-tidy              # Linting rules
├── .editorconfig            # Editor consistency rules
├── .gitignore               # Git ignore rules
├── CMakeLists.txt           # Top-level CMake build configuration
├── README.md                # Project documentation (you are here)
└── setup.sh                 # Bash script to configure network and permissions in Alpine
```

## Authors
- @amgeorge22
- @EarlJr53
- @dakotacsk

## License

[MIT](https://choosealicense.com/licenses/mit/)
