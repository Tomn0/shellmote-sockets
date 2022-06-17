#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>      // inet_pton, inet_ntop 
#include <string.h>
#include <unistd.h>

#define MAXLINE 1024

int main(int argc, char *argv[]) {
  // Main declarations
  int connfd;
  struct sockaddr_in  servaddr;
  int err;

  char buffer[MAXLINE];

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

  printf("Connected\n");

  int n;
  int count=0;
  // if (n = recv(connfd, buffer, MAXLINE, 0) < 0) {
  //   fprintf(stderr,"read error : %s \n", strerror(errno));
  // }

  // buffer[n] = 0; /* null terminate */
  // if (fputs(buffer, stdout) == EOF){
  //     fprintf(stderr,"fputs error: %s\n",strerror(errno));
  //     return 1;
  // }

  //close(connfd);

  while ( (n = read(connfd, buffer, MAXLINE)) > 0) {
		buffer[n] = 0;	/* null terminate */
		if(count == 0 )
		  if ( fputs(buffer, stdout) == EOF) {
			fprintf(stderr,"fputs error : %s\n", strerror(errno));
			return 1;
		  }
		count++;
	}

  close(connfd);  


  



}