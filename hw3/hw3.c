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
#include <errno.h>

#define HOST_LEN 50
#define PORT_FILE_LEN 10
#define URL_LEN 2000
#define BUF_LEN 10000

char ip[5][HOST_LEN], port[5][PORT_FILE_LEN], file[5][PORT_FILE_LEN];
FILE *fp[5];
int write_count[5];

char *replaceltgt(char *mes_buf){
	char *cp = malloc(sizeof(char)*BUF_LEN);
	strcpy(cp,mes_buf);

	char *result = malloc(sizeof(char)*BUF_LEN);
	bzero(result,sizeof(char)*BUF_LEN);
	char *t1 = malloc(sizeof(char)*1000);
	char *t2 = malloc(sizeof(char)*1000);

	strcpy(result, mes_buf);

	while(strstr(cp, "<")!=NULL){//find '<'
		if(cp[0] !='<'){
			t1 = strtok(cp,"<");
		    t2 = strtok(NULL,"<");
		    if(t1 != NULL){
	    		strcpy(result,t1);
	    	}
		}
		else {
			t2 = strtok(cp,"<");
		}

    	strcat(result, "&lt;");

    	if(t2 != NULL){
			strcat(result,t2);
		}
		strcpy(cp,result);
    }

    while(strstr(cp, ">")!=NULL){
    	t1 = strtok(cp,">");
	    t2 = strtok(NULL,">");

	    if(t1 != NULL && t1[0] ){
	    	strcpy(result,t1);
    	}
    	strcat(result, "&gt;");
    	if(t2 != NULL){
			strcat(result,t2);
		}
		strcpy(cp,result);
    }
    return(result);
}
int readline(int fd, char *ptr, int maxlen){// read from file
    int n=0,rc;
    char c;
    for(n=0; n<maxlen;n++){
        if((rc=read(fd, &c, 1))==1){
            if(c=='\n' || c=='\0' || c=='\r')
                break;
            *ptr++ = c;
        }
        else if (rc==0){
            if(n==1) return(0);
            else break;
        }
        else
            return(-1);
    }
    *ptr = 0;
    return(n);//string length
}
int readfile(FILE *fp,char *mes_buf){//return strlen
	int len=0;
	char c[1000];
	bzero(c, sizeof(c));
	fgets(c, sizeof(c), fp);
	//if(c[strlen(c)-1] == '\n')
		//c[strlen(c)-1] ='\0';

	strcpy(mes_buf, c);
	len = strlen(c);
	return(len);
}
int connect_to_server(int i){//return fd
	struct hostent *he;
	struct sockaddr_in client_sin;
	int    client_fd;
	int one = 1;

	if(ip[i][0]=='\0'){
		//printf("ip[%d]=NULL",i);
		return -1;
	}
	if(port[i][0]=='\0'){
		//printf("port[%d]=NULL",i);
		return -1;
	}
	if(file[i][0]=='\0'){
		//printf("ip[%d]=NULL",i);
		return -1;
	}


	he=gethostbyname(ip[i]);
	client_fd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&client_sin,sizeof(client_sin));
	client_sin.sin_family = AF_INET;
	client_sin.sin_addr = *((struct in_addr *)he->h_addr);
	client_sin.sin_port = htons((u_short)atoi(port[i]));
	if(connect(client_fd,(struct sockaddr *)&client_sin,sizeof(client_sin)) == -1){
		printf("<script>document.all['m%d'].innerHTML += \"connect error<br>\";</script>",i);
	}
	if (ioctl(client_fd, FIONBIO, (char *)&one))
		printf("<script>document.all['m%d'].innerHTML += \"can't mark socket nonblocking<br>\";</script>",i);

	return (client_fd);
}
void gen_html(void){
	printf("HTTP/1.1 200 OK\r\n");
	printf("Content-Type: text/html\r\n\r\n");
	printf("<html>\n");
	printf("<head>\n");
	printf("<meta http-equiv='Content-Type' content='text/html; charset=big5' />\n");
	printf("<title>Network Programming Homework 3</title>\n");
	printf("</head>\n");
	printf("<body bgcolor=#336699>\n");
	printf("<font face='Courier New' size=2 color=#FFFF99>\n");
	printf("<table width='800' border='1'>\n");
	printf("<tr>\n");
	printf("<td>%s</td>\n",ip[0]);
	printf("<td>%s</td>\n",ip[1]);
	printf("<td>%s</td>\n",ip[2]);
	printf("<td>%s</td>\n",ip[3]);
	printf("<td>%s</td>\n",ip[4]);
	printf("<tr>\n");
	printf("<td valign='top' id='m0'></td>\n");
	printf("<td valign='top' id='m1'></td>\n");
	printf("<td valign='top' id='m2'></td>\n");
	printf("<td valign='top' id='m3'></td>\n");
	printf("<td valign='top' id='m4'></td>\n");
	printf("</table>\n");
	printf("</font>\n");
	printf("</body>\n");
	printf("</html>\n");
	fflush(stdout);
}
void cut_url(char *buff){
	char temp[15][HOST_LEN];
	int i = 0, j = 0;

	//init all char array
	for(i=0; i<5; i++){
		bzero(ip[i],  sizeof(ip[i]));
		bzero(port[i],sizeof(port[i]));
		bzero(file[i],sizeof(file[i]));
		write_count[i]=0;
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
	fd_set rfds,afds;
	FD_ZERO(&afds);
	FD_ZERO(&rfds);

	int host_num =0;
	int nfds=10;
	int Ssockfd[5]={0,0,0,0,0};
	int len;
	char *mes_buf=malloc(sizeof(char)*BUF_LEN);

	char query[URL_LEN];
    strcpy(query , getenv("QUERY_STRING") );
    //strcpy(query,"h1=nplinux1.cs.nctu.edu.tw&p1=7575&f1=t1.txt");

	cut_url(query);
	gen_html();

	for(int i=0; i<5; i++){
		Ssockfd[i] = connect_to_server(i);// connect to server
		//printf("Ssockfd[%d]=%d\n",i,Ssockfd[i]);
		if(Ssockfd[i] > 0){
			host_num++;
			//printf("host_num=%d\n",host_num);
			nfds=Ssockfd[i]+1;
			//printf("nfds=%d\n",nfds);
			FD_SET(Ssockfd[i],&afds);

			fp[i] = fopen(file[i], "r");// open file
			if (fp[i] == NULL)
			    printf("<script>document.all['m%d'].innerHTML += \"Error : '%s' doesn't exist<br>\";</script>",i,file[i]);
		}
	}


	while(host_num){
		fflush(stdout);
		memcpy(&rfds, &afds, sizeof(rfds));

		if(select(nfds, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0){
			printf("select error\n");
            return(0);
        }

		for(int i=0; i<5; i++){
			if(Ssockfd[i]>0 && FD_ISSET(Ssockfd[i], &rfds) ){//
				bzero(mes_buf, BUF_LEN);
				printf("read from server<br>");
				if(readline(Ssockfd[i],mes_buf,BUF_LEN-1)){
					strcpy(mes_buf,replaceltgt(mes_buf));
					printf("read from=%d,%d,%s,<br>",i,strlen(mes_buf),mes_buf);
					if(strstr(mes_buf, "% ")==NULL){
						strcat(mes_buf,"<br>");
					}

					printf("<script>document.all['m%d'].innerHTML += \"<b>%s</b>\";</script>\r\n",i,mes_buf);
					fflush(stdout);
				}

				if(strstr(mes_buf, "% ")!=NULL ){
					bzero(mes_buf, BUF_LEN);

					len = readfile(fp[i],mes_buf);

					printf("in main : len=%d,mes_buf=%s,%d,<br>",len,mes_buf,mes_buf[strlen(mes_buf)-1]);
					if( len <= 0 ){//or read exit
						printf("host_num--<br>");
						host_num--;
						FD_CLR(Ssockfd[i],&rfds);
						close(Ssockfd[i]);
						Ssockfd[i] = 0;
					}
					else if( len>0 ){
						//write to server
						write_count[i] += len;
						printf("write_count[%d]=%d<br>",i,write_count[i]);
						write_count[i] -= write(Ssockfd[i], mes_buf,strlen(mes_buf));
						printf("write_count[%d]=%d<br>",i,write_count[i]);
						printf("write to Ssockfd[%d]=%d,%s,<br>",i,Ssockfd[i],mes_buf);
						//print to html
						mes_buf[strlen(mes_buf)-1] = '\0';
						printf("<script>document.all['m%d'].innerHTML += \"<b>%s</b><br>\";</script>",i,mes_buf);
						fflush(stdout);
					}
					if(strstr(mes_buf, "exit")!=NULL){
						printf("host_num--<br>");
						host_num--;
						FD_CLR(Ssockfd[i],&rfds);
						close(Ssockfd[i]);
						Ssockfd[i] = 0;
					}
				}

			}
		}
	}
    return (0);
}