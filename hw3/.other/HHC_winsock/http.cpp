#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"
#include "np_project3.h"

char *query_string = NULL;
char *cmd = NULL, *buff = NULL;

void http_read_handler(SOCKET sockfd, HWND hwndEdit)
{
	unsigned int n, buff_size = MINBUFF;
	char *pch;

	if (cmd != NULL)
	{
		free(cmd);
		cmd = NULL;
	}
	if (buff != NULL)
	{
		free(buff);
		buff = NULL;
	}
	cmd = (char *)malloc(buff_size * sizeof(char));
	buff = (char *)malloc(MINBUFF * sizeof(char));
	if (buff == NULL || cmd == NULL)
	{
		EditPrintf(hwndEdit, TEXT("malloc fail\n"));
		return;
	}
	memset(cmd, 0, buff_size * sizeof(char));
	memset(buff, 0, MINBUFF * sizeof(char));

	while ((n = recv(sockfd, buff, MINBUFF-1, 0)) > 0)
	{
		buff[n] = '\0';
		if ((buff_size - strlen(cmd) - 1) < n)
		{
			buff_size *= 2;
			char * temp_buff = cmd;
			cmd = (char *)malloc(buff_size * sizeof(char));
			if (cmd == NULL)
			{
				EditPrintf(hwndEdit, TEXT("vector malloc fail\n"));
				free(temp_buff);
				return;
			}
			memset(cmd, 0, buff_size * sizeof(char));
			strncpy(cmd, temp_buff, strlen(temp_buff));
			free(temp_buff);
		}

		/* Only process http Request-Line */
		if ((pch = strchr(buff, '\n')) != NULL)
		{
			*pch = '\0';
			while (*(--pch) == '\r')
				*pch = '\0';
			strncat(cmd, buff, strlen(buff));
			break;
		}
		strncat(cmd, buff, n);
	}
	if (n < 0)
		send(sockfd, "shell_server: read error\n", 25, 0);
}

void http_write_handler(HWND hwnd, SOCKET sockfd, HWND hwndEdit)
{
	FILE * fp;
	char *pch;

	/* Only parameters in GET method should be processed */
	if ( cmd != NULL && strlen(cmd) >= 3 && strncmp(cmd, "GET", 3) == 0)
	{
		sprintf(buff, "HTTP/1.1 200 OK\r\n");
		send(sockfd, buff, strlen(buff), 0);

		sprintf(buff, "Connection: close\r\n");
		send(sockfd, buff, strlen(buff), 0);

		pch = strtok(cmd, " ");
		pch = strtok(NULL, " ");
		if (strncmp(pch + 1, "form_get.htm", 12) == 0)
		{
			sprintf(buff, "Content-Type: text/html\r\n\r\n");
			send(sockfd, buff, strlen(buff), 0);
			if ((fp = fopen(pch + 1, "r")) == NULL)
			{
				EditPrintf(hwndEdit, TEXT("can't open %s\n"), pch + 1);
				goto EXIT;
			}
			else
			{
				while (fgets(buff, MINBUFF - 1, fp) != NULL)
					send(sockfd, buff, strlen(buff), 0);
				fclose(fp);
			}
		}
		else if (strstr(pch + 1, ".cgi") != NULL)
		{
			query_string = strchr(pch + 1, '?');
			if (query_string != NULL)
			{
				/* Set HTTP GET method parameter */
				*query_string = '\0';
				query_string++;
			}
			hw3_cgi(hwnd, sockfd, hwndEdit);
		}
EXIT:
		free(cmd);
		cmd = NULL;
	}
}