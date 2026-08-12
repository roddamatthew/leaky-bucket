#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/sched.h>
#include <sched.h>
#include <sys/syscall.h>
#include <errno.h>

int child_task()
{
    pid_t child_pid = getpid();
    printf("Im a child, with PID=%d\n", child_pid);
    return 0;
}

int parent_task()
{
    pid_t parent_pid = getpid();
    printf("Im a parent, with PID=%d\n", parent_pid);
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
    printf("%s %s\n", cmd[0], cmd[1]);

    int child_tid = clone(child_task);
    if (child_tid == -1) {
        perror("clone");
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