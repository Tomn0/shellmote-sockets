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
#define LISTENQ 3

int main(int argc, char **argv) {
    // Main declarations
    int listenfd, cli_socket, client_socket[10], max_clients = 30, sd, max_sd, activity, valread;
    int addrlen;
    struct sockaddr_in	servaddr, cliaddr;

    char buffer[MAXLINE], str[INET_ADDRSTRLEN+1];
    // select declarations
    fd_set readfds;

    //a message 
    char *welcome_message = "Remote server v1.0 \r\n";  

    //initialise all client_socket[] to 0 so not checked 
    for (int i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }  


    // creation of a TCP socket

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return 1;

    }

    // set SO_REUSEADDR option
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

    printf("Waiting for connections...\n");

    addrlen = sizeof(cliaddr);
    for(;;){
        // prepare socket sets
        FD_ZERO(&readfds);

        // add listenfd to socket set
        FD_SET(listenfd, &readfds);
        max_sd = listenfd;

        // add child sockets to set
        for (int i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            
            // if valid add to socket set
            if (sd > 0) {
                FD_SET( sd, &readfds );
            }

            // save the highest sockete number
            if(sd > max_sd) {
                max_sd = sd; 
            }

        }

        //waiting for socket activity
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
       
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
                
            printf("Welcome message sent successfully");  



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

        // check for IO operation son client sockets
        for (int i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , buffer, 1024)) == 0)  
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
                     
                //Echo back the message that came in 
                else 
                {  
                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    buffer[valread] = '\0';  
                    send(sd , buffer , strlen(buffer) , 0 );  
                }  
            }  
        }  

        // close(cli_socket);
    }



    return 0;
}
