#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <stdlib.h>

#define MAXLINE 500

int main(int argc, char **argv) {
    // Main declarations
    int listenfd;
    int cli_socket;
    struct sockaddr_in	servaddr, cliaddr;
    char buffer;


    // creation of a TCP socket

    if (listenfd = socket(AF_INET, SOCK_STREAM, 0) < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return 1;

    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13);
    servaddr.sin_addr.s_addr = inet_addr();

    return 0;
}
