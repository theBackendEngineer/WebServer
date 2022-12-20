// Name: Vemuri Venkat Rohit
// NetId: gs4010
// IDE: VSCode
// OS: Ubuntu 22 LTS
// Server: Server program which appends "back to you" to msg and sends to client.

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Some structs for reference
#if 0
/* 
 * Structs exported from in.h
 */

/* Internet address */
struct in_addr {
  unsigned int s_addr; 
};

/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; /* Address family */
  unsigned short int sin_port;   /* Port number */
  struct in_addr sin_addr;	 /* IP address */
  unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
}
#endif

// as we are only message passing i am only keeping buffer of 2kb
#define BUFSIZE 2048

// print error and exit
void err(char *msg){
    perror(msg);
    exit(1);
}

// function to append string to message buffer
void appendMsgToBuffer(char ar[]){
    char str[] = " Back to You";
    for(int i = 0;i < BUFSIZE;i++){
        if(ar[i] == 10){
            for(int j = 0;j< sizeof(str)/sizeof(str[0]);j++){
                ar[i+j] = str[j];
            }
        }
    }
}


// main function start
int main(int argc, char **argv)
{
    // sockets
    int serverfd, clientfd;
    // portno
    int portNo = 8080;
    // byte size of client address
    int clientlen;
    // server addresses
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    // client host info
    struct hostent *hostp;
    // buffer to store messages
    char buf[BUFSIZE];
    // dotted decimal host addr string
    char *hostaddrp;
    // flag value for socketopt
    int optval;
    // msg byte size
    int n;
    // check arguments
    if (argc != 2)
    {
        printf("Deafult port number 8080\n");
        printf("For custom pass: %s <port>\n", argv[0]);
    }
    else
    {
        portNo = atoi(argv[1]);
    }
    // create the TCP server socket
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0)
        err("Error opening socket");

    // setsockopt for letting us rerun server immediately
    optval = 1;
    setsockopt(serverfd,SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    // build server address
    bzero((char *)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((unsigned short)portNo);

    // bind port
    if (bind(serverfd, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0)
        err("ERROR on binding");

    // listen to make socket ready. putting upto 100 connections in queue
    if (listen(serverfd, 100) < 0)
        err("Error on listen");

    clientlen = sizeof(clientAddr);
    // main loop
    while (1)
    {
        // wait and accept connections
        clientfd = accept(serverfd, (struct sockaddr *)&clientAddr, &clientlen);
        if (clientfd < 0)
        {
            err("Error on accept");
        }
        // crete a child process to handle the rest
        int childPid;
        if ((childPid = fork()) == 0)
        {
            // this is the child process
            // first close the serverfd
            close(serverfd);
            // determine who sent the message
            // gethostbyaddr: determine who sent the message
            hostp = gethostbyaddr((const char *)&clientAddr.sin_addr.s_addr,
                                  sizeof(clientAddr.sin_addr.s_addr), AF_INET);
            if (hostp == NULL)
                err("Error on gethostbyaddr");
            hostaddrp = inet_ntoa(clientAddr.sin_addr);
            if (hostaddrp == NULL)
                err("ERROR on inet_ntoa\n");
            // log
            printf("server established connection with %s (%s)\n", hostp->h_name, hostaddrp);

            // read from client
            bzero(buf, BUFSIZE);
            n = read(clientfd, buf, BUFSIZE);
            if (n < 0)
                err("ERROR reading from socket");
            printf("server received %d bytes: %s\n", n, buf);
            appendMsgToBuffer(buf);
            // write response back to client
            n = write(clientfd, buf, strlen(buf));
            if (n < 0)
                err("ERROR writing to socket");
            close(clientfd);
            exit(EXIT_SUCCESS);
        }else{
            // close the clientfd
            close(clientfd);
            printf("A child process is created with pid %i. Moving on to accept connections\n",childPid);
        }
    }
}