#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

// #include    <signal.h>
#include 	<sys/ioctl.h>
// #include 	<unistd.h>
#include 	<net/if.h>
// #include	<netdb.h>
#include	<sys/utsname.h>     // struct utsname used for the uname call
// #include	<linux/un.h>
// #include	<sys/utsname.h>

#include "parserv3.c"
#include "utils.c"

#define MAXLINE 1024
#define LISTENQ 3
#define MAXCLIENTS 10
#define STDIN 0
#define PORT "3490"
#define IPADDRESS "10.0.2.15"
#define IF_NAME "enp0s3"
// #define SA struct sockaddr


int family_to_level(int family)
{
	switch (family) {
        case AF_INET:
            return IPPROTO_IP;
        #ifdef	IPV6
            case AF_INET6:
                return IPPROTO_IPV6;
        #endif
        default:
            return -1;
        }
}

unsigned int
_if_nametoindex(const char *ifname)
{
	int s;
	struct ifreq ifr;
	unsigned int ni;

	s = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (s != -1) {

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	
	if (ioctl(s, SIOCGIFINDEX, &ifr) != -1) {
			close(s);
			return (ifr.ifr_ifindex);
	}
		close(s);
		return -1;
	}
}

int create_send_udp_socket()
{
    int udpsockfd;

    if (udpsockfd = socket(AF_INET, SOCK_DGRAM, 0) < 0)
    {
        fprintf(stderr, "UDP socket error: %s\n", strerror(errno));
        exit(1);
    }

    return udpsockfd;
}

int mcast_join(int sockfd, const struct sockaddr *grp, socklen_t grplen,
		   const char *ifname, u_int ifindex)
{
	struct group_req req;
	if (ifindex > 0) {
		req.gr_interface = ifindex;
	} else if (ifname != NULL) {
		if ( (req.gr_interface = if_nametoindex(ifname)) == 0) {
			errno = ENXIO;	/* if name not found */
			return(-1);
		}
	} else
		req.gr_interface = 0;
	if (grplen > sizeof(req.gr_group)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),
			MCAST_JOIN_GROUP, &req, sizeof(req)));
}

int sockfd_to_family(int sockfd)
{
	struct sockaddr_storage ss;
	socklen_t	len;

	len = sizeof(ss);
	if (getsockname(sockfd, (struct sockaddr *) &ss, &len) < 0)
		return(-1);
	return(ss.ss_family);
}

int mcast_set_loop(int sockfd, int onoff)
{
	switch (sockfd_to_family(sockfd)) {
	case AF_INET: {
		u_char		flag;

		flag = onoff;
		return(setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
						  &flag, sizeof(flag)));
	}

#ifdef	IPV6
	case AF_INET6: {
		u_int		flag;

		flag = onoff;
		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
						  &flag, sizeof(flag)));
	}
#endif

	default:
		errno = EAFNOSUPPORT;
		return(-1);
	}
}


void	send_all(int, struct sockaddr *, socklen_t);

#define	SENDRATE	5		/* send one datagram every five seconds */

void send_all(int sendfd, struct sockaddr *sadest, socklen_t salen)
{
	char		line[MAXLINE];		/* hostname and process ID */
	struct utsname	myname;

	if (uname(&myname) < 0)
		perror("uname error");
	snprintf(line, sizeof(line), "%s, PID=%d", myname.nodename, getpid());

	for ( ; ; ) {
		if(sendto(sendfd, line, strlen(line), 0, sadest, salen) < 0 )
		  fprintf(stderr,"sendto() error : %s\n", strerror(errno));
		sleep(SENDRATE);
	}
}

