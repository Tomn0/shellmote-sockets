#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int execute_command(char *command, int pipefd[2])
{
    int datalen;
    command = strtok(command, "\n");

    if (fork() == 0)
    {
        dup2(pipefd[1], 1);
        dup2(pipefd[1], 2);
        close(pipefd[0]);
        close(pipefd[1]);

        char *prog[4] = {"/bin/sh", "-c", command, NULL};
        execvp(prog[0], prog);
        perror("execvp failed");
        exit(1);
    }

    wait(0);
    close(pipefd[1]);
    if (ioctl(pipefd[0], FIONREAD, &datalen) != 0)
    {
        perror("ioctl failed");
    }

    return datalen;
}
