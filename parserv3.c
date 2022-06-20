#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int execute_command(char *command, char *buffor, int buff_size)
{
    command = strtok(command, "\n");

    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("Pipe failed");
        exit(1);
    }

    if (fork() == 0)
    {
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        close(pipefd[1]);

        char *prog[4] = {"/bin/sh", "-c", command, NULL};
        execvp(prog[0], prog);
        perror("execvp failed");
        exit(1);
    }

    close(pipefd[1]);
    read(pipefd[0], buffor, buff_size-1);
    buffor[buff_size-1] = "\0";
    close(pipefd[0]);

    return 0;
}
