# Container Networking

Container Networking is a class project for Software Systems (Spring 2025). In this project, we built minimal containers in C that support basic networking functionality. In the current configuration, we load an Alpine image (included in the repo—feel free to swap in others), run a setup bash script to configure networking permissions, and enable communication between the created containers.

This repo contains the code to create two light-weight containers that can exchange messages using a simple echo function, based on Software Systems Assignment 08 and a [low-level container tutorial](https://blog.lizzie.io/linux-containers-in-500-loc.html#org65bbba4).

Our learning goals are to deepen our understanding of Linux containers, networking, and C systems programming, with a strong emphasis on documentation and testing.

## Requirements

- Follow these instructions to install docker. This is needed to initially build the client and server code, as our container does not have a compiler. 
   - https://docs.docker.com/engine/install/ubuntu/
- Run the following command to install the packages necessary to run a bridge network.
   - `sudo apt install bridge-utils`

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

3. **Build the scripts in Alpine**

   Our containers run Alpine Linux. To run the `run_server` and `run_client` scripts in the Alpine image, we need to build them in the Alpine environment. To do so, we use a Docker container.

   ```bash
   cd <project_root>
   docker run -it -v ./alpine/home:/home/ alpine
   ```

   This command mounts the `alpine/home` directory from your host machine to the `/home` directory in the Docker container. Once in the Docker container, you can run the following commands to build:

   ```bash
   apk add build-base cmake
   cd /home
   mkdir build
   cd build
   cmake ..
   make
   ```

   From here, you can exit out of the Docker container and the executables will be available in our custom containers.

## Usage

1. **Start both containers**

   To start the containers, run the following command in two separate terminal windows:

   ```bash
   sudo ./build/src/contained -u 0 -m ./alpine/ -c /bin/sh
   ```

   As part of the command, we specify the user ID (`-u 0`, or the root user), the path to the Alpine image (`-m ./alpine/`), and the command to run in the container (`-c /bin/sh`). This will start a shell in the container.

   One of the output lines will look like this:

   ```bash
   => [container] PID: 1032630
   ```

   Where `1032630` is the PID of the container. You will need this PID from both containers to set up the network.

2. **Run the network setup script**

   In a third terminal window, run the following command to set up the network:

   ```bash
   sudo bash ./setup.sh <SERVER_PID> <CLIENT_PID>
   ```

   Replace `<SERVER_PID>` and `<CLIENT_PID>` with the PIDs of the server and client containers, respectively. This will set up the network namespaces and create a virtual Ethernet interface for communication between the two containers.

3. **Run the server and client**

   In the first terminal window (the server), run the following command to start the server:

   ```bash
   /home/build/src/run_server
   ```

   In the second terminal window (the client), run the following command to start the client:

   ```bash
   /home/build/src/run_client
   ```

4. **Experiment with the echo functionality**

   In the client terminal, you can now send messages to the server and receive responses. Any words you type in the client terminal will be sent to the server, and the server will echo them back.

## Testing

We have included unit tests for the cgroup and namespace modules. To run the tests, you can use the following commands:

```bash
cd build
sudo ctest
```

This will run all the tests in the `test` directory. You can also run individual tests by specifying the test name.

## Repo Structure

```{bash}
container-networking/
├── alpine/                  # Alpine-related files or image config
│   └── home/src/            # Code for server/client
├── research/                # Research notes, background materials
├── src/                     # Core container code
│   ├── cgroups.c/h          # Handles cgroup setup, configuration, and cleanup
│   ├── contained.c/h        # Main logic for container lifecycle and setup
│   ├── utils.c/h            # Simple error reporting and shared include statements
│   ├── syscalls.c/h         # Wrappers for key system calls, removes some capabilities
│   ├── namespaces.c/h       # Sets up namespaces
│   └── CMakeLists.txt       # Build file for source
├── test/                    # Unit and integration tests
│   ├── test_cgroups.c       # Tests for cgroup module
│   ├── test_namespaces.c    # Tests for namespace logic
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
