#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/sched.h>

#define STACK_SIZE (1024 * 1024)

struct child_task_args_t {
    int pipefd[2];
    char **cmd;
};

void pprint_cmd(char **cmd, int count)
{
    printf("running [");
    for (int i = 0; i < count; i++) {
        printf("%s", cmd[i]);
        if (i < count - 1) printf(" ");
    }
    printf("]\n");
}

int child_task(void *arg)
{
    struct child_task_args_t *args = (struct child_task_args_t*)arg;
    int *pipefd = args->pipefd;
    char **cmd = args->cmd;

    close(pipefd[0]); // Child doesn't need to read

    // In the child set and get the hostname
    char buf[256] = {0};
    char hostname[] = "leaky-bucket";
    sethostname(hostname, sizeof(hostname));
    gethostname(buf, sizeof(buf));

    pid_t child_pid = getpid();
    dprintf(pipefd[1], "Im a child, with PID=%d\n", child_pid);
    dprintf(pipefd[1], "And my hostname is %s\n", buf);
    close(pipefd[1]);
    // execvp(cmd[0], cmd);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3 || strcmp(argv[1], "run") != 0) {
        printf("Expected to be called like %s run {your-cmd}\n", argv[0]);
        exit(1);
    }
    
    // Extract the command
    char **cmd = argv + 2;
    pprint_cmd(cmd, argc - 2);

    // Print hostname for checking against namespace
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        perror("gethostname");
        exit(1);
    }
    printf("parent hostname: %s\n", hostname);

    // Make a stack for the child to start execution
    char *child_stack = malloc(STACK_SIZE);
    if (!child_stack) {
        perror("malloc");
        exit(1);
    }

    // Make a pipe to get its output
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    // Pack arguments into struct
    struct child_task_args_t args = {
        .pipefd = {pipefd[0], pipefd[1]},
        .cmd = cmd
    };

    // Create a new process with its own hostname namespace
    int flags = CLONE_NEWUTS | SIGCHLD ;
    pid_t child_pid = clone(child_task, child_stack + STACK_SIZE, flags, &args);
    if (child_pid == -1) {
        perror("clone");
        free(child_stack);
        exit(1);
    }

    // Parent continues
    if (child_pid > 0) {
        // Read from pipe
        close(pipefd[1]); // parent doesn't need to write
        char buf[4096];
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, n);
        }
        close(pipefd[0]);

        int status;
        if (waitpid(child_pid, &status, 0) == -1) {
            perror("waitpid");
            exit(1);
        }
    }

    // Get the hostname again and it should be the same!
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        perror("gethostname");
        exit(1);
    }
    printf("parent hostname is still: %s\n", hostname);

    return 0;
}
