#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <seccomp.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <linux/sched.h>
#include <errno.h>

#include "utils.h"

int start_container(void *arg)
{
    // Unpack argument and setup IO
    container_args_t *args = (container_args_t*)arg;
    notify_parent(args->send_meta, args->recv_meta); // Notify parent to do UID mapping
    contain_container(); // Change container resources
    print_summary();

    // Add a deny list with seccomp
    scmp_filter_ctx ctx;
    ctx = seccomp_init(SCMP_ACT_ALLOW); // Default allow
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(ptrace), 0); // Block ptrace
    seccomp_load(ctx); // Install the filter
    seccomp_release(ctx); // Release context

    // Do the container task!
    setup_child_io(args->data);
    execvp(args->cmd[0], args->cmd);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3 || strcmp(argv[1], "run") != 0) {
        printf("[!] Expected to be called like %s run {your-cmd}\n", argv[0]);
        exit(1);
    }
    
    // Extract the command
    char **cmd = argv + 2;
    pprint_cmd(cmd, argc - 2);

    // Summarise parent to compare w/ container
    print_summary();

    // Make a stack for the child to start execution
    char *container_stack = malloc(STACK_SIZE);
    if (!container_stack) {
        perror("malloc");
        exit(1);
    }
    // ptr must point to top of the stack
    container_stack += STACK_SIZE;

    // Make pipes for bidirectional comms
    int pipes[3][2];
    for (int i = 0; i < 3; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    // Pack arguments for container
    container_args_t args = {
        .data = {pipes[0][0], pipes[0][1]},
        .send_meta = {pipes[1][0], pipes[1][1]},
        .recv_meta = {pipes[2][0], pipes[2][1]},
        .cmd = cmd
    };

    // Create a new process with its own hostname namespace
    int flags = CLONE_NEWNS | CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD;
    pid_t container_pid = clone(start_container, container_stack, flags, &args);
    if (container_pid == -1) {
        perror("clone");
        free(container_stack);
        exit(1);
    }

    // Parent continues
    if (container_pid > 0) {
        // Wait for container creation to write UID mapping 
        char ready = 'n';
        printf("parent waiting for args.send_meta read end\n");
        if (read(args.send_meta[0], &ready, 1) > 0) {
            if (ready == 'y') {
                printf("parent creating uid mapping\n");
                create_uid_mapping(container_pid, args.recv_meta);
            } else {
                printf("[!] Received unexpected metadata from container: %c\n", ready);
                exit(1);
            }
        } else {
            perror("read meta");
            exit(1);
        }

        // Can now close the metadata pipes
        for (int i = 0; i < 2; i++) {
            close(args.send_meta[i]);
            close(args.recv_meta[i]);
        }

        // Read from pipe
        close(args.data[1]); // parent doesn't need to write
        char buf[4096];
        ssize_t n;
        while ((n = read(args.data[0], buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, n);
        }
        close(args.data[0]);

        int status;
        if (waitpid(container_pid, &status, 0) == -1) {
            perror("waitpid");
            exit(1);
        }
        printf("Container exited gracefully.\n");
    }

    return 0;
}
