/*******************************************
 *
 *               client.c
 *
 *  Jonathan Hermans - 100642234
 *  CS3310U - Systems Programming
 *  2020-03-13
 ****************************************/
#include <stdio.h>
#include "lib.h"
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT "55555"
#define IP "127.0.0.1"
#define BUFLEN 1024


void getServerResponse(int sock_descr) 
{
    char buf[BUFLEN];
    memset(buf, '\0', sizeof(buf));
    int msg_len;
    
    if((msg_len = recv(sock_descr, buf, 1024, 0)) <= 0) 
        perror("recv: Error occurred.");
    
    buf[msg_len] = '\0';
    printf("%s", buf);
    fflush(stdout);
}

void getUserInput(int sock_descr) 
{
    char buf[BUFLEN];
    memset(buf, '\0', sizeof(buf));
    
    if(fgets(buf, 1023, stdin) == NULL) 
        return;
    
    if(send(sock_descr, buf, strlen(buf), 0) == -1) 
        perror("Error: Send");
}

void *ginaddr(struct sockaddr *sa) 
{
	// ipv4
    if (sa->sa_family == AF_INET) 
        return &(((struct sockaddr_in*)sa)->sin_addr);
    // or ipv6 
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void checkSocket(int sock_descr) 
{
    int i;
    int fd_max;
    int msg_len;
    fd_set master_fd, read_fds;
	
	
	FD_ZERO(&master_fd);
    FD_ZERO(&read_fds);
    FD_SET(sock_descr, &master_fd);
    FD_SET(STDIN_FILENO, &master_fd);
    fd_max = sock_descr;
    fflush(stdin);

    while(1) 
	{
        read_fds = master_fd;
        if( select(fd_max+1, &read_fds, NULL, NULL, NULL) == -1 ) 
		{
            perror("Error: Select");
            exit(3);
        }
        for(i = 0; i <= fd_max; i++) 
		{
            if(FD_ISSET(i, &read_fds)) 
			{
                if(i == sock_descr) 
                    getServerResponse(sock_descr);
                else 
                    getUserInput(sock_descr);
            }
            fflush(stdin);
            fflush(stdout);	            
        }
    }
}

int main(int argc, char *argv[]) 
{
    int sock_descr;
    char *port;
	char *ip;
    char servAddr[INET6_ADDRSTRLEN];
    struct addrinfo hints, *servinfo, *result;
	
		    
    if(argc != 3) 
	{
        ip = IP;
        port = PORT;
    }
    else 
	{
        ip = argv[1];
        port = argv[2];
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    getaddrinfo(ip, port, &hints, &servinfo);
    
    for(result = servinfo; result != NULL; result = result->ai_next) 
	{
        sock_descr = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        
        if (sock_descr < 0) 
		{
            printf("Error: Socket\n");
            perror("Error: Socket creation");
            continue;
        }

        if(connect(sock_descr, result->ai_addr, result->ai_addrlen) == -1) 
		{
            close(sock_descr);
            perror("Error: Client connection");
            continue;
        }
        break;
    }
    
    if(result == NULL) 
	{
        fprintf(stderr, "Client connection failed \n");
        exit(2);
    }


    inet_ntop(result->ai_family, ginaddr((struct sockaddr *)result->ai_addr), servAddr, sizeof servAddr);
    printf(">>> Connecting to %s \n", servAddr);
    
    // poll socket has infinite loop
    checkSocket(sock_descr);
    freeaddrinfo(servinfo);
    
    return 0;
}
