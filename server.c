#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>   
#include <unistd.h>
#include <netdb.h>

#include "parserv3.c"

#define MAXLINE 1024
#define LISTENQ 3
#define MAXCLIENTS 10

#define STDIN 0

#define PORT "3490"


int handle_client_connection(int fd, char * command) {
    char output_buffer[MAXLINE];
    int output_size = MAXLINE;

    bzero(&output_buffer, sizeof(output_buffer));

    printf("Handling client's connection\n");

    // print client command
    if (fputs(command, stdout) == EOF)
        perror("fputs error");

    if (execute_command(command, output_buffer, output_size) != 0) {
        printf("Error in command execution");
        return -1;
    }
    printf("No error\n");
    printf("Output: %s", output_buffer);

    send(fd , output_buffer , output_size , 0);  

    return 0;
}

int main(int argc, char **argv) {
    // Main declarations
    int listenfd, cli_socket, client_socket[MAXCLIENTS], max_clients = MAXCLIENTS, sd, max_sd, activity, valread;
    
    
    int addrlen;
    struct sockaddr_in	servaddr, cliaddr;

    char recvbuff[MAXLINE], str[INET_ADDRSTRLEN+1], return_output[MAXLINE];
    // select declarations
    fd_set readfds;

    //a message 
    char *welcome_message = "Remote shell server v1.0 \r\n";  


    // // set address resolution
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;  // will point to the results

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    // hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    
	// get ready to connect
	if (status = getaddrinfo("remote_shell.com", "remote_shell", &hints, &servinfo) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    	return 1;
	}

    //initialise all client_socket[] to 0 so not checked 
    for (int i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }  

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {

        // creation of a TCP socket
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            fprintf(stderr, "socket error: %s\n", strerror(errno));
            continue;

        }

        // set SO_REUSEADDR option
        const int reuseaddr = 1;
        if( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (int *) &reuseaddr, sizeof(reuseaddr)) < 0) {
            fprintf(stderr, "setsockopt:REUSEADDR error: %s\n", strerror(errno));
            return 1;
        }

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(listenfd);
            fprintf(stderr, "bind error: %s\n", strerror(errno));
            continue;
        }
        break;

    }
    freeaddrinfo(servinfo); // all done with this structure


    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }


    if (listen(listenfd, LISTENQ) < 0) {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        return 1;
    }

    printf("Waiting for connections...\n");

    addrlen = sizeof(cliaddr);
    for(;;){
        // prepare socket sets
        FD_ZERO(&readfds);

        // add listenfd to socket set
        FD_SET(listenfd, &readfds);
        //FD_SET(STDIN, &readfds);
        max_sd = listenfd;
        //printf("max_sd: %d\n", max_sd);

        // add child sockets to set
        for (int i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            
            // if valid add to socket set
            if (sd > 0) {
                FD_SET( sd, &readfds );
            }

            // save the highest socket number
            if(sd > max_sd) {
                max_sd = sd; 
            }
            // printf("max_sd: %d\n", max_sd);

        }

        //waiting for socket activity
        // printf("max_sd: %d\n", max_sd);
        activity = select( max_sd+1 , &readfds , NULL , NULL , NULL);  
        printf("activity: %d\n", activity);

        if ((activity < 0) && (errno!=EINTR))  
        {  
            fprintf(stderr, "select error: %s\n", strerror(errno));
            return 1;
        }  

        // handle incoming connections
        if (FD_ISSET(listenfd, &readfds)) {

            if ((cli_socket = accept(listenfd, (struct sockaddr *) &cliaddr, &addrlen)) < 0) {
                fprintf(stderr, "accept error: %s\n", strerror(errno));
                return 1;
            }

            //inform user of socket number - used in send and receive commands 
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , cli_socket , inet_ntoa(cliaddr.sin_addr) , ntohs(cliaddr.sin_port));  

            bzero(str, sizeof(str));
            inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  str, sizeof(str));
            printf("Connection from %s\n", str);


            if( send(cli_socket, welcome_message, strlen(welcome_message), 0) != strlen(welcome_message) )  
            {  
                fprintf(stderr, "send error: %s\n", strerror(errno));
            }  
                
            printf("Welcome message sent successfully\n");  


            //add new socket to array of sockets 
            for (int i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = cli_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                         
                    break;  
                }  
            }  
        }

        // check for IO operation on client sockets
        for (int i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , recvbuff, 1024)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&cliaddr , \
                        (socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(cliaddr.sin_addr) , ntohs(cliaddr.sin_port));  
                         
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0;  
                }  
                     
                // handle the client 
                else 
                {  
                    
                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    recvbuff[valread] = '\0'; 
                    handle_client_connection(sd, recvbuff); 

                }  
            }  
        }  

        // close(cli_socket);
    }

    return 0;
}
