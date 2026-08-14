#pragma once

#include <stdlib.h>

#define STACK_SIZE (1024 * 1024)

typedef struct container_args_t {
    int data[2]; // data channel
    int send_meta[2]; // metadata from container to parent
    int recv_meta[2]; // metadata from parent to container
    char **cmd;
} container_args_t;

void pprint_cmd(char **cmd, int count);
int setup_child_io(int data[2]);
int notify_parent(int send[2], int recv[2]);
int create_uid_mapping(pid_t container_pid, int recv[2]);
void print_summary();
int contain_container();