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
#include "common.h"

using namespace std;

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
int isTimeout = 0, spid;

int setSocket(char *port);
void alarmHandler(int signal);
int checkFirewall(sock4r *sr);

void client_handler(int browserfd)
{
	alarm(TIME_OUT);

	unsigned long pid = getpid();
	char cmd[CMD_LENGTH];
	int cmdLength;
	sock4r *sr = new sock4r();
	unsigned char request[8];

	bzero(sr, sizeof(sock4r));

	//check sock4 request
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

	//get id
	n = readUntilNull(browserfd, sr->id, BUFFER_SIZE -1);

	//TODO
	//get domainname
	if(!sr->ip[0] && !sr->ip[1] && !sr->ip[2])
	{
		printf("cleint%lu: fail ip\n", pid);
		return;
	}

	//TODO
	request[0] = 0;
	request[1] =(checkFirewall(sr) == 1) ? 90 : 91;

	//show message
	printf("client%lu: REQUEST: VN=%d, CD=%d, DST_IP=%u.%u.%u.%u, DST_PORT=%u, ID=%s\n", pid, sr->vn, sr->cd, sr->ip[0], sr->ip[1], sr->ip[2], sr->ip[3], sr->port, sr->id);
	printf("client%lu: REQUEST: mode=%s, reply=%s\n", pid, ((sr->cd == 0x01)? "CONNECT" : "BIND"), ((request[1] == 90)? "ACCEPT" : "REJECT"));

	if(request[1] == 91)
	{
		write(browserfd, request, 8);
		return;
	}

	signal(SIGALRM, alarmHandler);

	if(sr->cd == 0x01)//connect mode
	{
		//reply
		write(browserfd, request, 8);

		int webfd;
		struct sockaddr_in webaddr;

		if((webfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("client%lu: create socket fail\n", pid);
			return;
		}

		bzero((char*)&webaddr, sizeof(webaddr));
		webaddr.sin_family = AF_INET;
		webaddr.sin_port = htons(sr->port);
		webaddr.sin_addr.s_addr = (sr->ip[3]&0xff)*256*256*256 + (sr->ip[2]&0xff)*256*256 + (sr->ip[1]&0xff)*256 + (sr->ip[0]&0xff);

		if(connect(webfd, (struct sockaddr*)&webaddr, sizeof(webaddr)) < 0)
		{
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
					printf("client%lu: browser send: %s\n", pid, buffer);
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
					printf("client%lu: ftp send=%s\n", pid, buffer);
				}
			}
		}
		close(ftpfd);

	}//end bind

	return;
}

void reaper(int sig)
{
	int status;
	while(waitpid(-1, &status, WNOHANG) <= 0);
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

void alarmHandler(int signal)
{
	isTimeout = 1;
	printf("client%lu: TIMEOUT\n", getpid());
}

int checkFirewall(sock4r *sr)
{
	FILE *f = fopen("socks.conf", "r");
	char cmd[CMDS_SIZE][BUFFER_SIZE];
	char ip[CMDS_SIZE][BUFFER_SIZE];
	while((n = readline(fileno(f), buffer, BUFFER_SIZE -1)) > 0)
	{
		if(buffer[0] == '#') continue;

		newSplit(cmd, n, buffer, " ");
		newSplit(ip, n, cmd[2], ".");

		if(cmd[1][0] != ((sr->cd == 0x01)?'c':'b')) continue;

		if(ip[0][0] != '*' && atoi(ip[0]) != sr->ip[0]) continue;

		if(ip[1][0] != '*' && atoi(ip[1]) != sr->ip[1]) continue;

		if(ip[2][0] != '*' && atoi(ip[2]) != sr->ip[2]) continue;

		if(ip[3][0] != '*' && atoi(ip[3]) != sr->ip[3]) continue;

		if(strstr(cmd[0], "permit") != NULL)
			return 1;
		else
		{
			printf("client%lu: REQUEST DENY\n", getpid());
			return 0;
		}
	}

	return 0;

}
int main(int argc, char *argv[], char ** envp){
	int sockfd, newsockfd, childpid;
	struct sockaddr_in client_addr;
	unsigned int client_len;
	char cip[IP_LENGTH];
	char cport[PORT_LENGTH];

	printf("server: server starting...\n");

	//if user give port
	if(argc == 2)
		strcpy(port, argv[1]);

	//set socket
	sockfd = setSocket(port);

	signal(SIGCHLD, reaper);

	while(1)
	{
		client_len = sizeof(client_addr);
		printf("server: server is waiting...\n");

		//wait client connect
		if((newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len)) < 0)
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

	close(sockfd);
	return 0;
}



