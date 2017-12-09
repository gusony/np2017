#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"
#include <winsock2.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include <string.h>

#pragma comment (lib, "Ws2_32.lib")

#define SERVER_PORT 7787
#define DATA_BUFSIZE 8192
#define WM_SOCKET_NOTIFY (WM_USER + 1)
#define WM_SERVER_NOTIFY (WM_USER + 2)
#define BUF_LEN 40000
#define MAX_CLI_NUM 5
#define HOST_LEN 50
#define PORT_FILE_LEN 10
#define URL_LEN 2000
#define	F_READ 1
#define F_WRITE 2
#define F_ALLEXIT 3

const char *error_file = "Error : '%s' doesn't exist<br>";




typedef struct _SOCKET_INFORMATION {
	BOOL RecvPosted;
	CHAR Buffer[DATA_BUFSIZE];
	WSABUF DataBuf;
	SOCKET Socket;
	DWORD BytesSEND;
	DWORD BytesRECV;
	struct _SOCKET_INFORMATION *Next;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

//=================================================================
//	Global Variables
//=================================================================
char * http_mes_buf = (char *)malloc(sizeof(char)*BUF_LEN);
static HWND hwndEdit;
static SOCKET msock, ssock, httpsocket;

SOCKET host_num = 0;//0~5
char *mes_buf[MAX_CLI_NUM] = { (char*)malloc(sizeof(char)*BUF_LEN), (char*)malloc(sizeof(char)*BUF_LEN), (char*)malloc(sizeof(char)*BUF_LEN), (char*)malloc(sizeof(char)*BUF_LEN), (char*)malloc(sizeof(char)*BUF_LEN) };
char ip[MAX_CLI_NUM][HOST_LEN], port[MAX_CLI_NUM][PORT_FILE_LEN], file[MAX_CLI_NUM][PORT_FILE_LEN];
FILE *fp[MAX_CLI_NUM];
int Ssockfd[MAX_CLI_NUM] = { 0,0,0,0,0 };
int issend[MAX_CLI_NUM] = { 0,0,0,0,0 };
int ready_write_len[MAX_CLI_NUM] = { 0,0,0,0,0 };
int haven_write_len[MAX_CLI_NUM] = { 0,0,0,0,0 };

/************
 * Function**
 ************/
BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf(HWND, TCHAR *, ...);

list<SOCKET> Socks;


void replace_special_char(char *mes_buf) {
	char cp[BUF_LEN];
	strcpy(cp, mes_buf);

	char result[BUF_LEN];
	memset(result, 0, BUF_LEN);

	for (int i = 0; i<strlen(cp); i++) {
		if (cp[i] == '<')
			strcat(result, "&lt;");
		else if (cp[i] == '>')
			strcat(result, "&gt;");
		else if (cp[i] == ' ')
			strcat(result, "&nbsp;");
		else if (cp[i] == '\"')
			strcat(result, "\\\"");
		else if (cp[i] == '\n')
			strcat(result, "<br>");
		else if (cp[i] == '\r')
			strcat(result, "");

		else
			result[strlen(result)] = cp[i];
	}
	strcpy(mes_buf, result);
}
void print_to_html(int i, char *mes_buf) {
	char temp[BUF_LEN];
	sprintf(temp, "<script>document.all['m%d'].innerHTML += \"<b>%s</b>\";</script>\r\n", i, mes_buf);
	send(httpsocket, temp, strlen(temp), 0);
}
int gen_html(void) {
	char temp[10000];
	FILE *fp = fopen("form_get.htm", "r");
	if (fp == NULL) {
		EditPrintf(hwndEdit, "form_get.htm not found\n");
		return(-1);
	}

	while (fgets(temp, 10000, fp) != NULL) {
		send(ssock, temp, strlen(temp), 0);
	}
	fclose(fp);
	return(0);
}
int readline(int fd, char *str, int maxlen) {
	int n, rc;
	char c;
	for (n = 1; n < maxlen; ++n)
	{
		if ((rc = recv(fd, &c, 1, 0)) == 1)
		{
			//if (c == '\n') break;
			//if (c == '\r') continue;
			if (c == '\0') break;
			*str++ = c;
		}
		else if (rc == 0)
		{
			if (n == 1) return 0;
			else break;
		}
		else return rc;
	}
	*str = 0;
	return n;
}
int read_http_request(int fd, char *mes_buf) {
	int isresult = 0;
	char temp[BUF_LEN];
	int n = 0;
	memset(temp, 0, sizeof(char)*BUF_LEN);

	isresult = recv(fd, temp, BUF_LEN - 1, 0);
	EditPrintf(hwndEdit, TEXT("RHR.isresult=%d\n-----\n%s\n-----\n"), isresult, temp);
	if (isresult != -1)
		while (temp[n++] != '\n') {}
	temp[n] = '\0';
	strcpy(mes_buf, temp);
	return(strlen(mes_buf));
}
void gen_cgi_html(void) {
	char temp[100] = "";
	send(httpsocket, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
	send(httpsocket, "Content-Type: text/html\r\n\r\n", strlen("Content-Type: text/html\r\n\r\n"), 0);
	send(httpsocket, "<html>\n", strlen("<html>\n"), 0);
	send(httpsocket, "<head>\n", strlen("<head>\n"), 0);
	send(httpsocket, "<meta http-equiv='Content-Type' content='text/html; charset=big5' />\n", strlen("<meta http-equiv='Content-Type' content='text/html; charset=big5' />\n"), 0);
	send(httpsocket, "<title>Network Programming Homework 3</title>\n", strlen("<title>Network Programming Homework 3</title>\n"), 0);
	send(httpsocket, "</head>\n", strlen("</head>\n"), 0);
	send(httpsocket, "<body bgcolor=#336699>\n", strlen("<body bgcolor=#336699>\n"), 0);
	send(httpsocket, "<font face='Courier New' size=2 color=#FFFF99>\n", strlen("<font face='Courier New' size=2 color=#FFFF99>\n"), 0);
	send(httpsocket, "<table width='800' border='1'>\n", strlen("<table width='800' border='1'>\n"), 0);
	send(httpsocket, "<tr>\n", strlen("<tr>\n"), 0);
	sprintf(temp, "<td>%s</td>\n", ip[0]);
	send(httpsocket, temp, strlen(temp), 0);
	sprintf(temp, "<td>%s</td>\n", ip[1]);
	send(httpsocket, temp, strlen(temp), 0);
	sprintf(temp, "<td>%s</td>\n", ip[2]);
	send(httpsocket, temp, strlen(temp), 0);
	sprintf(temp, "<td>%s</td>\n", ip[3]);
	send(httpsocket, temp, strlen(temp), 0);
	sprintf(temp, "<td>%s</td>\n", ip[4]);
	send(httpsocket, temp, strlen(temp), 0);
	send(httpsocket, "<tr>\n", strlen("<tr>\n"), 0);
	send(httpsocket, "<td valign='top' id='m0'></td>\n", strlen("<td valign='top' id='m0'></td>\n"), 0);
	send(httpsocket, "<td valign='top' id='m1'></td>\n", strlen("<td valign='top' id='m1'></td>\n"), 0);
	send(httpsocket, "<td valign='top' id='m2'></td>\n", strlen("<td valign='top' id='m2'></td>\n"), 0);
	send(httpsocket, "<td valign='top' id='m3'></td>\n", strlen("<td valign='top' id='m3'></td>\n"), 0);
	send(httpsocket, "<td valign='top' id='m4'></td>\n", strlen("<td valign='top' id='m4'></td>\n"), 0);
	send(httpsocket, "</table>\n", strlen("</table>\n"), 0);
	send(httpsocket, "</font>\n", strlen("</font>\n"), 0);
	send(httpsocket, "</body>\n", strlen("</body>\n"), 0);
	send(httpsocket, "</html>\n", strlen("</html>\n"), 0);
	//fflush(stdout);
}
void cut_url(char *buff) {
	char temp[15][HOST_LEN];
	int i = 0, j = 0;

	//init all char array
	for (i = 0; i<5; i++) {
		memset(ip[i], 0, sizeof(ip[i]));
		memset(port[i], 0, sizeof(port[i]));
		memset(file[i], 0, sizeof(file[i]));
		//write_count[i]=0;
	}

	//cut by "&"
	i = 0;
	char *t = strtok(buff, "&");
	while (t != NULL) {
		strcpy(temp[i++], t);
		t = strtok(NULL, "&");
	}

	//cut by "="
	for (i = 0; i<15; i++) {
		t = strtok(temp[i], "=");
		t = strtok(NULL, "=");
		if (t != NULL) {
			switch (i % 3) {
			case 0:
				strcpy(ip[j], t);
				break;
			case 1:
				strcpy(port[j], t);
				break;
			case 2:
				strcpy(file[j++], t);
				break;
			}
		}
	}
}
int connect_to_server(int i, HWND hwnd) {//return fd
	struct hostent *he;
	struct sockaddr_in client_sin;
	int client_fd;
	int one = 1;
	char temp[100];

	if (ip[i][0] == '\0' || port[i][0] == '\0' || file[i][0] == '\0')
		return(-1);


	he = gethostbyname(ip[i]);
	client_fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&client_sin, 0, sizeof(client_sin));
	client_sin.sin_family = AF_INET;
	client_sin.sin_addr = *((struct in_addr *)he->h_addr);
	client_sin.sin_port = htons((u_short)atoi(port[i]));
	if (connect(client_fd, (struct sockaddr *)&client_sin, sizeof(client_sin)) == -1) {
		EditPrintf(hwndEdit, "client: can't connect to server, ec:%d\n", WSAGetLastError());
		sprintf(temp, "<script>document.all['m%d'].innerHTML += \"connect error<br>\";</script>", i);
		send(ssock, temp, strlen(temp), 0);
		return(-1);
	}
	else {
		if (WSAAsyncSelect(client_fd, hwnd, WM_SERVER_NOTIFY, F_READ) == SOCKET_ERROR) {
			EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
			closesocket(Ssockfd[i]);
			WSACleanup();
			return -1;
		}
		else {
			EditPrintf(hwndEdit, "WSAAsyncSelect set successful\n");
		}
	}


	ioctlsocket(client_fd, FIONBIO, (u_long*)&one);

	return (client_fd);
}
int readfile(FILE *fp, char *mes_buf) {//return strlen
	int len = 0, t;
	char c[BUF_LEN];
	memset(c, 0, sizeof(c));
	if (fgets(c, sizeof(c), fp) == NULL)
		return(0);

	/*while (c[strlen(c)] == 0 && c[strlen(c) - 1] == 10 && c[strlen(c) - 2] == 32) {
	t = strlen(c);
	c[t - 1] = 0;
	c[t - 2] = 10;
	}*/
	strcpy(mes_buf, c);
	len = strlen(c);
	return(len);
}
int hw3cgi(HWND hwnd) {
	char *error = (char *)malloc(sizeof(char) *BUF_LEN);
	int temp = 0;

	// connect to server
	for (int i = 0; i<5; i++) {
		Ssockfd[i] = connect_to_server(i, hwnd);
		if (Ssockfd[i] > 0) {
			EditPrintf(hwndEdit, "Ssockfd[%d]=%d\n", i, Ssockfd[i]);
			host_num++;

			if ((fp[i] = fopen(file[i], "r")) == NULL) {
				EditPrintf(hwndEdit, "open file error:%d,%s\n", i, file[i]);
				sprintf(error, error_file, file[i]);
				print_to_html(i, error);
			}

			EditPrintf(hwndEdit, "Ssockfd[%d]=%d,WM_SERVER_NOTIFY,F_READ\n", i, Ssockfd[i]);

		}
	}
	return(0);
}

BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WSADATA wsaData;
	static struct sockaddr_in sa;
	SOCKET Accept;
	LPSOCKET_INFORMATION SocketInfo;
	DWORD RecvBytes;
	DWORD SendBytes;
	DWORD Flags;

	int i = 0;
	int j = 0;
	int read_len = 0;
	int isresult = 0;
	int err;
	
	char temp[10000];
	char buff[BUF_LEN];
	

	switch (Message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_LISTEN:
			WSAStartup(MAKEWORD(2, 0), &wsaData);
			//create master socket
			msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			if (msock == INVALID_SOCKET) {
				EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
				WSACleanup();
				return TRUE;
			}

			err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
				closesocket(msock);
				WSACleanup();
				return TRUE;
			}

			//fill the address info about server
			sa.sin_family = AF_INET;
			sa.sin_port = htons(SERVER_PORT);
			sa.sin_addr.s_addr = INADDR_ANY;

			//bind socket
			err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
				WSACleanup();
				return FALSE;
			}

			err = listen(msock, 2);

			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\r\n"));
				WSACleanup();
				return FALSE;
			}
			else {
				EditPrintf(hwndEdit, TEXT("=== Server START ===\r\n"));
			}

			break;
		case ID_EXIT:
			EndDialog(hwnd, 0);
			break;
		};
		break;
	case WM_CLOSE:
		EditPrintf(hwndEdit, "WM_CLOSE wParam=%d,httpsocket=%d\n", wParam, httpsocket);
		EndDialog(hwnd, 0);
		break;
	case WM_SOCKET_NOTIFY:{
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT: {
			ssock = accept(msock, NULL, NULL);
			Socks.push_back(ssock);
			EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\r\n"), ssock, Socks.size());
			err = WSAAsyncSelect(ssock, hwnd, WM_SOCKET_NOTIFY, FD_READ);
			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
				closesocket(ssock);
				WSACleanup();
				return TRUE;
			}
			break;
		}
		case FD_READ: {//recv http request
						//Write your code for read event here.
			isresult = read_http_request(ssock, http_mes_buf);//return the length of first legal line inclued GET
			if (isresult > 0) {
				if (strstr(http_mes_buf, "GET") != NULL && strstr(http_mes_buf, "form_get.htm") != NULL) {
					httpsocket = ssock;
					if ((err = WSAAsyncSelect(httpsocket, hwnd, WM_SOCKET_NOTIFY, FD_WRITE)) == SOCKET_ERROR) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(ssock);
						WSACleanup();
						return TRUE;
					}
				}
			}
			break;
		}
		case FD_WRITE: {//print form_get html
						//Write your code for write event here
			char *buff = (char*)malloc(sizeof(char)*BUF_LEN);

			//EditPrintf(hwndEdit, "FD_WRITE,socket=%d,%s\n", wParam, http_mes_buf);
			if (strstr(http_mes_buf, "GET") != NULL && strstr(http_mes_buf, "form_get.htm") != NULL) {
				//EditPrintf(hwndEdit, "FD_WRITE:find form_get.htm\n");
				sprintf(buff, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
				send(httpsocket, buff, strlen(buff), 0);
				gen_html();
				closesocket(httpsocket);
			}
			else if (strstr(http_mes_buf, "GET") != NULL && strstr(http_mes_buf, "hw3.cgi") != NULL) {
				httpsocket = ssock;
				if ((err = WSAAsyncSelect(httpsocket, hwnd, WM_SERVER_NOTIFY, F_READ)) == SOCKET_ERROR) {
					//EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
					closesocket(ssock);
					WSACleanup();
					return TRUE;
				}
				EditPrintf(hwndEdit, "FD_WRITE:find hw3.cgi,%s\n", http_mes_buf);
				sscanf(http_mes_buf, "GET %s HTTP/1.1\n", http_mes_buf);
				cut_url(http_mes_buf);
				/*for(j=0;j<5;j++)
				EditPrintf(hwndEdit, "j=%d,ip=%s,port=%s,file=%s\n", j, ip[j],port[j],file[j]);*/
				gen_cgi_html();
				hw3cgi(hwnd);
			}
			free(buff);
			break;
		}
		case FD_CLOSE:
			break;
		};
		break;
	}
	case WM_SERVER_NOTIFY: {
		switch (WSAGETSELECTEVENT(lParam)) {
		case F_READ: {
			for (i = 0; i < MAX_CLI_NUM; i++)
				if (wParam == Ssockfd[i])
					break;
			if (i == 5)
				break;

			memset(mes_buf[i], 0, BUF_LEN);
			read_len = readline(Ssockfd[i], mes_buf[i], BUF_LEN - 1);
			

			if (strstr(mes_buf[i], "% ") != NULL) {
				//EditPrintf(hwndEdit, "find %_ \n");
				WSAAsyncSelect(Ssockfd[i], hwnd, WM_SERVER_NOTIFY, F_WRITE);
				issend[i] = 1;
			}
			if (strlen(mes_buf[i]) > 0) {
				replace_special_char(mes_buf[i]);
				EditPrintf(hwndEdit, "print to html,%s,\n", mes_buf[i]);
				print_to_html(i, mes_buf[i]);
			}

			break;
		}
		case F_WRITE: {
			for (i = 0; i < MAX_CLI_NUM; i++) {
				if (wParam == Ssockfd[i]) {
					if (issend[i] == 1 && ready_write_len[i] > 0) {//still have date need to send
						if (ready_write_len[i] <= haven_write_len[i]) {
							issend[i] = 0;
							ready_write_len[i] = haven_write_len[i] = 0;
							if (WSAAsyncSelect(Ssockfd[i], hwnd, WM_SERVER_NOTIFY, F_READ) == SOCKET_ERROR) {
								EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
								closesocket(Ssockfd[i]);
								WSACleanup();
								return -1;
							}
							continue;
						}
						//printf("issend[%d]=1,haven_write_len[%d] =%d,<br>",i,i,haven_write_len[i]);
						strcpy(temp, mes_buf[i]);

						haven_write_len[i] += send(Ssockfd[i], &temp[haven_write_len[i]], strlen(mes_buf[i]), 0);
					}
					else {//no data need to send, so read file command
						memset(mes_buf[i], 0, BUF_LEN);
						if ((ready_write_len[i] = readfile(fp[i], mes_buf[i])) <= 0) {
							//printf("read the end of file[%d] ,exit<br>",i);
							host_num--;
							if (WSAAsyncSelect(Ssockfd[i], hwnd, WM_SERVER_NOTIFY, F_READ) == SOCKET_ERROR) {
								EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
								closesocket(Ssockfd[i]);
								WSACleanup();
								return -1;
							}
							fclose(fp[i]);
							Ssockfd[i] = 0;
							continue;
						}
						else if (ready_write_len[i] > 0) {
							//printf("1.ready_write_len[%d]=%d<br>",i,ready_write_len[i]);
							haven_write_len[i] = 0;
							haven_write_len[i] += send(Ssockfd[i], mes_buf[i], strlen(mes_buf[i]),0);
							//printf("1.haven_write_len[%d]=%d<br>",i,haven_write_len[i]);
							if (ready_write_len[i] <= haven_write_len[i]) {
								//printf("send finish[%d]<br>",i);
								issend[i] = 0;
								ready_write_len[i] = 0;
								haven_write_len[i] = 0;
								if (WSAAsyncSelect(Ssockfd[i], hwnd, WM_SERVER_NOTIFY, F_READ) == SOCKET_ERROR) {
									EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
									closesocket(Ssockfd[i]);
									WSACleanup();
									return -1;
								}
							}

							replace_special_char(mes_buf[i]);
							print_to_html(i, mes_buf[i]);
						}
					}
					/*
					haven_write_len = 0;						
					haven_write_len += send(Ssockfd[i], mes_buf[i], strlen(mes_buf[i]), 0);						
					replace_special_char(mes_buf[i]);						
					print_to_html(i, mes_buf[i]);						

					if (WSAAsyncSelect(Ssockfd[i], hwnd, WM_SERVER_NOTIFY, F_READ) == SOCKET_ERROR) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(Ssockfd[i]);
						WSACleanup();
						return -1;
					}
					
					}*/
				}
			}
			break;
		}
		case F_ALLEXIT: {
			closesocket(httpsocket);
			break;
		}
		default:
			break;
		}
		break;
	}
	default:
		return FALSE;
	};

	return 1;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}
int EditPrintf(HWND hwndEdit, TCHAR * szFormat, ...)
{
	TCHAR   szBuffer[4096];
	va_list pArgList;

	va_start(pArgList, szFormat);
	wvsprintf(szBuffer, szFormat, pArgList);
	va_end(pArgList);

	SendMessage(hwndEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
	SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer);
	SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
	return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
}