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
    close(pipe[0]); // Close read end of pipe
    
    // Redirect STDOUT and STDERR to write end of pipe
    if (dup2(pipe[1], STDOUT_FILENO) == -1) {
        perror("dup2 stdout");
        exit(1);
    }
    if (dup2(pipe[1], STDERR_FILENO) == -1) {
        perror("dup2 stderr");
        exit(1);
    }

    close(pipe[1]); // Can now close write end

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