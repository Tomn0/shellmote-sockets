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
#include <math.h>

// #include    <signal.h>
#include <sys/ioctl.h>
// #include 	<unistd.h>
#include <net/if.h>
// #include	<netdb.h>
#include <sys/utsname.h> // struct utsname used for the uname call
// #include	<linux/un.h>
// #include	<sys/utsname.h>
#include <syslog.h>
// #include <sys/stat.h>
#include <fcntl.h>


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


void send_all(int, struct sockaddr *, socklen_t);

#define SENDRATE 5 /* send one datagram every five seconds */

void send_all(int sendfd, struct sockaddr *sadest, socklen_t salen)
{
    char line[MAXLINE]; /* hostname and process ID */
    struct utsname myname;

    if (uname(&myname) < 0)
        perror("uname error");
    snprintf(line, sizeof(line), "%s, PID=%d", myname.nodename, getpid());

    for (;;)
    {
        if (sendto(sendfd, line, strlen(line), 0, sadest, salen) < 0)
            syslog(LOG_ERR, "sendto() error : %s\n", strerror(errno));

        sleep(SENDRATE);
    }
}

void multicast_service(const char *serv, char *port, char *interface)
{
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

    pservaddrv4 = malloc(sizeof(struct sockaddr_in6));

    // pservaddrv4 = (struct sockaddr_in*)sasend;

    bzero(pservaddrv4, sizeof(struct sockaddr_in));

    // create send socket

    if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) != -1)
    {

        //free(sasend);
        sasend = malloc(sizeof(struct sockaddr_in));
        pservaddrv4 = (struct sockaddr_in *)sasend;
        bzero(pservaddrv4, sizeof(struct sockaddr_in));

        if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0)
        {
            syslog(LOG_ERR, "AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
            exit(1);
        }
        else
        {
            pservaddrv4->sin_family = AF_INET;
            pservaddrv4->sin_port = htons(port_val);
            salen = sizeof(struct sockaddr_in);
            if ((sendfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            {
                syslog(LOG_ERR, "UDP socket error: %s\n", strerror(errno));
                exit(1);
            }
        }
    }
    else
    {
        // printf("The protocol is not supported\n");
        exit(1);
    }


    if (setsockopt(sendfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        syslog(LOG_ERR, "setsockopt error : %s\n", strerror(errno));
        exit(1);
    }

    // create receive socket

    if ((recvfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        syslog(LOG_ERR, "UDP socket error: %s\n", strerror(errno));
        exit(1);
    }

    if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        syslog(LOG_ERR, "setsockopt error : %s\n", strerror(errno));
        exit(1);
    }

    sarecv = malloc(salen);
    memcpy(sarecv, sasend, salen);

    // pass interface as the third argument???
    setsockopt(sendfd, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface));

    if (sarecv->sa_family == AF_INET)
    {
        ipv4addr = (struct sockaddr_in *)sarecv;
        ipv4addr->sin_addr.s_addr = htonl(INADDR_ANY);

        struct in_addr localInterface;
        localInterface.s_addr = inet_addr(IPADDRESS);
        if (setsockopt(sendfd, IPPROTO_IP, IP_MULTICAST_IF,
                       (char *)&localInterface,
                       sizeof(localInterface)) < 0)
        {
            perror("setting local interface");
            exit(1);
        }
    }
    else
    {
        // printf("The protocol is not supported\n");
        exit(1);
    }

    if (bind(recvfd, sarecv, salen) < 0)
    {
        syslog(LOG_ERR,  "bind error : %s\n", strerror(errno));
        exit(1);
    }

    if (mcast_join(recvfd, sasend, salen, IF_NAME, 0) < 0)
    {
        syslog(LOG_ERR, "mcast_join() error : %s\n", strerror(errno));
        exit(1);
    }

    mcast_set_loop(sendfd, 1);

    send_all(sendfd, sasend, salen); /* parent -> sends */
}

int handle_client_connection(int fd, char *command)
{
    char output_buffer[MAXLINE];
    int output_size = MAXLINE;
    int pipelen;
    int packets;

    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("Pipe failed");
        exit(1);
    }

    bzero(&output_buffer, sizeof(output_buffer));

    syslog (LOG_NOTICE, "Handling client's connection\n");

    pipelen = execute_command(command, pipefd);
    if (pipelen < 0)
    {
        syslog (LOG_ERR, "Error in command execution\n");
        syslog (LOG_NOTICE, "ERRNO = %m"); 
        return -1;
    }
    close(pipefd[1]);

    // datatype mess calculating and sendint the info about amount of packets to send
    packets = ceil((double)pipelen / output_size);
    send(fd, &packets, sizeof(int), 0);

    syslog (LOG_NOTICE, "executed command: %s\nMAXLEN: %d\ndatalen: %d\npackets expected: %d (%lf)\n",
    command, output_size, pipelen, packets, (double)pipelen/output_size);

    // send the command output
    while (read(pipefd[0], output_buffer, output_size) != 0)
    {
        // syslog (LOG_NOTICE, "Sending:\n%s\n", output_buffer);    // for debuggin: print output buffer to syslog
        send(fd, output_buffer, output_size, 0);
    }
    close(pipefd[0]);

    return 0;
}

