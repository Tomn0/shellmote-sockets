#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[])
{
    char buffer[1024];
    char workbuffer[1024];
    char* commands[5][10];
    char* token;
    int i=0; int z=0;


    printf("Input: ");
    fgets(buffer, 1024, stdin);

    strcpy(workbuffer, buffer);

    token = strtok(workbuffer, "|");
    while(token != NULL)
    {
        commands[i][0] = token;
        i += 1;
        token = strtok(NULL,"|");
    }
    for (int n=0 ; n<i ; n++)
    {
        printf("index: %d ", n);
        strcpy(workbuffer, commands[n][0]);
        printf("%s\n", workbuffer);
        token = strtok(workbuffer," ");
        z = 0;
        while(token != NULL)
        {
            commands[n][z] = token;
            printf("n: %d z: %d word: %s\n", n, z, commands[n][z]);
            z += 1;
            token = strtok(NULL," ");
        }
    }
    return 0;
}