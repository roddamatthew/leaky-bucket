# `leaky-bucket`

An implementation of a Docker-like container for learning! This repo focusses on recreating the process-isolation security features of containers for security research and container escapes.

## Chapters:

Each chapter of the implementation is briefly described below. Each builds on the last, adding more security features and greater isolation.

### `01-skeleton`:

Implements that basic interface for spawning a child process and passing your commands to it.

### `02-namespace`:

Adds basic isolation through namespaces! Check out your:
- User with `whoami`
- Hostname with `hostname`
- Mounts with `mount`
- Processes with `ps aux`
- Filesystem with `ls`

### `03-seccomp`:

Adds a `seccomp` deny list for filtering system calls. Many of the blocked system calls in Docker are somewhat of a pain to demonstrate, so the only blocked system call here is `ptrace` which can be shown by calling `strace`.

### `04-cgroup`:

Adds a `cgroup` to stop the container from being able to DoS the host. Try calling the forkbomb program in the container and see that the host is unaffected! The `cgroup` limits 

## Cloning a FS

The `rootfs` directory needs to be created from a Docker image (or just use the provided `rootfs.tar` if you're a trusting individual)

```
docker pull alpine
docker create --name demo-rootfs alpine
docker run --name demo-rootfs alpine sh -c 'apk add --no-cache strace'
docker export demo-rootfs -o rootfs.tar
mkdir rootfs
tar -xf rootfs.tar -C rootfs
docker rm demo-rootfs
```

## References:

- Containers From Scratch, Liz Rice 2018