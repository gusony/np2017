#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOST_LEN 50
#define PORT_FILE_LEN 10

char ip[5][HOST_LEN], port[5][PORT_FILE_LEN], file[5][PORT_FILE_LEN];

void cut_url(char *buff){
	char temp[15][HOST_LEN];
	int i = 0,j=0;
	char *t = strtok(buff, "&");

	while(t != NULL){
		strcpy(temp[i++], t);
		t = strtok(NULL, "&");
	}

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
	char *query = getenv("QUERY_STRING");
	cut_url(query);


	gen_html();

    return (0);
}
