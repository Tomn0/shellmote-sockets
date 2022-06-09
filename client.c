#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>      // inet_pton, inet_ntop 
#include <string.h>


int main(int argc, char *argv[]) {
    // Main declarations
    int connfd;
    struct sockaddr_in  servaddr;
    int err;

    char buffer[256];

    if (argc < 2)
    {
      fprintf(stderr,"usage %s hostname \n", argv[0]);
      return 1;
    }


    if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return 1;
    }

    bzero((char *) &servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13);

    if ( (err=inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) <= 0){
		if(err == 0 )
			fprintf(stderr,"inet_pton error for %s \n", argv[1] );
		else
			fprintf(stderr,"inet_pton error for %s : %s \n", argv[1], strerror(errno));
		return 1;
	}

	if (connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"connect error : %s \n", strerror(errno));
		return 1;
	}


}