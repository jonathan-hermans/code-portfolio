/*******************************************
 *
 *               server.c
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

#define DEFAULT_PORT "55555"
#define MAX_MSG 12
#define MAX_BACKLOG 5          
#define MAX_CLIENTS 5
#define BUFLEN 1024

struct chatStruct
{
    char* clientMessage;
};

struct clientStruct
{
    int clientId;
    int clientNameTest;    
    int conn_desc;
    char clientName[50];
};


struct chatStruct *history[MAX_MSG] = {NULL};
struct clientStruct *clients[MAX_CLIENTS] = {NULL};

void *get_in_addr(struct sockaddr *sa) 
{
    if (sa->sa_family == AF_INET) 
        return &(((struct sockaddr_in*)sa)->sin_addr);
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void clientMessage(int descr, char *msg) 
{
    if(send(descr, msg, strlen(msg), 0) == -1) 
        perror("Error : Send");        
}

void getNewClient(int new_conn) 
{
	int i;
    struct clientStruct *temp = (struct clientStruct *)malloc(sizeof(struct clientStruct));
	    
	for(i = 0; i < MAX_CLIENTS; i++) 
	{
        if(!clients[i]) 
		{
            temp->clientId = i;
            temp->conn_desc = new_conn;
            temp->clientNameTest = 0;
            clients[i] = temp;
            break;
        }
    }
    
    
}


int getClientId(int connId) 
{
    int i;
    for(i = 0; i < MAX_CLIENTS; i++) 
	{
        if(clients[i]) 
		{
            if(clients[i]->conn_desc == connId) 
                return i;
        }
    }
	return -1;  
}

int getClientConnDesc(char *client) 
{
    int i;
    for(i = 0; i < MAX_CLIENTS; i++) 
	{
        if(clients[i]) 
	{
            if(!strcmp(client, clients[i]->clientName)) 
                return clients[i]->conn_desc;
        }
    }
}

void getClientDetails(char *out_msg) 
{
    char clientName[50];
    int counter = 1;	
    int i;

    
    for(i = 0; i < MAX_CLIENTS; i++) 
	{
        if(clients[i]) 
		{
            memset(clientName, '\0', sizeof(clientName));
            sprintf(clientName, "  [%d] %s \n", counter, clients[i]->clientName);
            strcat(out_msg, clientName);
            counter++;
            fflush(stdin);
        }
    }
}

void getClientName(int new_conn) 
{
    char name[] = "Create your user name: ";
    clientMessage(new_conn, name);
}

void broadcastMessage(int conn, fd_set *temp_fd, int fd_max, int sock_desc, char *msg) 
{
    int i, j;
    struct chatStruct *temp = (struct chatStruct *)malloc(sizeof(struct chatStruct));
    
   	for(i = 0; i < MAX_MSG; i++) 
	{
        if(!history[i]) 
		{
            temp->clientMessage = msg;
            break;
        }
    }    
    
    
    for(j = 0; j <= fd_max; j++) 
	{
        if(FD_ISSET(j, temp_fd)) 
		{
            if(j != sock_desc && j != conn && j != 0) 
			{
                if(send(j, msg, strlen(msg), 0) == -1) 
                    perror("Error: Send");
            }
        }
    }
    
}



int getNewConnection(int sock_desc, fd_set *temp_fd, int fd_max) 
{
    char incoming_IP[INET6_ADDRSTRLEN];
    int new_conn;
    struct sockaddr_storage in_address;

    socklen_t addr_size;	    
    addr_size = sizeof in_address;
    
    if(( new_conn = accept(sock_desc, (struct sockaddr *)&in_address, &addr_size )) < 0) 
        perror("Error: Accept");
        
        
        
    FD_SET(new_conn, temp_fd);
    if(new_conn > fd_max)
        fd_max = new_conn;
        
    inet_ntop(in_address.ss_family, get_in_addr((struct sockaddr *)&in_address), incoming_IP, INET6_ADDRSTRLEN);
    
    getNewClient(new_conn);
    getClientName(new_conn);

    return fd_max;
}

void getServerInput(fd_set *tempfd, int fd_max, int sock_desc) 
{
    char buf[BUFLEN];
    memset(buf, '\0', sizeof(buf));
    if(fgets(buf, 1023, stdin) == NULL)
        return;
    else 
	{
        char serverMsg[1024];
        sprintf(serverMsg, "%s", buf);      
        broadcastMessage(0, tempfd, fd_max, sock_desc, serverMsg);
        fflush(stdin);        
    }
}

void getClientConnection(int sock_descr, int fd_max, int conn, fd_set *temp_fd, int msg_len)
{
    int clientId = getClientId(conn);
    char buf[BUFLEN];
	    
    if (clientId < 0)
    	perror("Error: Client ID");
	
    if(msg_len == 0) 
	{	
        sprintf(buf, "-[%s] : left the chat. \n", clients[clientId]->clientName);
        broadcastMessage(conn, temp_fd, fd_max, sock_descr, buf);
        fflush(stdin);        
    }
    else 
        perror("Error: recv");
}

void setClientName(char *in_msg, int conn, int sock_descr, fd_set *temp_fd, int fd_max) 
{
    int clientId = getClientId(conn);
    char welcome_msg[BUFLEN];
    memset(welcome_msg, '\0', sizeof(welcome_msg));

    strncpy(clients[clientId]->clientName, in_msg, sizeof(in_msg));
    clientMessage(conn, welcome_msg);

    printf("+%s entered chat \n", clients[clientId]->clientName);
    
    broadcastMessage(conn, temp_fd, fd_max, sock_descr, welcome_msg);
    fflush(stdin);
}


void getClientInput(int sock_desc, int fd_max, fd_set *temp_fd, char *in_msg, int conn) 
{
    int clientId = getClientId(conn);
    char out_msg[BUFLEN];
    char chatUser[255];
    
    memset(out_msg, '\0', sizeof(out_msg));
        
    printf("[%s] : %s \n", clients[clientId]->clientName, in_msg);
    sprintf(out_msg, "[%s] : %s \n", clients[clientId]->clientName, in_msg);
    broadcastMessage(conn, temp_fd, fd_max, sock_desc, out_msg);
    fflush(stdin);    
}


void pollSocket(int sock_desc) 
{
    int i;
    int fd_max; 
    int msg_len;  
    fd_set master_fd; 
    fd_set read_fds;  
    FD_ZERO(&master_fd); 
    FD_ZERO(&read_fds); 
    FD_SET(sock_desc, &master_fd); 
    FD_SET(0, &master_fd); 
    fd_max = sock_desc; 
    
    while(1) 
	{
        read_fds = master_fd;
        if( select(fd_max+1, &read_fds, NULL, NULL, NULL) == -1 ) 
	{
            perror("Error: Select");
            exit(4);
        }
        
        for(i = 0; i <= fd_max; i++) 
	{
            if(FD_ISSET(i, &read_fds))
	    {
                if(i == sock_desc)
                    fd_max = getNewConnection(sock_desc, &master_fd, fd_max);   
                else
                {
                    char buf[BUFLEN];
                    memset(buf, '\0', sizeof(buf));
                    if(i == 0)
                        getServerInput(&master_fd, fd_max, sock_desc);
                    else 
			{
                        int clientId = getClientId(i);
                        fflush(stdin);
                        fflush(stdout);
                        if((msg_len = recv(i, buf, 1023, 0)) <= 0) 
			{
                            getClientConnection(sock_desc, fd_max, i, &master_fd, msg_len);
                            close(i);
                            FD_CLR(i, &master_fd);
                            clients[clientId] = NULL;
                        }
                        else 
			{
                            buf[strlen(buf)-1] = '\0';
                            char *in_msg;
                            in_msg = malloc(strlen(buf)+1);
                            memset(in_msg, '\0', sizeof(in_msg));
                            strncpy(in_msg, buf, strlen(buf));

                            if(!clients[clientId]->clientNameTest) 
			    {
                                clients[clientId]->clientNameTest = 1;
                                setClientName(in_msg, i, sock_desc, &master_fd, fd_max);							                                
                            }
                            else 
			   {
                                char temp[50];
                                memset(temp, '\0', sizeof(temp));
                                getClientInput(sock_desc, fd_max, &master_fd, in_msg, i);
                            }
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) 
{
    char *port;
    int sock_descr;  
    int check = 1;
    struct addrinfo hints, *server_info, *result; 
	

    printf(">>> Server Started \n");
			        
	if(argc != 2) 
	{
        printf(">>> Port # is missing: %s [port] \n", argv[0]);
        printf(">>> Using default port %s \n", (char*) DEFAULT_PORT);
        port = DEFAULT_PORT;
        fflush(stdin);        
    }
    else
        port = argv[1];



    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 


    getaddrinfo(NULL, port, &hints, &server_info);
    
    for(result = server_info; result!= NULL; result = result->ai_next) 
	{
        sock_descr = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    
	    if (sock_descr < 0) 
		{
            printf("Creation failed on the socket\n");
            perror("Error: Socket");
            continue;
        }
        if (setsockopt(sock_descr, SOL_SOCKET, SO_REUSEADDR, &check, sizeof check) == -1) 
		{
            perror("Error: Socket Options");
            exit(1);
        } 
        if (bind(sock_descr, result->ai_addr, result->ai_addrlen) == -1) 
		{
            printf("Binding failed on the socket\n");
            perror("Error: Binding");
            continue;
        }
        break;
    }
    
	if(result == NULL) 
	{
        fprintf(stderr, "Server cannot bind \n");
        exit(2);
    }

    if(listen(sock_descr, MAX_BACKLOG) < 0) 
	{
        printf("Server cannot listen \n");
        perror("Error : Listen");
        exit(3);
    }

    freeaddrinfo(server_info); 
    pollSocket(sock_descr);
    fflush(stdin);   
    return 0;

}
