#include <sys/select.h>
#include <sys/types.h>
#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <map>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 10240
#define CLIENT_NUMBER 5
#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3

#define FILE_DIR "test/"

using namespace std;

string formHostName[CLIENT_NUMBER] = {"h1", "h2", "h3", "h4", "h5"};
string formPortName[CLIENT_NUMBER] = {"p1", "p2", "p3", "p4", "p5"};
string formFileName[CLIENT_NUMBER] = {"f1", "f2", "f3", "f4", "f5"};
string formTextName[CLIENT_NUMBER] = {"m1", "m2", "m3", "m4", "m5"};

void errexit(string msg);
int readline(int fd, char *ptr, int maxlen);
void appendMsg(const char *name, const char *msg, bool isCmd);
void split(char arr[30][BUFFER_SIZE], int &len, const char *str, const char *del);
void initPara(map<string, string> &paras);
int connect(const char *host, const char *port, int &sockfd);

void errexit(string msg)
{
	printf("%s<br>", msg.data());
	exit(errno);
}

int readline(int fd, char *ptr, int maxlen)
{
	int n,rc;
	char c;
	*ptr = 0;
	for(n=1;n<maxlen;n++)
	{
		if((rc=read(fd,&c,1)) == 1)
		{
			*ptr++ = c;
			if(c==' '&& *(ptr-2) =='%'){  break; }
			if(c=='\n')  break;
		}
		else if(rc==0)
		{
			if(n==1)     return 0;
			else         break;
		}
		else
			return -1;
	}
	return n;
}

void appendMsg(const char *name, const char *msg, bool isCmd = false)
{
	char arr[30][BUFFER_SIZE];
	int c, pt;
	string temp;

	split(arr, c, msg, "\n");

	for(int i=0;i<c;++i)
	{
		temp = string(arr[i]);
		while((pt = temp.find('<')) != string::npos)
			temp.replace(pt,1,"&lt;");
		while((pt = temp.find('>')) != string::npos)
			temp.replace(pt,1,"&gt;");
		strcpy(arr[i], temp.data());

		if(isCmd)
		{
			arr[i][strlen(arr[i])-1] = 0;
			printf("<script>document.all['%s'].innerHTML += \"<b>%s</b><br/>\";</script>", name, arr[i]);
		}
		else
			printf("<script>document.all['%s'].innerHTML += \"%s<br/>\";</script>", name, arr[i]);
		fflush(stdout);
	}
}

void split(char arr[30][BUFFER_SIZE], int &len, const char *str, const char *del)
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

void initPara(map<string, string> &paras)
{
	char* raw = getenv("QUERY_STRING");
	char raws[15][BUFFER_SIZE];
	int count=0,temp;
	split(raws, count, raw, "&");
	for(int i=0; i<count; ++i)
	{
		char par[2][BUFFER_SIZE];
		strcpy(par[0], "");
		strcpy(par[1], "");
		split(par, temp, raws[i], "=");
		paras[string(par[0])]=string(par[1]);
	}
}

int connect(const char *host, const char *port, int &sockfd)
{
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		errexit("ERROR: opening socket<br>");

	//set non-blocking
	int flag = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);

	if((server = gethostbyname(host)) == NULL)
		errexit("ERROR: no such host<br>");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(atoi(port));

	int n;
	if ((n = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0)
		if (errno != EINPROGRESS)
			errexit("ERROR: connecting");

	return n;
}

void printOriHtml()
{
	printf("HTTP/1.1 200 OK\r\n");
	printf("Content-Type: text/html\r\n\r\n");
	printf("<html>\n");
	printf("<head>\n");
	printf("<meta http-equiv=\"Content-Type\" content=\"text/html\"; charset=UTF-8/>\n");
	printf("<title>Network Programming Homework 3</title>\n");
	printf("</head>\n");
	printf("<body bgcolor=#336699>\n");
	printf("<font face=\"Courier New\" size=2 color=#FFFF99>\n");
	printf("<table width=\"800\" border=\"1\">\n");
	printf("<tr>\n");
	printf("<td id=\"h1\"></td><td id=\"h2\"></td><td id=\"h3\"></td><td id=\"h4\"></td><td id=\"h5\"></td>\n");
	printf("</tr>\n");
	printf("<tr>\n");
	printf("<td valign=\"top\" id=\"m1\"></td><td valign=\"top\" id=\"m2\"></td><td valign=\"top\" id=\"m3\"></td><td valign=\"top\" id=\"m4\"></td><td valign=\"top\" id=\"m5\"></td>\n");
	printf("</tr>\n");
	printf("</table>\n");
	printf("</body>\n");
	printf("</html>\n");
	fflush(stdout);
}

