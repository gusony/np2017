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

	/*
	cd:
		request:
			1:connect command
			2:bind command
		reply:
			90:request granted
			91:request rejected or failed
	*/
}sock4r;


char port[5] = "7657";
char buffer[BUFFER_SIZE];
int n;
int isTimeout = 0;
int readall(int fd, char *str, int maxlen){
	int n, rc;
	char c;
	for(n = 1; n < maxlen; ++n)
	{
		if((rc = read(fd, &c, 1))==1)
		{
			if(c == '\0') break;
			*str++ = c;
		}
		else if(rc == 0)
		{
			if(n == 1) return 0;
			else break;
		}
		else return -1;
	}
	*str = 0;
	return n;
}
int readline(int fd, char *str, int maxlen){
	int n, rc;
	char c;
	for(n = 1; n < maxlen; ++n)
	{
		if((rc = read(fd, &c, 1))==1)
		{
			if(c == '\n') break;
			if(c == '\r') continue;
			*str++ = c;
		}
		else if(rc == 0)
		{
			if(n == 1) return 0;
			else break;
		}
		else return -1;
	}
	*str = 0;
	return n;
}
int setSocket(char *port){
	int fd;
	struct sockaddr_in server_addr;

	//set socket
	if((fd = socket(PF_INET, SOCK_STREAM, 6)) < 0)
		fprintf(stderr,"server: can't open stream socket : %s\n", strerror(errno));

	//init server addr
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = INADDR_ANY;

	int optval = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	//bind socket
	if(bind(fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
		fprintf(stderr,"server: can't bind local address : %s\n", strerror(errno));

	//listen socket
	listen(fd, CLIENT_NUMBERS);
	printf("server: server port is %s, and can handle %d clients. \n", port, CLIENT_NUMBERS);

	return fd;
}
void sig_fork(int sig){
    int status;
	while(waitpid(-1, &status, WNOHANG) <= 0);
}
void alarmHandler(int signal){
	isTimeout = 1;
	printf("client%lu: TIMEOUT\n", getpid());
}
int checkFirewall(sock4r *sr){
	FILE *f = fopen("socks.conf", "r");
	char *mes = (char*)malloc(sizeof(char)*100);
	char firewall[3][100];
	char permitIP[4][4];
	char *t = NULL;
	int i = 0;

	while(fgets(mes, sizeof(char)*100, f) != NULL){
		//	mes = permit c 140.113.*.*
		*(mes+strlen(mes)-1) = 0; //remove '\n' at the end of line
		if(*mes == '#') continue;

		i = 0;
		t = strtok(mes, " ");
		while(t != NULL){
			strcpy(firewall[i++], t);
			t = strtok(NULL, " ");
		}

		i = 0;
		t = strtok(firewall[2], ".");
		while(t != NULL){
			strcpy(permitIP[i++], t);
			t = strtok(NULL, ".");
		}

		//check the firewall
		if(strcmp(firewall[0],"permit") != 0) continue;
		if(firewall[1][0] != ((sr->cd == 0x01)?'c':'b') ) continue;
		if(permitIP[0][0] != '*' && atoi(permitIP[0]) != sr->ip[0]) continue;
		if(permitIP[1][0] != '*' && atoi(permitIP[1]) != sr->ip[1]) continue;
		if(permitIP[2][0] != '*' && atoi(permitIP[2]) != sr->ip[2]) continue;
		if(permitIP[3][0] != '*' && atoi(permitIP[3]) != sr->ip[3]) continue;

		return (1);
	}
	return (0);
}
void client_handler(int browserfd){
	alarm(TIME_OUT);

	unsigned long pid = getpid();
	char cmd[CMD_LENGTH];
	int cmdLength;
	sock4r *sr = (sock4r*)malloc(sizeof(sock4r));  bzero(sr, sizeof(sock4r));
	unsigned char request[8];

	n = read(browserfd, &request, 8);
	if (n != 8 || request[0] != 0x04 || (request[1] != 0x01 && request[1] != 0x02)) {
		printf("client%lu: not socks4 request %d\n", pid, sr->vn);
		return;
	}
	sr->vn = request[0];
	sr->cd = request[1];
	sr->port = (request[2] & 0xff) * 256 + (request[3] & 0xff);
	sr->ip[0] = request[4];
	sr->ip[1] = request[5];
	sr->ip[2] = request[6];
	sr->ip[3] = request[7];

	n = readall(browserfd, sr->id, BUFFER_SIZE-1);


	if(!sr->ip[0] && !sr->ip[1] && !sr->ip[2]){
		fprintf(stderr,"cleint%lu: fail ip\n", pid);
		return;
	}

	//show message
	printf("client%d: <D_IP>   :%u.%u.%u.%u\n", browserfd, sr->ip[0], sr->ip[1], sr->ip[2], sr->ip[3]);
	printf("client%d: <D_PORT> :%u\n", browserfd, sr->port);
	printf("client%d: <Command>:%s\n", browserfd, ((sr->cd == 0x01)? "CONNECT" : "BIND"));
	printf("client%d: <Reply>  :%s\n", browserfd, ((request[1] == 90)? "ACCEPT" : "REJECT"));
	printf("client%d: <Content>:",browserfd);
	for (int k=0; k< ((n<10)?n:10) ;k++)
		printf("0x%X, ",sr->id[k]);
	printf("\n");

	request[0] = 0;
	request[1] =(checkFirewall(sr) == 1) ? 90 : 91;
	if(request[1] == 91){
		write(browserfd, request, 8);
		return;
	}

	signal(SIGALRM, alarmHandler);

	if(sr->cd == 0x01){//connect mode
		//reply
		write(browserfd, request, 8);

		int webfd;
		struct sockaddr_in webaddr;

		if((webfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			printf("client%lu: create socket fail\n", pid);
			return;
		}

		bzero((char*)&webaddr, sizeof(webaddr));
		webaddr.sin_family = AF_INET;
		webaddr.sin_port = htons(sr->port);
		webaddr.sin_addr.s_addr = (sr->ip[3]&0xff)*256*256*256 + (sr->ip[2]&0xff)*256*256 + (sr->ip[1]&0xff)*256 + (sr->ip[0]&0xff);

		if(connect(webfd, (struct sockaddr*)&webaddr, sizeof(webaddr)) < 0){
			printf("client%lu: connect web error %d,%s\n", pid, errno, strerror(errno));
			return;
		}

		int nfds = ((browserfd < webfd) ? webfd : browserfd ) + 1;
		fd_set rfds, afds;
		int isConnect = 2;

		FD_ZERO(&afds);
		FD_SET(browserfd, &afds);
		FD_SET(webfd, &afds);

		while(isConnect && !isTimeout)
		{
			alarm(TIME_OUT);
			FD_ZERO(&rfds);
			memcpy(&rfds, &afds, sizeof(rfds));

			if(select(nfds, &rfds, NULL, NULL, NULL) < 0)
			{
				if(isTimeout)
					close(webfd);
				else
					printf("client%lu: select error \n", pid);
				return;
			}

			if(FD_ISSET(browserfd, &rfds))
			{
				memset(buffer, 0, BUFFER_SIZE);
				n = read(browserfd, buffer, BUFFER_SIZE - 1);
				if(n <= 0)
				{
					printf("client%lu: browser over\n", pid);
					FD_CLR(browserfd, &afds);
					isConnect--;
				}
				else
					n = write(webfd, buffer, n);

			}
			else if(FD_ISSET(webfd, &rfds))
			{
				memset(buffer, 0, BUFFER_SIZE);
				n = read(webfd, buffer, BUFFER_SIZE - 1);
				if(n <= 0)
				{
					printf("client%lu: web over\n", pid);
					FD_CLR(webfd, &afds);
					isConnect--;
				}
				else
				{
					n = write(browserfd, buffer, n);
					buffer[10] = 0;
					//printf("client%lu: web receive=%s\n", pid, buffer);
				}
			}
		}
		close(webfd);
	}//end connect
	else if(sr->cd == 0x02)//bind mode
	{
		int bindfd, ftpfd;
		struct sockaddr_in bindaddr, ftpaddr, tmpaddr;
		int len;

		if((bindfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("client%lu: create socket fail\n", pid);
			return;
		}

		bzero((char*)&bindaddr, sizeof(bindaddr));
		bindaddr.sin_family = AF_INET;
		bindaddr.sin_port = htons(INADDR_ANY);
		bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);

		//bind socket
		if(bind(bindfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) < 0)
		{
			printf("client%lu: bind socket fail\n", pid);
			return;
		}

		//get bind port
		len = sizeof(tmpaddr);
		if(getsockname(bindfd, (struct sockaddr *)&tmpaddr, (socklen_t*)&len) < 0)
		{
			printf("client%lu: getsockname fail\n", pid);
			return;
		}

		//listen
		if(listen(bindfd, 5) < 0)
		{
			printf("client%lu: listen fail\n");
			return;
		}

		//reply
		request[2] = (ntohs(tmpaddr.sin_port) / 256) & 0xff;
		request[3] = (ntohs(tmpaddr.sin_port) % 256) & 0xff;
		request[4] = 0;
		request[5] = 0;
		request[6] = 0;
		request[7] = 0;

		write(browserfd, request, 8);

		//wait connect
		len = sizeof(ftpaddr);
		if((ftpfd = accept(bindfd, (struct sockaddr*)&ftpaddr, (socklen_t*)&len)) < 0)
		{
			printf("client%lu: accept fail\n", pid);
			return;
		}

		printf("client%lu: BIND SUCCESS\n", pid);
		write(browserfd, request, 8);

		int nfds = ((browserfd < ftpfd) ? ftpfd : browserfd ) + 1;
		fd_set rfds, afds;
		int isConnect = 2;

		FD_ZERO(&afds);
		FD_SET(browserfd, &afds);
		FD_SET(ftpfd, &afds);

		while(isConnect && !isTimeout)
		{
			alarm(TIME_OUT);
			FD_ZERO(&rfds);
			memcpy(&rfds, &afds, sizeof(rfds));

			if(select(nfds, &rfds, NULL, NULL, NULL) < 0)
			{
				if(isTimeout)
					close(ftpfd);
				else
					printf("client%lu: select error \n", pid);
				return;
			}

			if(FD_ISSET(browserfd, &rfds))
			{
				memset(buffer, 0, BUFFER_SIZE);
				n = read(browserfd, buffer, BUFFER_SIZE - 1);
				if(n <= 0)
				{
					//printf("client%lu: browser over\n", pid);
					//FD_CLR(browserfd, &afds);
					//isConnect--;
				}
				else
				{
					n = write(ftpfd, buffer, n);
					//buffer[10] = 0;
					//printf("client%lu: browser send: %s\n", pid, buffer);
				}
			}
			else if(FD_ISSET(ftpfd, &rfds))
			{
				memset(buffer, 0, BUFFER_SIZE);
				n = read(ftpfd, buffer, BUFFER_SIZE - 1);
				if(n <= 0)
				{
					//printf("client%lu: ftp over\n", pid);
					//FD_CLR(ftpfd, &afds);
					//isConnect--;
				}
				else
				{
					n = write(browserfd, buffer, n);
					//buffer[10] = 0;
					//printf("client%lu: ftp send=%s\n", pid, buffer);
				}
			}
		}
		close(ftpfd);

	}
	return;

}
int main (int argc, char *argv[], char **envp){
	int serverfd, newsockfd, childpid;
	struct sockaddr_in client_addr;
	char cip[IP_LENGTH];
	char cport[PORT_LENGTH];
	int clilen = 0;

	printf("server: server starting...\n");

	//if user give port
	if(argc == 2)
		strcpy(port, argv[1]);

	//set socket
	serverfd = setSocket(port);

	signal(SIGCHLD, sig_fork);

	while(1){
		printf("server: server is waiting...\n");

		//wait client connect
		clilen = sizeof(client_addr);
		if((newsockfd = accept(serverfd, (struct sockaddr*)&client_addr,(socklen_t *) &clilen)) < 0)
			fprintf(stderr,"server: accept error : %s\n", strerror(errno));
		strcpy(cip, inet_ntoa(client_addr.sin_addr));
		sprintf(cport, "%u", client_addr.sin_port);

		//fork child to handle client
		if((childpid = fork()) < 0)
			fprintf(stderr,"server: fork error : %s\n", strerror(errno));
		else if(childpid == 0) //child
		{
			close(serverfd);
			printf("client%d: <S_IP>   :=%s\n", newsockfd, cip);
			printf("client%d: <S_PORT> :=%s\n", newsockfd, cport);
			client_handler(newsockfd);
			close(newsockfd);
			printf("server: client %d exit\n", getpid());
			return 0;
		}
		else
			close(newsockfd);
	}
}