int main(int argc, char **argv)
{

    if (argc < 4)
    {
        fprintf(stderr, "usage: %s <IP-multicast-address> <port> <if name>\n", argv[0]);
        return 1;
    }

    // Daemon init
    if(daemon(0,1) != 0)
    {
        fprintf(stderr, "Daemon init failed");
        return 1;
    }

   
    // Main declarations
    int listenfd, cli_socket, client_socket[MAXCLIENTS], max_clients = MAXCLIENTS, sd, max_sd, activity, valread;

    int addrlen;
    struct sockaddr_in servaddr, cliaddr;

    char recvbuff[MAXLINE], str[INET_ADDRSTRLEN + 1], return_output[MAXLINE];
    // select declarations
    fd_set readfds;

    // a message
    char *welcome_message = "Remote shell server v1.0 \r\n";

    // set syslog
    setlogmask (LOG_UPTO (LOG_NOTICE));
	openlog ("remote_shell", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL7);


    // LOG_CONS
    // Write messages to the system console if they cannot be sent 
    // to the logging facility. This option is safe to use in processes 
    // that have no controlling terminal, since the syslog() function forks 
    // before opening the console.
    // LOG_NDELAY
    // Open the connection to the logging facility immediately. 
    // Normally the open is delayed until the first message is logged. 
    // This is useful for programs that need to manage the order in which 
    // file descriptors are allocated..
    // LOG_PID
    // Log the processID with each message. 
    // This is useful for identifying specific processes. 
    // In the message header, the processID is surrounded by square brackets. 

    // // set address resolution
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p; // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;       // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
                                     // hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    // get ready to connect
    if (status = getaddrinfo(NULL, "remote_shell", &hints, &servinfo) != 0)
    {
        syslog(LOG_ERR, "getaddrinfo error: %s\n", gai_strerror(status));
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
            syslog(LOG_ERR,  "socket error: %s\n", strerror(errno));
            continue;
        }

        // set SO_REUSEADDR option
        const int reuseaddr = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (int *)&reuseaddr, sizeof(reuseaddr)) < 0)
        {
            syslog(LOG_ERR, "setsockopt:REUSEADDR error: %s\n", strerror(errno));
            return 1;
        }

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listenfd);
            syslog(LOG_ERR, "bind error: %s\n", strerror(errno));
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)
    {
        syslog(LOG_ERR,  "server: failed to bind\n");
        exit(1);
    }

    if (listen(listenfd, LISTENQ) < 0)
    {
        syslog(LOG_ERR, "listen error: %s\n", strerror(errno));
        return 1;
    }

    syslog (LOG_NOTICE, "Waiting for connections...\n");

    // declarations for multicasting - unused??
    socklen_t salen;
    struct sockaddr *sasend;

    // MULTICASTING - fork here is not a good idea
    int pid = fork();
    if (pid == 0)
    {
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
        }

        // waiting for socket activity
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            syslog(LOG_ERR, "select error: %s\n", strerror(errno));
            return 1;
        }

        // handle incoming connections
        if (FD_ISSET(listenfd, &readfds))
        {

            if ((cli_socket = accept(listenfd, (struct sockaddr *)&cliaddr, &addrlen)) < 0)
            {
                syslog(LOG_ERR, "accept error: %s\n", strerror(errno));
                return 1;
            }

            // inform user of socket number - used in send and receive commands
            syslog (LOG_NOTICE, "New connection , socket fd is %d , ip is : %s , port : %d \n", cli_socket, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            bzero(str, sizeof(str));
            inet_ntop(AF_INET, (struct sockaddr *)&cliaddr.sin_addr, str, sizeof(str));
	        syslog (LOG_NOTICE, "Connection from %s\n", str);

            if (send(cli_socket, welcome_message, strlen(welcome_message), 0) != strlen(welcome_message))
            {
                syslog(LOG_ERR, "send error: %s\n", strerror(errno));
            }
            syslog (LOG_NOTICE, "Welcome message sent successfully\n");

            // add new socket to array of sockets
            for (int i = 0; i < max_clients; i++)
            {
                // if position is empty
                if (client_socket[i] == 0)
                {
                    client_socket[i] = cli_socket;

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
                    syslog(LOG_NOTICE, "Host disconnected , ip %s , port %d \n",
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
                    if (strncmp(recvbuff, "exit", 4) == 0)
                    {
                        syslog (LOG_NOTICE, "Recieved 'exit', terminating\n");
                        kill(pid, SIGTERM);
                        return 1;
                    }
                    handle_client_connection(sd, recvbuff);
                }
            }
        }

        // close(cli_socket);
    }
    closelog();
    return 0;
}
