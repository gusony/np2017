#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/uio.h>

#define HOST_LEN 50
#define PORT_FILE_LEN 10
#define URL_LEN 2000

char ip[5][HOST_LEN], port[5][PORT_FILE_LEN], file[5][PORT_FILE_LEN];



int connect_to_server(int i){//return fd
	struct hostent *he;
	struct sockaddr_in client_sin;
	int    client_fd;

	if(ip[i][0]=='\0'){
		printf("ip[%d]=NULL",i);
		return -1;
	}
	if(port[i][0]=='\0'){
		printf("port[%d]=NULL",i);
		return -1;
	}
	if(file[i][0]=='\0'){
		printf("ip[%d]=NULL",i);
		return -1;
	}

	printf("start:%d<br>",i);
	he=gethostbyname(ip[i]);
	client_fd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&client_sin,sizeof(client_sin));
	client_sin.sin_family = AF_INET;
	client_sin.sin_addr = *((struct in_addr *)he->h_addr);
	client_sin.sin_port = htons((u_short)atoi(port[i]));
	if(connect(client_fd,(struct sockaddr *)&client_sin,sizeof(client_sin)) == -1){
		printf("connect error:id=%d<br>",i);
		perror("");
		exit(1);
	}
	printf("connect successful! client_fd=%d<br>", client_fd);
	return (client_fd);
}
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
void cut_url(char *buff){
	char temp[15][HOST_LEN];
	int i = 0, j = 0;

	//init all char array
	for(i=0; i<5; i++){
		strcpy(ip[i],"\0");
		strcpy(port[i],"\0");
		strcpy(file[i],"\0");
	}

	//cut by "&"
	i=0;
	char *t = strtok(buff, "&");
	while(t != NULL){
		strcpy(temp[i++], t);
		t = strtok(NULL, "&");
	}

	//cut by "="
	for(i=0; i<15; i++){
		t = strtok(temp[i],"=");
		t = strtok(NULL,"=");
		if(t != NULL){
			switch (i % 3){
				case 0:
					strcpy(ip[j],t);
					break;
				case 1:
					strcpy(port[j],t);
					break;
				case 2:
					strcpy(file[j++],t);
					break;
			}
		}
	}
}
int main(int argc, char* argv[],char *envp[]){
	int Ssockfd[5];
	FILE *fp[5];

    char query[URL_LEN];
    strcpy(query , getenv("QUERY_STRING") );
    //printf("Content-Type: text/html\n\n");
    //strcpy(query,"h1=nplinux1.cs.nctu.edu.tw&p1=7575&f1=t1.txt&h2=nplinux1.cs.nctu.edu.tw&p2=7575&f2=t2.txt&h3=nplinux1.cs.nctu.edu.tw&p3=7575&f3=t3.txt&h4=nplinux1.cs.nctu.edu.tw&p4=7575&f4=t4.txt&h5=nplinux1.cs.nctu.edu.tw&p5=7575&f5=t5.txt");

	cut_url(query);
	gen_html();
	for(int i=0; i<5; i++){
		// connect to server
		Ssockfd[i] = connect_to_server(i);

		// open file
		fp[i] = fopen(file[i], "r");
		if (fp[i] == NULL)
			printf("Error : '%s' doesn't exist<br>", file[i]);

	}



    return (0);
}
