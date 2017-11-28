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

char ip[5][HOST_LEN], port[5][PORT_FILE_LEN], file[5][PORT_FILE_LEN];
FILE *fp[5];
int write_count[5];


/*int recv_msg(int from){ // read from server
	char buf[3000];
	int len;
	if((len=readline(from,buf,sizeof(buf)-1)) <0)
		return -1;
	buf[len] = 0;
	printf("%s",buf);	//echo input
	fflush(stdout);
	return len;
}*/
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
	strcpy(c,"");
	fgets(c, sizeof(c), fp);
	if(c)
	strcpy(mes_buf, c);
	printf("readfile:%s,\n",c);
	len = strlen(c);
	printf("readfile:len=%d,\n",len);
	return(len);
}
int connect_to_server(int i){//return fd
	struct hostent *he;
	struct sockaddr_in client_sin;
	int    client_fd;
	int one = 1;

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
	/*
	//printf("<script>document.all['m%d'].innerHTML += \"connect successful!<br>\";</script>",i);
	//printf("connect successful! client_fd=%d<br>", client_fd);
	*/
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
	for(int i=0; i<5; i++)
		printf("%s,%s,%s,\n",ip[i],port[i],file[i]);

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

	cut_url(query);

	gen_html();
	for(int i=0; i<5; i++){
		Ssockfd[i] = connect_to_server(i);// connect to server
		if(Ssockfd[i] > 0){
			nfds=Ssockfd[i] +5 +500;
			FD_SET(Ssockfd[i],&afds);
		}

		fp[i] = fopen(file[i], "r");// open file
		if (fp[i] == NULL)
			printf("<script>document.all['m%d'].innerHTML += \"Error : '%s' doesn't exist<br>\";</script>",i,file[i]);
	}
	time_t t_start = clock();
	while(1){
		if( (clock() - t_start)/CLOCKS_PER_SEC >300){
			printf("close process\n");
			exit(0);
		}

		memcpy(&rfds, &afds, sizeof(rfds));

		if(select(nfds, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0){
            //printf("server: select error<br>");
            continue;
        }

		for(int i=0; i<5; i++){
			if(Ssockfd[i]>0 && FD_ISSET(Ssockfd[i], &rfds)){
				strcpy(mes_buf,"");
				if(readline(Ssockfd[i],mes_buf,BUF_LEN)){
					printf("<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n",i,mes_buf);
					//printf("readline:%d,%s,n\n",i,mes_buf);
					//fflush(stdout);
				}

				if(strstr(mes_buf, "% ")!=NULL){
					strcpy(mes_buf,"");
					len =readfile(fp[i],mes_buf);
					printf("readfile:%d,%s,\n",i,mes_buf);
					if(len <=0){
						close(Ssockfd[i]);
						Ssockfd[i] = 0;
						strcpy(mes_buf,"");
					}
					else if( len>0){
						write_count[i] += len;
						write_count[i] -= write(Ssockfd[i], mes_buf,strlen(mes_buf));
						printf("<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n",i,mes_buf);
						//fflush(stdout);
						strcpy(mes_buf,"");
					}
				}
			}
		}
		sleep(1);
		/*for all client
			// read line from server
			// print to html

			// send command
			// read file
			// write_count +=
			// send to server
			// write_count -=
		*/
		/*
		len =readfile(fp[i],mes_buf);
			if(len >0)
				printf("%d,%d,%s\n",i,len,mes_buf);
		 */
	}



    return (0);
}
