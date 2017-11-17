#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include "socket.h"
#include "common.h"

int getNewClientId(bool flags[])
{
	for(int i=0; i<CLIENT_NUMBERS; ++i)
		if(!flags[i]) return i;
	return -1;
}

int findClientIdByFd(int fd,bool flags[], client *clients[])
{
	for(int i=0; i<CLIENT_NUMBERS; ++i)
		if(flags[i] && clients[i]->fd == fd) return i;
	return -1;
}

void deleteClient(int id, bool flags[], client *clients[])
{
	delete clients[id];
	clients[id] = 0;
	flags[id] = false;
}

client::client()
{
	strcpy(name, DEFAULT_NAME);
	for(int i = 0; i < PIPE_SIZE; ++i)
	{
		pipes[i][0] = -1;
		pipes[i][1] = -1;
	}
	pipePtr = 0;
	strcpy(path, DEFAULT_PATH);

	id = -1;
	fd = -1;
	strcpy(ip, "0.0.0.0");
	strcpy(port, "0");
}


int setSocket(char *port)
{
	int fd;
	struct sockaddr_in server_addr;

	//set socket
	if((fd = socket(PF_INET, SOCK_STREAM, 6)) < 0)
		errexit("server: can't open stream socket : %s\n", strerror(errno));
	
	//init server addr
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = INADDR_ANY;

	//bind socket
	if(bind(fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
		errexit("server: can't bind local address : %s\n", strerror(errno));

	//listen socket
	listen(fd, CLIENT_NUMBERS);
	printf("server: server port is %s, and can handle %d clients. \n", port, CLIENT_NUMBERS);

	return fd;	
}


int getNewId(int *fds)
{
	for(int i=0; i<CLIENT_NUMBERS; ++i)
		if(fds[i] <= 0)
			return i;
	return -1;
}

void resetClient(client **clients, int id)
{
	close(clients[id]->fd);
	clients[id]->fd = -1;
	strcpy(clients[id]->ip, "0.0.0.0");
	strcpy(clients[id]->port, "0");
	strcpy(clients[id]->name, DEFAULT_NAME);
	closePipe(clients[id]->pipes, PIPE_SIZE);
	clients[id]->pipePtr = 0;
	strcpy(clients[id]->path, DEFAULT_PATH);
	printf("clear: id=%d\n", id);
}