void multicast_service(const char* serv, char* port, char* interface) {
    int sendfd;
    int recvfd;
    const int on = 1;
    socklen_t salen;
    struct sockaddr *sasend, *sarecv;
    struct sockaddr_in *ipv4addr;
    struct sockaddr_in *pservaddrv4;
    int port_val;
    // convert port to integer
    port_val = atoi(port);

    pservaddrv4 = malloc( sizeof(struct sockaddr_in6));
	
	// pservaddrv4 = (struct sockaddr_in*)sasend;

	bzero(pservaddrv4, sizeof(struct sockaddr_in));

    // create send socket

    if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) != -1){
	
		free(sasend);
		sasend = malloc( sizeof(struct sockaddr_in));
		pservaddrv4 = (struct sockaddr_in*)sasend;
		bzero(pservaddrv4, sizeof(struct sockaddr_in));

		if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0){
			fprintf(stderr,"AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
			return -1;
		}else{
			pservaddrv4->sin_family = AF_INET;
			pservaddrv4->sin_port   = htons(port_val);
			salen =  sizeof(struct sockaddr_in);
            if ((sendfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                fprintf(stderr, "UDP socket error: %s\n", strerror(errno));
                exit(1);
            }
		}
    } 
    else {
        printf("The protocol is not supported\n");
        exit(1);
    }

    // if (sendfd = socket(AF_INET, SOCK_DGRAM, 0) < 0) {
    //     fprintf(stderr, "UDP socket error: %s\n", strerror(errno));
    //     exit(1);
    // }


    if (setsockopt(sendfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		exit(1);
	}
    printf("passed\n");

    // create receive socket

    if ((recvfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "UDP socket error: %s\n", strerror(errno));
        exit(1);
    }
    printf("socket: %d\n", recvfd);
    if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		exit(1);
	}

    sarecv = malloc(salen);
	memcpy(sarecv, sasend, salen);

    // pass interface as the third argument???
	setsockopt(sendfd, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface));

    if(sarecv->sa_family == AF_INET){
        ipv4addr = (struct sockaddr_in *) sarecv;
        ipv4addr->sin_addr.s_addr =  htonl(INADDR_ANY);

        struct in_addr        localInterface;
        localInterface.s_addr = inet_addr(IPADDRESS);
        if (setsockopt(sendfd, IPPROTO_IP, IP_MULTICAST_IF,
                        (char *)&localInterface,
                        sizeof(localInterface)) < 0) {
            perror("setting local interface");
            exit(1);
    }
	}
    else {
        printf("The protocol is not supported\n");
        return -1;
    }

    if( bind(recvfd, sarecv, salen) < 0 ){
	    fprintf(stderr,"bind error : %s\n", strerror(errno));
	    return 1;
	}
	
	if( mcast_join(recvfd, sasend, salen, IF_NAME, 0) < 0 ){
		fprintf(stderr,"mcast_join() error : %s\n", strerror(errno));
		return 1;
	}
	  
	mcast_set_loop(sendfd, 1);

	send_all(sendfd, sasend, salen);	/* parent -> sends */

}

int handle_client_connection(int fd, char *command)
{
    char output_buffer[MAXLINE];
    int output_size = MAXLINE;
    int pipelen;

    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("Pipe failed");
        exit(1);
    }

    bzero(&output_buffer, sizeof(output_buffer));

    printf("Handling client's connection\n");

    // print client command
    if (fputs(command, stdout) == EOF)
        perror("fputs error");

    if (pipelen = execute_command(command, pipefd) < 0)
    {
        printf("Error in command execution");
        return -1;
    }
    close(pipefd[1]);
    printf("No error\n");

    while (read(pipefd[0], output_buffer, output_size) != 0)
    {
        printf("%s", output_buffer);
        send(fd, output_buffer, output_size, 0);
    }
    printf("\n");
    close(pipefd[0]);

    return 0;
}

