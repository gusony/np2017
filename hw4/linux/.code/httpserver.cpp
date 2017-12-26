#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <errno.h>
#include <signal.h>

#define CLIENT_NUMBERS 30
#define CMD_LENGTH 15000
#define CMDS_SIZE 30
#define BUFFER_SIZE 10240

using namespace std;

char port[5] = "7575";
const char htmlHeader[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const char fileNotFind[] = "file not found!";

bool endWith(const char* str, const char* end)
{
	if(strlen(str) >= strlen(end))
		return (!strcmp(str + (strlen(str) - strlen(end)), end));
	else
		return false;
}

void split(char arr[CMDS_SIZE][BUFFER_SIZE], int &len, char *str, const char *del)
{
	len = 0;
	string s = string(str);
	int pt = s.find(del);
	while(pt != string::npos)
	{
		strcpy(arr[len++], s.substr(0, pt).data());
		s = s.substr(pt + strlen(del), s.length());
		pt = s.find(del);
	}
	if(!s.empty())
		strcpy(arr[len++], s.data());
}

int readline(int fd, char *str, int maxlen)
{
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

void errexit(const char *message, const char *format)
{
	fprintf(stderr, message, format);
	exit(0);
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
	printf("server: server port is %s. \n", port);

	return fd;
}

void client_handler(int sockfd)
{
	unsigned long pid = getpid();
	char buffer[CMD_LENGTH];
	int n;

	printf("client%lu: commucation start\n", pid);

	n = read(sockfd, buffer, CMD_LENGTH-1);

	char msg[CMDS_SIZE][BUFFER_SIZE];
	int msgc;

	split(msg, msgc, buffer, "\n");
	strcpy(buffer, msg[0]);
	split(msg, msgc, buffer, " ");

	if(!strcmp(msg[0], "GET"))
	{
		strcpy(buffer, msg[1]);
		split(msg, msgc, buffer, "?");
		if(endWith(msg[0], ".htm"))
		{
			FILE *f = fopen(msg[0] + 1, "r");
			if(f != NULL)
			{
				write(sockfd, htmlHeader, strlen(htmlHeader));

				while((n = read(fileno(f), buffer, CMD_LENGTH - 1)) > 0)
					write(sockfd, buffer, strlen(buffer));

				fclose(f);
				return;
			}
		}
		else if(endWith(msg[0], ".cgi"))
		{
			FILE *f = fopen(msg[0] + 1, "r");
			if(f != NULL)
			{
				fclose(f);

				printf("client%lu: forward to CGI.\n", pid);

				if(msgc > 1)
					setenv("QUERY_STRING", msg[1], 1);
				else
					setenv("QUERY_STRING", "", 1);

				setenv("CONTENT_LENGTH", "", 1);
                                setenv("REQUEST_METHOD", "GET", 1);
                                setenv("SCRIPT_NAME", "", 1);
                                setenv("REMOTE_HOST", "", 1);
                                setenv("REMOTE_ADDR", "", 1);
                                setenv("AUTH_TYPE", "", 1);
                                setenv("REMOTE_USER", "", 1);
                                setenv("REMOTE_IDENT", "", 1);

				close(STDIN_FILENO);
				close(STDOUT_FILENO);
				close(STDERR_FILENO);

				dup(sockfd);
				dup(sockfd);
				dup(sockfd);

				close(sockfd);

				if(execl(msg[0] + 1, NULL) < 0)
					errexit("exec error %s\n", strerror(errno));

				return;
			}
		}
	}

	write(sockfd, htmlHeader, strlen(htmlHeader));
	write(sockfd, fileNotFind, strlen(fileNotFind));
}

void reaper(int sig){	while(waitpid(-1,(int*)0, WNOHANG) >= 0); }

int main(int argc, char *argv[]){
	int sockfd, newsockfd, childpid;
	struct sockaddr_in client_addr;
	unsigned int client_len;

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

		//fork child to handle client
		if((childpid = fork()) < 0)
			errexit("server: fork error : %s\n", strerror(errno));
		else if(childpid == 0) //child
		{
			close(sockfd);
			client_handler(newsockfd);
			printf("client%lu: commucation end\n", getpid());
			close(newsockfd);
			return 0;
		}
		else
		{
			close(newsockfd);
			printf ("server: fork finished, cpid = %d\n", childpid);
		}
	}

	close(sockfd);
	return 0;
}
