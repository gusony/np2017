#pragma once

#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3

#define WM_SERVER_NOTIFY (WM_USER + 2)
#define MINBUFF 128

typedef struct {
	char *ip;
	int port;
	FILE *fp;
	int sockfd;
	char *cmd;
	int buff_size;
	int NeedWrite;
}Client;

extern int EditPrintf(HWND, TCHAR *, ...);
extern char *query_string;
extern char *buff;
extern Client clients[5];
extern int conn;

void http_read_handler(SOCKET sockfd, HWND hwndEdit);
void http_write_handler(HWND hwnd, SOCKET sockfd, HWND hwndEdit);
void hw3_cgi(HWND hwnd, SOCKET sockfd, HWND hwndEdit);
void writeweb(SOCKET sockfd, int i, char *data, int n, int b_flag);
void WriteFooter(SOCKET sockfd);
void readfile(int i, HWND hwndEdit);