# Recreational OS Project
Intended for teaching myself OS implementation and 64-bit ARM architecture.

## Features
- Preemptive scheduling of kernel and user tasks
- Virtual filesystem API with support for multiple filesystem implementations
- RAM filesystem implementation
- Isolated user processes
- POSIX-like syscall API
    - File operations: open, read, write, close
    - Process management: fork, exec, getpid, sleep
- SMP support
- Semaphores and spinlocks in the kernel
- Virtual memory with two-level page table hierarchy implemented
- Interrupts with GICv2
- Minimal 'systemless' C library for usage in kernel and user space
- Simple object pool allocator for paging, kernel objects and allocating memory to user processes
- pl011 driver for UART
- Command-line interface as kernel process 

## Limitations
- Targets only aarch64 raspi4b
- Only tested in QEMU
- Not designed to be very portable
- Memory layout is quite static

## How to build and run
### Requirements
- AArch64 cross compiler (e.g. `aarch64-none-elf-gcc`) in PATH
- AArch64 QEMU system emulator `qemu-system-aarch64` in PATH, tested with versions 10 and 11
- CMake 3.28+

OR


- Docker/podman

#### Building the container image
If using Podman without Docker CLI emulation, replace `docker` with `podman` in the commands.

In repository root, run:
```bash
docker build -t laos-dev --build-arg USER_UID=$(id -u) --build-arg USER_GID=$(id -g) --build-arg USER_NAME=$(id -un) .
```

#### Launching the container
To get a shell in the container with your user and the repository mounted, run in repository root:
```bash
docker run --rm -it --userns=keep-id -v $(pwd):/home/$(id -un)/LaOS laos-dev bash
```
If using SELinux (such as Fedora), you may need to add `:z` to the volume mount option:
```bash
docker run --rm -it --userns=keep-id -v $(pwd):/home/$(id -un)/LaOS:z laos-dev bash
```

Then repository root will be at `/home/<username>/LaOS` in the container.
```bash
cd /home/$(id -un)/LaOS
```

### Building
Inside the container or host with the required tools installed, in repository root, run:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-toolchain.cmake
cmake --build build
```

### Running
Launch QEMU with the built kernel:
```bash
./run.sh
```

Output should look something like this:
```
[fs]      RamFS size: 0x400440
[fs]      Mounted RamFS at /
[fs]      Created initial RamFS directories
[fs]      Created init process file /sbin/init in RamFS
[general] Created init process with PID 1
[general] Primary CPU0 started
[general] Secondary CPU1 starting up...
[general] Secondary CPU3 starting up...
[general] Secondary CPU2 starting up...
Welcome to LaOS
/ # Hello world!
Hello from init process! PID: 1
Created file /testfile with fd: 0
Wrote 'Hello world!
' (14 bytes) to file /testfile
Closed file /testfile, ret=0
Re-opened file /testfile as read-only with fd: 0
Read 14 bytes from file /testfile: 'Hello world!
'
Closed file /testfile, ret=0

/ # 
```

### Available commands in the kernel CLI
- `cd <path>` - Change current directory
- `ls` - List files in current directory
- `touch <path>` - Create file
- `mkdir <path>` - Create directory
- `rm <path>` - Remove file or directory
- `cat <path>` - Print file contents to console

Note: this shell runs in kernel mode, user-space shell is WIP
