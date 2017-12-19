
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
//#include <iostream>
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

typedef struct {
	unsigned char vn;
	unsigned char cd;
	unsigned int port;
	unsigned char ip[4];
	char id[BUFFER_SIZE];

	/*	cd:
		request:
			1:connect command
			2:bind command
		reply:
			90:request granted
			91:request rejected or failed
	*/
}sock4r;




int main(void){
	unsigned long long i = 1,a,b;
	a = i <<24;
	b = i *256*256*256;
	if(a == b)
		printf("same\n");
	return(0);
}