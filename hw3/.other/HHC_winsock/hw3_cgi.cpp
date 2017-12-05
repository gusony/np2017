#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"
#include "np_project3.h"

Client clients[5];
int conn;

void WriteHeader(SOCKET sockfd)
{
	char *header =
		"Content-Type: text/html\n\n"
		"<html>\n"
		"<head>\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n"
		"<title>Network Programming Homework 3</title>\n"
		"</head>\n";
	send(sockfd, header, strlen(header), 0);
}

void WriteTable(SOCKET sockfd, Client *clients, int conn)
{
	int i;
	char *table =
		"<body bgcolor=#336699>\n"
		"<font face=\"Courier New\" size=2 color=#FFFF99>\n"
		"<table width=\"800\" border=\"1\">\n"
		"<tr>\n";
	send(sockfd, table, strlen(table), 0);

	for (i = 0; i < conn; i++)
	{
		sprintf(buff, "<td>%s</td>", clients[i].ip);
		send(sockfd, buff, strlen(buff), 0);
	}
	send(sockfd, "</tr>\n", strlen("</tr>\n"), 0);
	send(sockfd, "<tr>\n", strlen("<tr>\n"), 0);
	for (i = 0; i < conn; i++)
	{
		sprintf(buff, "<td valign=\"top\" id=\"m%d\">", i);
		send(sockfd, buff, strlen(buff), 0);
	}
	send(sockfd, "</tr>\n", strlen("</tr>\n"), 0);
	send(sockfd, "</table>\n", strlen("</table>\n"), 0);
}

void WriteFooter(SOCKET sockfd)
{
	char *temp =
		"</font>\n"
		"</body>\n"
		"</html>\n";
	send(sockfd, temp, strlen(temp), 0);
}

void readfile(int i, HWND hwndEdit)
{
	char *pch;

	while (fgets(buff, MINBUFF - 1, clients[i].fp) != NULL)
	{
		if ((clients[i].buff_size - strlen(clients[i].cmd) - 1) < strlen(buff))
		{
			clients[i].buff_size *= 2;
			char * temp_buff = clients[i].cmd;
			clients[i].cmd = (char *)malloc(clients[i].buff_size * sizeof(char));
			if (clients[i].cmd == NULL)
			{
				EditPrintf(hwndEdit, TEXT("malloc fail\n"));
				free(temp_buff);
				return;
			}
			memset(clients[i].cmd, 0, clients[i].buff_size * sizeof(char));
			strncpy(clients[i].cmd, temp_buff, strlen(temp_buff));
			free(temp_buff);
		}

		if ((pch = strchr(buff, '\n')) != NULL)
		{
			strncat(clients[i].cmd, buff, strlen(buff));
			break;
		}
		strncat(clients[i].cmd, buff, strlen(buff));
	}
}

void writeweb(SOCKET sockfd, int i, char *data, int n, int b_flag)
{
	char *pch;
	char http_data[1024] = { 0 }, temp[MINBUFF] = {0};
	int j;

	for (j = 0, pch = http_data; j < n; j++)
	{
		if (data[j] == '<')
		{
			strcat(http_data, "&lt;");
			pch += 4;
		}
		else if (data[j] == '>')
		{
			strcat(http_data, "&gt;");
			pch += 4;
		}
		else if (data[j] == '\n')
		{
			strcat(http_data, "<br>");
			pch += 4;
		}
		else if (data[j] == '"')
		{
			strcat(http_data, "&quot;");
			pch += 6;
		}
		else
		{
			*pch = data[j];
			pch++;
		}
	}
	if (b_flag)
	{
		sprintf(temp, "<script>document.all['m%d'].innerHTML += \"<b>", i);
		send(sockfd, temp, strlen(temp), 0);
	}
	else
	{
		sprintf(temp, "<script>document.all['m%d'].innerHTML += \"", i);
		send(sockfd, temp, strlen(temp), 0);
	}
	send(sockfd, http_data, strlen(http_data), 0);
	if (b_flag)
	{
		sprintf(temp, "</b><br>\";</script>\n");
		send(sockfd, temp, strlen(temp), 0);
	}
	else
	{
		sprintf(temp, "\";</script>\n");
		send(sockfd, temp, strlen(temp), 0);
	}
}

void hw3_cgi(HWND hwnd, SOCKET ssock, HWND hwndEdit)
{
	int i, j, sockfd;
	char *pch, *sch;
	struct sockaddr_in serv_addr;
	struct hostent *he;

	WriteHeader(ssock);

	conn = 0;
	memset(clients, 0, sizeof(Client) * 5);
	for (i = 0; i < 5; i++)
	{
		clients[i].ip = NULL;
		clients[i].cmd = NULL;
	}

	pch = strtok(query_string, "&");
	for (i = 0; i < 5; i++)
	{
		/* if column is empty. ex: h1=& ... */
		if (strlen(pch) <= 3)
		{
			pch = strtok(NULL, "&");
			pch = strtok(NULL, "&");
			pch = strtok(NULL, "&");
			continue;
		}

		clients[conn].ip = pch + 3;

		pch = strtok(NULL, "&");
		clients[conn].port = atoi(pch + 3);

		pch = strtok(NULL, "&");
		if ((clients[conn].fp = fopen(pch + 3, "r")) == NULL)
			EditPrintf(hwndEdit, TEXT("client: can't open file: %s\n"), pch + 3);
		clients[conn].cmd = (char *)malloc(MINBUFF * sizeof(char));
		memset(clients[conn].cmd, 0, MINBUFF * sizeof(char));
		clients[conn].buff_size = MINBUFF;

		conn++;
		pch = strtok(NULL, "&");
	}

	WriteTable(ssock, clients, conn);

	for (i = 0; i < conn; i++)
	{
		if ((he = gethostbyname(clients[i].ip)) == NULL)
		{
			EditPrintf(hwndEdit, TEXT("gethostbyname fail\n"));
			WSACleanup();
			return;
		}

		/*
		* Fill in the stucture "serv_addr" with the address of the
		* server that we want to connect with.
		*/
		ZeroMemory((char *)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
		serv_addr.sin_port = htons(clients[i].port);

		/* Open a TCP socket (an Internet stream socket). */
		sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sockfd == INVALID_SOCKET) {
			EditPrintf(hwndEdit, TEXT("client: can't open stream socket\n"));
			WSACleanup();
			return;
		}

		/* Connect to the server. */
		if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		{
			EditPrintf(hwndEdit, TEXT("client: can't connect to server, ec:%d"), WSAGetLastError());
			closesocket(sockfd);
			WSACleanup();
			return;
		}
		else
		{
			if (WSAAsyncSelect(sockfd, hwnd, WM_SERVER_NOTIFY, F_READING) == SOCKET_ERROR)
			{
				EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
				closesocket(sockfd);
				WSACleanup();
				return;
			}
		}
		clients[i].sockfd = sockfd;

		// If iMode!=0, non-blocking mode is enabled.
		u_long iMode = 1;
		ioctlsocket(sockfd, FIONBIO, &iMode);
	}
}