int main(int argc, char **argv)
{
	map<string, string> paras;
	initPara(paras);
	printOriHtml();

	char fileDir[BUFFER_SIZE];
	FILE *files[CLIENT_NUMBER];
	int clients[CLIENT_NUMBER];
	bool isExit[CLIENT_NUMBER] = {false, false, false, false, false};
	int conn = 0, n;

	//open file
	for(int i=0;i<CLIENT_NUMBER;++i)
	{
		if(!paras[formFileName[i]].empty())
		{
			strcpy(fileDir, FILE_DIR);
			strcat(fileDir, paras[formFileName[i]].data());
			files[i] = fopen(fileDir, "r");
			if(files[i] == NULL)
			appendMsg(formTextName[i].data(), "File open fail.");
		}
		else
			files[i] = NULL;
	}

	//set grid head, connect
	for (int i=0; i<CLIENT_NUMBER; ++i)
	{
		char temp[BUFFER_SIZE];
		clients[i] = -1;
		if(!paras[formHostName[i]].empty())
		{
			strcpy(temp, paras[formHostName[i]].data());
			strcat(temp, ":");

			if(!paras[formPortName[i]].empty())
			{
				strcat(temp, paras[formPortName[i]].data());
				if(files[i] != NULL)
				{
					connect(paras[formHostName[i]].data(), paras[formPortName[i]].data(), clients[i]);
					conn++;
				}
			}/*
			else
			{
				strcat(temp, "80");
				if(files[i] != NULL)
				{
					connect(paras[formHostName[i].data()].data(), "80", clients[i]);
					conn++;
				}
			}*/
			appendMsg(formHostName[i].data(), temp);
		}
	}

	char *noServer = "Server is not exist";
	char *noPort = "Port is not exist";
	char *noFile = "File is not exist";

	//stupid
	for(int i=0; i<CLIENT_NUMBER; ++i)
	{
		if(paras[formHostName[i]].empty())
			appendMsg(formTextName[i].data(), noServer);
		if(paras[formPortName[i]].empty())
			appendMsg(formTextName[i].data(), noPort);
		if(paras[formFileName[i]].empty())
			appendMsg(formTextName[i].data(), noFile);
	}

	fd_set rfds, wfds, rs, ws;
	int nfds = FD_SETSIZE;
	int error;
	socklen_t errorLen = sizeof(error);

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&rs);
	FD_ZERO(&ws);

	for(int i=0; i<CLIENT_NUMBER; ++i)
		if(clients[i] != -1)
		{
			FD_SET(clients[i], &rs);
			FD_SET(clients[i], &ws);
		}

	rfds = rs;
	wfds = ws;

	int status[CLIENT_NUMBER] = {F_CONNECTING, F_CONNECTING, F_CONNECTING, F_CONNECTING, F_CONNECTING};

	int cmdPtr = 0;
	char buffer[BUFFER_SIZE];

	while(conn>0)
	{
		memcpy(&rfds, &rs, sizeof(rfds));
		memcpy(&wfds, &ws, sizeof(wfds));

		if(select(nfds, &rfds, &wfds, (fd_set*)0, (struct timeval*)0 ) < 0)
			errexit("select error");

		for(int i=0; i<CLIENT_NUMBER; ++i)
		{
			if(clients[i] == -1) continue;

			if(status[i] == F_CONNECTING && (FD_ISSET(clients[i], &rfds) || FD_ISSET(clients[i], &wfds)))
			{
				if(getsockopt(clients[i], SOL_SOCKET, SO_ERROR, &error, &errorLen) < 0 || error !=0)
					errexit("getsockopt fail");
				status[i] = F_READING;
				FD_CLR(clients[i], &ws);
			}
			else if(status[i] == F_WRITING && FD_ISSET(clients[i], &wfds))
			{
				bzero(buffer, BUFFER_SIZE);
				n = readline(fileno(files[i]), buffer, BUFFER_SIZE - 1);

				if(n <= 0)
				{
					FD_CLR(clients[i], &ws);
					fclose(files[i]);
					files[i] = NULL;
					close(clients[i]);
					clients[i] = -1;
					conn--;
					status[i] == F_DONE;
					continue;
				}

				if(strstr(buffer, "exit") != NULL) isExit[i] = true;

				appendMsg(formTextName[i].data(), buffer, true);
				//buffer[n - 1] = '\r';
				buffer[n] = '\0';
				//buffer[n + 1] = '\0';
				n = write(clients[i], buffer, strlen(buffer));

				printf("read:%d, send:%d<br>", strlen(buffer), n);

				while(n != strlen(buffer))
				{
					n += write(clients[i], buffer + n, strlen(buffer + n));
					printf("read:%d, send:%d, %s<br>", strlen(buffer), n, buffer);
				}

	            FD_CLR(clients[i], &ws);
		        status[i] = F_READING;
	        	FD_SET(clients[i], &rs);
				sleep(1);
			}
			else if(status[i] == F_READING && FD_ISSET(clients[i], &rfds))
			{
				bzero(buffer, BUFFER_SIZE);
				n = read(clients[i], buffer, BUFFER_SIZE - 1);
				appendMsg(formTextName[i].data(), buffer);

				if(n <= BUFFER_SIZE - 1 && (strstr(buffer, "% ") != NULL || isExit[i]))
				{
					FD_CLR(clients[i], &rs);
					status[i] = F_WRITING;
					FD_SET(clients[i], &ws);
				}
			}

		}//end for

	}//end while

	return 0;
}
