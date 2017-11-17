#ifndef SOCKET_H

#define SOCKET_H
#include <string>
#include "common.h"

struct client{
    int id;
    int fd;
	char ip[15];
	char port[6];
	char name[NAME_LENGTH];
	int pipes[PIPE_SIZE][2];
	int pipePtr;
	char path[BUFFER_SIZE];

	client();
};
int getNewClientId(bool flags[]);
int findClientIdByFd(int fd, bool flags[], client *clients[]);
void deleteClient(int id, bool flags[], client *clients[]);
void initClient(client *c);

int setSocket(char *port);

int getNewId(int *fds);
void resetClient(client *clients[], int id);
#endif
