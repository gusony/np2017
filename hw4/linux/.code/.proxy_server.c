/*
this program is a proxy server
*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <string>
#include <iostream>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>

#define CLIENT_NUMBERS 30
#define CMD_LENGTH 15000
#define CMDS_SIZE 5000
#define BUFFER_SIZE 1024
#define PIPE_SIZE 1001
#define BIN_FILES_SIZE 20
#define PUBLIC_PIPE_SIZE 101
#define IP_LENGTH 16
#define PORT_LENGTH 6
#define NAME_LENGTH 21
#define TIME_OUT 6000

#define DEFAULT_PATH "bin:."
#define DEFAULT_DIR "ras"
#define DEFAULT_NAME "(no name)"

#define SHM_CLIENT_TO_SERVER_CMD 31464
#define SHM_CLIENT_TO_SERVER_PARA 31465
#define SHM_CLIENT_TO_SERVER_PARA2 31466
#define SHM_CLIENT_TO_SERVER_PARA3 31467

#define SHM_FD_KEY 18147
#define SHM_IP_KEY 18148
#define SHM_PORT_KEY 18149
#define SHM_NAME_KEY 18150

struct sock4r{
	unsigned char vn;
	unsigned char cd;
	unsigned int port;
	unsigned char ip[4];
	char id[BUFFER_SIZE];
};


char port[5] = "7575";
char buffer[BUFFER_SIZE];
int n;
int isTimeout = 0;

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


	int optval = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	//bind socket
	if(bind(fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
		errexit("server: can't bind local address : %s\n", strerror(errno));

	//listen socket
	listen(fd, CLIENT_NUMBERS);
	printf("server: server port is %s, and can handle %d clients. \n", port, CLIENT_NUMBERS);

	return fd;
}
void sig_fork(int sig)
{
    int status;
	while(waitpid(-1, &status, WNOHANG) <= 0);
}
void client_handler(int browserfd)
{
	alarm(TIME_OUT);

	unsigned long pid = getpid();
	char cmd[CMD_LENGTH];
	int cmdLength;
	sock4r *sr = new sock4r();  bzero(sr, sizeof(sock4r));
	unsigned char request[8];


}
int main (int argc, char *argv[], char **envp){
	int serverfd, newsockfd, childpid;
	struct sockaddr_in client_addr;

	char cip[IP_LENGTH];
	char cport[PORT_LENGTH];

	printf("server: server starting...\n");

	//if user give port
	if(argc == 2)
		strcpy(port, argv[1]);

	//set socket
	sockfd = setSocket(port);

	signal(SIGCHLD, reaper);

	while(1){
		printf("server: server is waiting...\n");

		//wait client connect
		if((newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr))) < 0)//client_len = sizeof(client_addr);
			errexit("server: accept error : %s\n", strerror(errno));

		strcpy(cip, inet_ntoa(client_addr.sin_addr));
		sprintf(cport, "%u", client_addr.sin_port);

		//fork child to handle client
		if((childpid = fork()) < 0)
			errexit("server: fork error : %s\n", strerror(errno));
		else if(childpid == 0) //child
		{
			close(sockfd);
			printf("client%lu: FROM: ip=%s, port=%s\n", getpid(), cip, cport);
			client_handler(newsockfd);
			close(newsockfd);
			printf("server: client %d exit\n", getpid());
			return 0;
		}
		else
			close(newsockfd);
	}
}