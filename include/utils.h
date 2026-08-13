#pragma once

#define STACK_SIZE (1024 * 1024)

typedef struct container_args_t {
    int pipefd[2];
    char **cmd;
} container_args_t;

void pprint_cmd(char **cmd, int count);
int setup_child_io(int pipe[2]);
int setup_parent_io(int pipe[2]);
void print_summary();