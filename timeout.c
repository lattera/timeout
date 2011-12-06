#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <signal.h>
#include <errno.h>

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s <timeout> <app> <arguments>\n", name);
    exit(1);
}

void sighandler(int signo)
{
    int status;

    if (signo == SIGCHLD) {
        waitpid(-1, &status, WNOHANG);
        if (WIFEXITED(status))
            exit(WEXITSTATUS(status));
    }
}

int main(int argc, char *argv[], char *envp[])
{
    pid_t pid;
    unsigned long timeout;
    struct timeval tv;
    time_t starttime;

    if (argc < 3)
        usage(argv[0]);

    if (sscanf(argv[1], "%ul", &timeout) != 1)
        usage(argv[0]);

    starttime = time(NULL);
    signal(SIGCHLD, sighandler);

    pid = fork();
    if (pid == 0)
        return execve(argv[2], &argv[2], envp);

    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    while (1) {
        if (select(1, NULL, NULL, NULL, &tv) == -1) {
            if (errno == EINTR) {
                if (((int)(time(NULL) - starttime - timeout)) <= 0)
                    break;
                tv.tv_sec = time(NULL) - starttime - timeout;
            }
        } else {
            break;
        }
    }

    kill(pid, SIGTERM);

    exit(0);
}
