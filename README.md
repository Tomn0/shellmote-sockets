# shellmote-sockets

## Setup
For testing of address resolution functionality add lines to following files:  
/etc/hosts  
```127.0.0.1 remote_shell.com```  


/etc/services  
```remote_shell	3490/tcp # remote shell - network programming project```  

For the multicast service discovery make sure that the interface have active multicast groups.  
Verify using the command:  
```ip maddr show dev <if-name>```  
Pass the corresponding multicast address and interface with selected port as program parameters.  

## Server syslog configuration (Ubuntu)  
1. Add logging file entry to syslog configuration file (on Ubuntu usually found under /etc/rsyslog.d directory under the name *-default.conf)  
    ```local7.*			/var/log/local7```  
2. In the /var/log directory create the file with permissions and ownership as below:  
    ```-rw-r--r-- 1 syslog adm 163322 lip  1 20:03 local7```  
3. Restart syslog service:  
    ```service rsyslog restart```  


## Running the program
Server  
compilation:  
```gcc server.c -o srv.out -lm```
  
```./srv.out <IP-multicast-address> <port> <if-name>```  

Client  
```./cli.out remote_shell <IP-multicast-address> <port> <if-name>```  