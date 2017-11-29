#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>

#define HOST_LEN 50
#define PORT_FILE_LEN 10
#define URL_LEN 2000
#define BUF_LEN 10000
void gen_html(void){
	printf("Content-Type: text/html\n\n");
	printf("<html>");
	printf("<head>");
	printf("<meta http-equiv='Content-Type' content='text/html; charset=big5' />");
	printf("<title>Network Programming Homework 3</title>");
	printf("</head>");
	printf("<body bgcolor=#336699>");
	printf("<font face='Courier New' size=2 color=#FFFF99>");
	printf("<table width='800' border='1'>");
	printf("<tr>");
	printf("<td>%s</td>",ip[0]);
	printf("<td>%s</td>",ip[1]);
	printf("<td>%s</td>",ip[2]);
	printf("<td>%s</td>",ip[3]);
	printf("<td>%s</td>",ip[4]);
	printf("<tr>");
	printf("<td valign='top' id='m0'></td>");
	printf("<td valign='top' id='m1'></td>");
	printf("<td valign='top' id='m2'></td>");
	printf("<td valign='top' id='m3'></td>");
	printf("<td valign='top' id='m4'></td>");
	printf("</table>");
}
int main(int argc, char* argv[],char *envp[]){
	fd_set rfds,afds;
	FD_ZERO(&afds);

	int nfds;
	int Ssockfd[5]={0,0,0,0,0};
	int len;
	char mes_buf[BUF_LEN];
	char query[URL_LEN];
    strcpy(query , getenv("QUERY_STRING") );
    //strcpy(query,"h1=nplinux1.cs.nctu.edu.tw&p1=7575&f1=t1.txt&h2=nplinux1.cs.nctu.edu.tw&p2=7575&f2=t2.txt");
    gen_html();
    return(0);
}