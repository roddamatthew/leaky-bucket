#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/sched.h>
#include <sched.h>
#include <sys/syscall.h>

void pprint_cmd(char **cmd, int count)
{
    printf("running [");
    for (int i = 0; i < count; i++) {
        printf("%s", cmd[i]);
        if (i < count - 1) printf(" ");
    }
    printf("]\n");
}

int child_task(char **cmd)
{
    pid_t child_pid = getpid();
    printf("Im a child, with PID=%d\n", child_pid);
    execvp(cmd[0], cmd);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3 || strcmp(argv[1], "run") != 0) {
        printf("Expected to be called like %s run {your-cmd}\n", argv[0]);
        exit(1);
    }

    // Extract the command and its length
    char **cmd = argv + 2;
    pprint_cmd(cmd, argc - 2);

    // Make a stack for the child to start executing
    char *buf = malloc(1024);
    if (!buf) {
        perror("malloc");
        exit(1);
    }

    pid_t child_pid = clone(child_task, buf, CLONE_NEWPID, cmd);
    if (child_pid == -1) {
        perror("clone");
        free(buf);
        exit(1);
    }

    // 
    
    // // Fork and get the child
    // pid_t pid = fork();
    // if (pid == 0) {
    //     // We are the child
    //     printf("Im a child\n");
    //     execvp(cmd[0], cmd);
    // } else {
    //     // We are the parent
    //     printf("Im a parent\n");
    // }
    
    return 0;
}