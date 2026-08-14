#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/mount.h>

void pprint_cmd(char **cmd, int count)
{
    printf("running [");
    for (int i = 0; i < count; i++) {
        printf("%s", cmd[i]);
        if (i < count - 1) printf(" ");
    }
    printf("]\n");
}

int setup_child_io(int pipe[2])
{
    // Redirect STDOUT and STDERR to write end of pipe
    if (dup2(pipe[1], STDOUT_FILENO) == -1) {
        perror("dup2 stdout");
        exit(1);
    }
    if (dup2(pipe[1], STDERR_FILENO) == -1) {
        perror("dup2 stderr");
        exit(1);
    }

    // Can now close the redirected pipe
    for (int i = 0; i < 2; i++) {
        close(pipe[i]);
    }
    return 0;
}

void print_summary()
{
    // Print a summary of the info that a container might change
    char hostname[256] = {0};
    char cwd[256] = {0};

    struct passwd *pw = getpwuid(getuid());
    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    printf("*** SUMMARY OF PID=%d ***\n", getpid());
    printf("> user=%s\n", pw->pw_name);
    printf("> hostname=%s\n", hostname);
    printf("> cwd=%s\n", cwd);
    printf("*** END OF SUMMARY ***\n");
}

int contain_container()
{
    // Update user (can do this since we made a UID mapping)
    if (setuid(0) == -1) {
        perror("setuid");
        exit(1);
    }
    if (setgid(0) == -1) {
        perror("setgid");
        exit(1);
    }
    
    // Update resources associated with container
    char hostname[] = "leaky-bucket";
    if (sethostname(hostname, strlen(hostname)) == -1) {
        perror("sethostname");
        exit(1);
    }
    if (chroot("../rootfs") == -1) {
        perror("chroot");
        exit(1);
    }
    if (chdir("/") == -1) {
        perror("chdir");
        exit(1);
    }
    if (mount("proc", "/proc", "proc", 0, "") == -1) {
        perror("mount");
        exit(1);
    }

    return 0;
}

int notify_parent(int send[2], int recv[2])
{
    // Notify the parent that it can write to the UID mapping of the container
    write(send[1], "y", 1);
    printf("wrote to args.send to say container ready\n");

    printf("container waiting for parent\n");
    // Wait to hear that the UID mapping has been written
    char ready = 'n';
    if (read(recv[0], &ready, 1) <= 0) {
        perror("read recv");
        exit(1);
    }
    if (ready != 'y') {
        printf("[!] Unexpected data from parent: %c\n", ready);
        exit(1);
    }

    // Close pipes!
    for (int i = 0; i < 2; i++) {
        close(send[i]);
        close(recv[i]);
    }
    return 0;
}

int create_uid_mapping(pid_t container_pid, int pipe[2])
{
    // Write to the containers UID mapping in the parent
    char uid_path[256] = {0};
    char gid_path[256] = {0};
    sprintf(uid_path, "/proc/%d/uid_map", container_pid);
    sprintf(gid_path, "/proc/%d/gid_map", container_pid);

    int uid_fd = open(uid_path, O_RDWR);
    int gid_fd = open(gid_path, O_RDWR);
    if (uid_fd == -1 || gid_fd == -1) {
        perror("/proc open");
        exit(1);
    }

    char mapping[] = "0 1000 1"; // Map root (0) to user (1000), just the one (1)
    int uid_write = write(uid_fd, mapping, sizeof(mapping));
    int gid_write = write(gid_fd, mapping, sizeof(mapping));
    if (uid_write != sizeof(mapping) || gid_write != sizeof(mapping)) {
        perror("/proc write");
        exit(1);
    }

    close(uid_fd);
    close(gid_fd);

    // Once complete, notify the container
    
    write(pipe[1], "y", 1);
    printf("wrote to args.recv\n");
    return 0;
}