int main(int argc, char **argv)
{
    // Main declarations
    int listenfd, cli_socket, client_socket[MAXCLIENTS], max_clients = MAXCLIENTS, sd, max_sd, activity, valread;

    int addrlen;
    struct sockaddr_in servaddr, cliaddr;

    char recvbuff[MAXLINE], str[INET_ADDRSTRLEN + 1], return_output[MAXLINE];
    // select declarations
    fd_set readfds;

    // a message
    char *welcome_message = "Remote shell server v1.0 \r\n";

    if (argc < 4)
	{
		fprintf(stderr,"usage: %s <IP-multicast-address> <port> <if name>\n", argv[0]);
		return 1;
	}


    // // set address resolution
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p; // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;       // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
                                     // hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    // get ready to connect
    if (status = getaddrinfo("remote_shell.com", "remote_shell", &hints, &servinfo) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    // initialise all client_socket[] to 0 so not checked
    for (int i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {

        // creation of a TCP socket
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            fprintf(stderr, "socket error: %s\n", strerror(errno));
            continue;
        }

        // set SO_REUSEADDR option
        const int reuseaddr = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (int *)&reuseaddr, sizeof(reuseaddr)) < 0)
        {
            fprintf(stderr, "setsockopt:REUSEADDR error: %s\n", strerror(errno));
            return 1;
        }

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listenfd);
            fprintf(stderr, "bind error: %s\n", strerror(errno));
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(listenfd, LISTENQ) < 0)
    {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        return 1;
    }

    printf("Waiting for connections...\n");


    // declarations for multicasting - unused??
    socklen_t salen;
	struct sockaddr	*sasend;

    // MULTICASTING - fork here is not a good idea
    if (fork() == 0) {
        multicast_service(argv[1], argv[2], argv[3]);
    }

    addrlen = sizeof(cliaddr);
    for (;;)
    {
        // prepare socket sets
        FD_ZERO(&readfds);

        // add listenfd to socket set
        FD_SET(listenfd, &readfds);
        // FD_SET(STDIN, &readfds);
        max_sd = listenfd;
        // printf("max_sd: %d\n", max_sd);

        // add child sockets to set
        for (int i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            // if valid add to socket set
            if (sd > 0)
            {
                FD_SET(sd, &readfds);
            }

            // save the highest socket number
            if (sd > max_sd)
            {
                max_sd = sd;
            }
            // printf("max_sd: %d\n", max_sd);
        }

        // waiting for socket activity
        //  printf("max_sd: %d\n", max_sd);
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        printf("activity: %d\n", activity);

        if ((activity < 0) && (errno != EINTR))
        {
            fprintf(stderr, "select error: %s\n", strerror(errno));
            return 1;
        }

        // handle incoming connections
        if (FD_ISSET(listenfd, &readfds))
        {

            if ((cli_socket = accept(listenfd, (struct sockaddr *)&cliaddr, &addrlen)) < 0)
            {
                fprintf(stderr, "accept error: %s\n", strerror(errno));
                return 1;
            }

            // inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n", cli_socket, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            bzero(str, sizeof(str));
            inet_ntop(AF_INET, (struct sockaddr *)&cliaddr.sin_addr, str, sizeof(str));
            printf("Connection from %s\n", str);

            if (send(cli_socket, welcome_message, strlen(welcome_message), 0) != strlen(welcome_message))
            {
                fprintf(stderr, "send error: %s\n", strerror(errno));
            }

            printf("Welcome message sent successfully\n");

            // add new socket to array of sockets
            for (int i = 0; i < max_clients; i++)
            {
                // if position is empty
                if (client_socket[i] == 0)
                {
                    client_socket[i] = cli_socket;
                    printf("Adding to list of sockets as %d\n", i);

                    break;
                }
            }
        }

        // check for IO operation on client sockets
        for (int i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds))
            {
                // Check if it was for closing , and also read the
                // incoming message
                if ((valread = read(sd, recvbuff, 1024)) == 0)
                {
                    // Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *)&cliaddr,
                                (socklen_t *)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n",
                           inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                }

                // handle the client
                else
                {

                    // set the string terminating NULL byte on the end
                    // of the data read
                    recvbuff[valread] = '\0';
                    handle_client_connection(sd, recvbuff);
                }
            }
        }

        // close(cli_socket);
    }

    return 0;
}
