#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>      // inet_pton, inet_ntop 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "utils.c"

#define MAXLINE 1024
#define STDIN 0

#define IF_NAME "enp0s3"
#define MULT_GROUP "224.0.0.1"
#define MULT_PORT "55555"


int resolve_address_and_connect (char* name) {
	int connfd;
	// address discovery
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;  // will point to the results

	char s[INET_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets


	// get ready to connect
	if (status = getaddrinfo(name, "remote_shell", &hints, &servinfo) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    	exit(1);
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


	return connfd;
}


int connect_service (struct sockaddr_in* server_address, char* service_name) {
	int connfd;
	// address discovery
	int status;
	struct sockaddr_in* addr;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;  // will point to the results

	char s[INET_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_addr = (struct sockaddr *) server_address;

	if (status = getaddrinfo(NULL, service_name, &hints, &servinfo) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    	exit(1);
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


	return connfd;
}



struct sockaddr_in* recv_all(int recvfd, socklen_t salen)
{
	int					n;
	char				line[MAXLINE+1];
	socklen_t			len;
	struct sockaddr		*safrom;
	char str[128];
	struct sockaddr_in6*	 cliaddr;
	struct sockaddr_in*	 cliaddrv4;
	char addr_str[INET6_ADDRSTRLEN+1];

	safrom = malloc(salen);

	for ( ; ; ) {
		len = salen;
		if( (n = recvfrom(recvfd, line, MAXLINE, 0, safrom, &len)) < 0 )
		  perror("recvfrom() error");

		line[n] = 0;	/* null terminate */
		
		if( safrom->sa_family == AF_INET6 ){
		      // cliaddr = (struct sockaddr_in6*) safrom;
		      // inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr->sin6_addr,  addr_str, sizeof(addr_str));
			printf("Protocol not supported\n");
			exit(1);
		}
		else{
		      cliaddrv4 = (struct sockaddr_in*) safrom;
		      inet_ntop(AF_INET, (struct sockaddr  *) &cliaddrv4->sin_addr,  addr_str, sizeof(addr_str));
		}

		printf("Datagram from %s : %s (%d bytes)\n", addr_str, line, n);
		fflush(stdout);

		return cliaddrv4;
		
	}
}


struct sockaddr_in* multicast_discover_service(const char* serv, char* port, char* interface) {
	int recvfd;
	struct sockaddr_in* servaddr;
	struct sockaddr_in* sarecv;
	const int on = 1;
	socklen_t salen;

	// int sendfd;
    // int recvfd;
    // struct sockaddr_in *ipv4addr;
    struct sockaddr_in *pservaddrv4;
	int port_val;
    // convert port to integer
    port_val = atoi(port);

	printf("Started discovery service\n");

	
	pservaddrv4 = malloc( sizeof(struct sockaddr_in));

	bzero(pservaddrv4, sizeof(struct sockaddr_in));
	
	printf("Creating receive socket\n");
	
	
    if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) != -1){
	
		sarecv = malloc( sizeof(struct sockaddr_in));
		pservaddrv4 = (struct sockaddr_in*)sarecv;
		bzero(pservaddrv4, sizeof(struct sockaddr_in));

		if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0){
			fprintf(stderr,"AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
			exit(1);
		}else{
			pservaddrv4->sin_family = AF_INET;
			pservaddrv4->sin_port   = htons(port_val);
			salen =  sizeof(struct sockaddr_in);
			if ( (recvfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				fprintf(stderr,"socket error : %s\n", strerror(errno));
				exit(1);
			}
		}
    } 
    else {
        printf("The protocol is not supported\n");
        exit(1);
    }

	if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		exit(1);
	}


	
	if( bind(recvfd, (struct sockaddr*) sarecv, salen) < 0 ){
	    fprintf(stderr,"bind error : %s\n", strerror(errno));
	    exit(1);
	}


	if( mcast_join(recvfd, (struct sockaddr*) sarecv, salen, interface, 0) < 0 ){
		fprintf(stderr,"mcast_join() error : %s\n", strerror(errno));
		exit(1);
	}

	printf("Joined group\n");

	servaddr = recv_all(recvfd, salen);
	return servaddr;

}

///////////////////////
///       Main      ///
///////////////////////

int main(int argc, char *argv[]) {
	// Main declarations
	int connfd;
	struct sockaddr_in  *servaddr;
	int err;

	char buffer[MAXLINE];

	if (argc < 2)
	{
		fprintf(stderr,"usage %s hostname \n", argv[0]);
		return 1;
	}


	char addr_str[INET6_ADDRSTRLEN+1];
	servaddr = multicast_discover_service(MULT_GROUP,  MULT_PORT,  IF_NAME);
	inet_ntop(AF_INET, (struct sockaddr  *) &servaddr->sin_addr,  addr_str, sizeof(addr_str));

	printf("Discovered server address: %s:%d\n", addr_str, servaddr->sin_port);
	fflush(stdout);

	// using multicast service discovery
	connfd = connect_service(servaddr, argv[1]);

	// using addres resolution
	// connfd = resolve_address_and_connect(argv[1]);

	printf("Created new socket: %d\n", connfd);
	int n;
	int count=0;

	// read and print welcome message
	n = recv(connfd, buffer, MAXLINE, 0);
	buffer[n] = 0;	/* null terminate */
	if ( fputs(buffer, stdout) == EOF) {
		fprintf(stderr,"fputs error : %s\n", strerror(errno));
		return 1;
	}
	printf("Enter command: ");
	// sleep(20);

	// Sending message to server

	// reading from STDIN 
	char	sendline[MAXLINE], recvline[MAXLINE];
	int packets = 0;

	while (Fgets(sendline, MAXLINE, stdin) != NULL) {

		Writen(connfd, sendline, strlen(sendline));
		
		// if (Readline(connfd, recvline, MAXLINE) == 0){
		// 	perror("str_cli: server terminated prematurely");
		// 	exit(0);
		// }
		printf("Waiting...\n");

		if (read(connfd, &packets, sizeof(int)) == 0){
			perror("str_cli: server terminated prematurely");
			exit(0);
		}

		for(int n=0; n < packets; n++)
		{
			if (read(connfd, recvline, MAXLINE) == 0)
			{
				perror("str_cli: server terminated prematurely");
				exit(0);
			}
			Fputs(recvline, stdout);

			bzero(sendline, sizeof(sendline));
			bzero(recvline, sizeof(recvline));
		}
		printf("\nEnter command: ");
	}

	close(connfd);

}