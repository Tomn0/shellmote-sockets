#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>   
#include <unistd.h>

#define MAXLINE 1024
#define LISTENQ 2

int main(int argc, char **argv) {
    // Main declarations
    int listenfd;
    int cli_socket;
    struct sockaddr_in	servaddr, cliaddr;
    int addrlen;
    char buffer[MAXLINE], str[INET_ADDRSTRLEN+1];


    // creation of a TCP socket

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return 1;

    }

    // ser reuseaddr option
    const int reuseaddr = 1;
    if( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (int *) &reuseaddr, sizeof(reuseaddr)) < 0) {
        fprintf(stderr, "setsockopt:REUSEADDR error: %s\n", strerror(errno));
        return 1;
    }


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        return 1;
    }


    if (listen(listenfd, LISTENQ) < 0) {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        return 1;
    }

    for(;;){
        addrlen = sizeof(cliaddr);
        if ((cli_socket = accept(listenfd, (struct sockaddr *) &cliaddr, &addrlen)) < 0) {
            fprintf(stderr, "accept error: %s\n", strerror(errno));
            return 1;
        }

        bzero(str, sizeof(str));
	   	inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  str, sizeof(str));
		printf("Connection from %s\n", str);




        close(cli_socket);
    }



    return 0;
}
