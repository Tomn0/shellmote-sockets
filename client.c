#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>      // inet_pton, inet_ntop 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>

#define MAXLINE 1024

#define STDIN 0

///////////////////////
///      Utils      ///
///////////////////////
void Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		perror("fputs error");
}

char * Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		perror("fgets error");

	return (rptr);
}
/* Write "n" bytes to a descriptor. */
ssize_t writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/* end writen */

void Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		perror("writen error");
}

static int	read_cnt;
static char	*read_ptr;
static char	read_buf[MAXLINE];

static ssize_t my_read(int fd, char *ptr)
{

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

/* end readline */

ssize_t Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		perror("readline error");
	return(n);
}


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
	else
		printf("Protocol not supported\n");
		exit(1);
}


///////////////////////
///       Main      ///
///////////////////////

int main(int argc, char *argv[]) {
	// Main declarations
	int connfd;
	struct sockaddr_in  servaddr;
	int err;

	char buffer[MAXLINE];

	// address discovery
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;  // will point to the results

	char s[INET_ADDRSTRLEN];

	if (argc < 2)
	{
		fprintf(stderr,"usage %s hostname \n", argv[0]);
		return 1;
	}

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets


	// get ready to connect
	if (status = getaddrinfo(argv[1], "remote_shell", &hints, &servinfo) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    	return 1;
	}


	// loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((connfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			fprintf(stderr, "socket error: %s\n", strerror(errno));
			continue;
		}

		if (connect(connfd, p->ai_addr, p->ai_addrlen) < 0){
			close(connfd);
			fprintf(stderr,"connect error : %s \n", strerror(errno));
			continue;
		}

		break;
	}

	if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
	else {
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    	printf("client: connecting to %s\n", s);
		printf("Connected\n");
		freeaddrinfo(servinfo);	// all done with this structure
	}


	int n;
	int count=0;

	// read and print welcome message
	n = recv(connfd, buffer, MAXLINE, 0);
	buffer[n] = 0;	/* null terminate */
	if ( fputs(buffer, stdout) == EOF) {
		fprintf(stderr,"fputs error : %s\n", strerror(errno));
		return 1;
	}

	// sleep(20);

	// Sending message to server

	// reading from STDIN 
	char	sendline[MAXLINE], recvline[MAXLINE];

	while (Fgets(sendline, MAXLINE, stdin) != NULL) {

		Writen(connfd, sendline, strlen(sendline));
		
		// if (Readline(connfd, recvline, MAXLINE) == 0){
		// 	perror("str_cli: server terminated prematurely");
		// 	exit(0);
		// }
		printf("Waiting...\n");

		if (read(connfd, recvline, MAXLINE) == 0){
			perror("str_cli: server terminated prematurely");
			exit(0);
		}
	

		Fputs(recvline, stdout);

		bzero(sendline, sizeof(sendline));
		bzero(recvline, sizeof(recvline));
	}

	close(connfd);

}