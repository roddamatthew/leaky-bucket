# `leaky-bucket`

An implementation of a Docker-like container for learning! This repo focusses on recreating the process-isolation security features of containers for security research and container escapes.

## Chapters:

Each chapter of the implementation is briefly described below. Each builds on the last, adding more security features and greater isolation.

### `01-skeleton`:

Implements that basic interface for spawning a child process and passing your commands to it.

### `02-namespace`:

Adds basic isolation through namespaces. Checkout your:
- Hostname with `hostname`
- Mounts with `mount`
- 

## Cloning a FS

The `rootfs` directory needs to be created from a Docker image:

```
docker pull alpine
docker create --name demo-rootfs alpine
docker export demo-rootfs -o rootfs.tar
mkdir rootfs
tar -xf rootfs.tar -C rootfs
docker rm demo-rootfs
```

## References:

- Containers From Scratch, Liz Rice 2018