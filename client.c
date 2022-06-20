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


///////////////////////
///       Main      ///
///////////////////////

int main(int argc, char *argv[]) {

	// address discovery
	// int status;
	// struct addrinfo hints;
	// struct addrinfo *servinfo;  // will point to the results

	// memset(&hints, 0, sizeof hints); // make sure the struct is empty
	// hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	// hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	// // get ready to connect
	// status = getaddrinfo("remote_shell.com", "3490", &hints, &servinfo);

	// freeaddrinfo(servinfo);


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
	servaddr.sin_port = htons(3490);

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

		if (Readline(connfd, recvline, MAXLINE) == 0){
			perror("str_cli: server terminated prematurely");
			exit(0);
		}
		Fputs(recvline, stdout);
	}

	close(connfd);

}