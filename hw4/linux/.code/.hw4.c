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
#include <fcntl.h>

#define HOST_LEN 50
#define PORT_LEN 10
#define FILE_LEN 10
#define URL_LEN 2000
#define BUF_LEN 40000
#define MAX_CLI_NUM 5

#define BUFFER_SIZE 10240

const char *error_file = "Error : '%s' doesn't exist<br>";

typedef enum {
	F_READ,
	F_WRITE,
}status_code;
char buffer[BUFFER_SIZE];

char ip       [MAX_CLI_NUM][HOST_LEN];
char port     [MAX_CLI_NUM][PORT_LEN];
char file     [MAX_CLI_NUM][FILE_LEN];
char proxyip  [MAX_CLI_NUM][HOST_LEN];
char proxyport[MAX_CLI_NUM][PORT_LEN];

FILE *fp[MAX_CLI_NUM];

void replace_special_char(char *mes_buf){
	char cp[BUF_LEN];
	strcpy(cp,mes_buf);

	char result[BUF_LEN];
	bzero(result,BUF_LEN);

	for(int i=0; i<strlen(cp); i++){
		if(cp[i] == '<')
			strcat(result,"&lt;");
		else if(cp[i] == '>')
			strcat(result,"&gt;");
		else if(cp[i] == '\"')
			strcat(result,"\\\"");
		else if(cp[i] == '\n')
			strcat(result,"<br>");
		else if(cp[i] == '\r')
			strcat(result,"");
		else if(cp[i] == ' ')
			strcat(result,"&nbsp;");
		else
			result[strlen(result)] = cp[i];
	}
	strcpy(mes_buf,result);
}
void print_to_html(int i,char *mes_buf){
	printf("<script>document.all['m%d'].innerHTML += \"<b>%s</b>\";</script>\r\n",i,mes_buf);
	fflush(stdout);
}
int readline(int fd, char *ptr, int maxlen){// read from file
    int n=0,rc;
    char c;

    for(n=0; n<maxlen;n++){
        if((rc=read(fd, &c, 1))==1){
            if(c=='\0')
                break;
            *ptr++ = c;
        }
        else if (rc==0){
            if(n==1) return(0);
            else break;
        }
        else
            return(rc);
    }
    *ptr = 0;
    return(strlen(ptr));//string length
}
int readfile(FILE *fp,char *mes_buf){//return strlen or 0
	if(fgets(mes_buf, sizeof(char)*100, fp) == NULL)
		return(0);
	return(strlen(mes_buf));
}
int connect_to_server(int i){//successful return fd, fail return 0
	struct hostent *server_he;
	struct hostent *proxy_he;
	struct sockaddr_in server_in; bzero(&server_in,sizeof(server_in));
	struct sockaddr_in proxy_in;  bzero(&proxy_in, sizeof(proxy_in));
	int    client_fd;
	int one = 1;

	if(ip[i][0]=='\0' || port[i][0]=='\0' || file[i][0]=='\0')
		return(0);

	if((client_fd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("<script>document.all['m%d'].innerHTML += \"socket error<br>\";</script>",i);
		return(0);
	}

	int flag = fcntl(client_fd, F_GETFL, 0);
	fcntl(client_fd, F_SETFL, flag | O_NONBLOCK);
	/*if (ioctl(client_fd, FIONBIO, (char *)&one)){ //set non-blocking error
		printf("<script>document.all['m%d'].innerHTML += \"can't mark socket nonblocking<br>\";</script>",i);
		return(0);
	}*/

	if((server_he=gethostbyname(ip[i])) == NULL )//get service ip error
		printf("<script>document.all['m%d'].innerHTML += \"gethostbyname error<br>\";</script>",i);

	server_in.sin_family = AF_INET;
	server_in.sin_addr = *((struct in_addr *)server_he->h_addr);
	server_in.sin_port = htons((u_short)atoi(port[i]));

	if(proxyip[i][0] !='\0' && proxyport[i][0]!='\0' ){//proxy
		printf("%s,%s<br>",proxyip[i],proxyport[i]);
		if((proxy_he=gethostbyname(proxyip[i])) == NULL ){//get proxy ip error
			printf("<script>document.all['m%d'].innerHTML += \"proxy gethostbyname error<br>\";</script>",i);
			return(0);
		}
		proxy_in.sin_family = AF_INET;
		proxy_in.sin_addr = *((struct in_addr *)proxy_he->h_addr);
		proxy_in.sin_port = htons((u_short)atoi(proxyport[i]));

		if(connect(client_fd,(struct sockaddr *)&proxy_in,sizeof(proxy_in)) == -1){//connect proxy error
			if(errno != EINPROGRESS){
				printf("<script>document.all['m%d'].innerHTML += \"proxy connect error<br>\";</script>",i);
				return(0);
			}
		}

		buffer[0] = 4;
		buffer[1] = 1;
		buffer[2] = (atoi(port[i]) / 256) & 0xff;
		buffer[3] = (atoi(port[i]) % 256) & 0xff;
		buffer[4] = (server_in.sin_addr.s_addr) & 0xff;
		buffer[5] = (server_in.sin_addr.s_addr >> 8) & 0xff;
		buffer[6] = (server_in.sin_addr.s_addr >> 16) & 0xff;;
		buffer[7] = (server_in.sin_addr.s_addr >> 24) & 0xff;
		buffer[8] = 0;
		buffer[9] = 0;
		//printf("%d,2:%d,3:%d,4:%d,5:%d,6:%d,7%s<br>",i,buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);

		while(write(client_fd, buffer, 9) <= 0);//wait send finish
		while(read(client_fd, buffer, 8) <= 0); //wait read finish
		if(buffer[0] != 0 || buffer[1] != 90){
			printf("<script>document.all['m%d'].innerHTML += \"socks reply error<br>\";</script>",i);
			return(0);
		}
	}
	else{//no proxy
		if(connect(client_fd,(struct sockaddr *)&server_in,sizeof(server_in)) == -1){
			printf("<script>document.all['m%d'].innerHTML += \"connect error<br>\";</script>",i);
			//return(0);
		}
	}
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
	char temp[25][HOST_LEN];
	int i = 0, j = 0;

	//init all char array
	for(i=0; i<5; i++){
		bzero(ip[i],       sizeof(ip[i]));
		bzero(port[i],     sizeof(port[i]));
		bzero(file[i],     sizeof(file[i]));
		bzero(proxyip[i],  sizeof(proxyip[i]));
		bzero(proxyport[i],sizeof(proxyport[i]));
	}

	//cut by "&"
	i=0;
	char *t = strtok(buff, "&");
	while(t != NULL){
		strcpy(temp[i++], t);
		t = strtok(NULL, "&");
	}

	//cut by "="
	for(i=0; i<25; i++){
		t = strtok(temp[i],"=");
		t = strtok(NULL,"=");
		if (t == NULL)
			continue;

		switch (i % 5){
			case 0:
				strcpy(ip[j],t);
				break;
			case 1:
				strcpy(port[j],t);
				break;
			case 2:
				strcpy(file[j],t);
				break;
			case 3:
				strcpy(proxyip[j],t);
				break;
			case 4:
				strcpy(proxyport[j++],t);
				break;
		}
	}
}
int main(int argc, char* argv[],char *envp[]){

	int nfds=FD_SETSIZE;//init var
	fd_set rfds,wfds,rs, ws;
	FD_ZERO(&rs);
	FD_ZERO(&ws);
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	int host_num = 0;//0~5
	int Ssockfd[MAX_CLI_NUM]= {0,0,0,0,0};
	int status[MAX_CLI_NUM] = {F_READ, F_READ, F_READ, F_READ, F_READ};
	int read_len = 0;
	int ready_write_len[MAX_CLI_NUM] = {0, 0, 0, 0, 0};
	int haven_write_len[MAX_CLI_NUM] = {0, 0, 0, 0, 0};
	int i = 0;
	int issend[5] = {0, 0, 0, 0, 0};
	char *mes_buf[MAX_CLI_NUM] = {NULL, NULL, NULL, NULL, NULL};
	char temp[BUF_LEN];
	for (i =0 ; i<MAX_CLI_NUM; i++)
		mes_buf[i] = malloc(sizeof(char)*BUF_LEN);
	i = 0;
	int err=0;

	char query[URL_LEN];
    strcpy(query , getenv("QUERY_STRING") );
    //strcpy(query,"?h1=nplinux1.cs.nctu.edu.tw&p1=7780&f1=t1.txt&sh1=p1&sp1=pp1&h2=nplinux1.cs.nctu.edu.tw&p2=7780&f2=t5.txt&sh2=p2&sp2=pp2&h3=nplinux1.cs.nctu.edu.tw&p3=7780&f3=t3.txt&sh3=p3&sp3=pp3&h4=nplinux1.cs.nctu.edu.tw&p4=7780&f4=t5.txt&sh4=p4&sp4=pp4&h5=nplinux1.cs.nctu.edu.tw&p5=7780&f5=t6.txt&sh5=p5&sp5=pp5");

	cut_url(query);
	gen_html();

	// connect to server
	for(int i=0; i<5; i++){
		Ssockfd[i] = connect_to_server(i);
		if(Ssockfd[i] > 0){
			host_num++;
			nfds = Ssockfd[i]+1>nfds? Ssockfd[i]+1:nfds;
			FD_SET(Ssockfd[i],&rs);
			FD_SET(Ssockfd[i],&ws);
			status[i] = F_READ;
			if ((fp[i] = fopen(file[i], "r")) == NULL){
				sprintf(mes_buf[i], error_file ,file[i]);
				print_to_html(i,mes_buf[i]);
			}
		}

	}

	while(host_num){
		memcpy(&rfds, &rs, sizeof(rfds));
		memcpy(&wfds, &ws, sizeof(wfds));

		if(select(nfds, &rfds, &wfds, (fd_set*)0, (struct timeval*)0) < 0){
			printf("select error<br>");
            return(0);
        }

		for(int i=0; i<5; i++){
			if(Ssockfd[i] <= 0)	continue;

			if(     FD_ISSET(Ssockfd[i], &rfds) && status[i] == F_READ && issend[i]==0){//read from server
				bzero(mes_buf[i], BUF_LEN);

				err = readline(Ssockfd[i], mes_buf[i], BUF_LEN-1);

				if(strlen(mes_buf[i]) == 0 || err == 0){
					printf("close fd[%d]<br>",i);
					host_num--;
					FD_CLR(Ssockfd[i],&rs);
					FD_CLR(Ssockfd[i],&ws);
					close(Ssockfd[i]);
					fclose(fp[i]);
					Ssockfd[i] = 0;
					continue;
				}

				if(strstr(mes_buf[i], "% ")!=NULL ){
					status[i] = F_WRITE;
					FD_CLR(Ssockfd[i], &rs);
					FD_SET(Ssockfd[i], &ws);
					issend[i] =1;
				}

				if(strlen(mes_buf[i]) > 0){
					replace_special_char(mes_buf[i]);
					print_to_html(i,mes_buf[i]);
				}
			}
			else if(FD_ISSET(Ssockfd[i], &wfds) && status[i] == F_WRITE ){//write to server
				if(issend[i]==1 && ready_write_len[i] > 0){//still have date need to send
					if(ready_write_len[i] <= haven_write_len[i]){
						issend[i]=0;
						ready_write_len[i] = haven_write_len[i] = 0;
						status[i] = F_READ;
			        	FD_CLR(Ssockfd[i], &ws);
		        		FD_SET(Ssockfd[i], &rs);
		        		continue;
					}
					//printf("issend[%d]=1,haven_write_len[%d] =%d,<br>",i,i,haven_write_len[i]);
					strcpy(temp,mes_buf[i]);

					haven_write_len[i] += write(Ssockfd[i], &temp[haven_write_len[i]],strlen(mes_buf[i]));
				}
				else {//no data need to send, so read file command
					bzero(mes_buf[i], BUF_LEN);
					if( (ready_write_len[i] = readfile(fp[i],mes_buf[i]) ) <= 0 ){
						host_num--;
						FD_CLR(Ssockfd[i],&rs);
						FD_CLR(Ssockfd[i],&ws);
						close(Ssockfd[i]);
						fclose(fp[i]);
						Ssockfd[i] = 0;
						continue;
					}
					else if( ready_write_len[i] > 0 ){
						haven_write_len[i] = 0;
						haven_write_len[i] += write(Ssockfd[i], mes_buf[i],strlen(mes_buf[i]));
						if(ready_write_len[i] <= haven_write_len[i]){
							//printf("send finish[%d]<br>",i);
							issend[i]=0;
							ready_write_len[i] = 0;
							haven_write_len[i] = 0;
							status[i] = F_READ;
				        	FD_CLR(Ssockfd[i], &ws);
			        		FD_SET(Ssockfd[i], &rs);
						}

						replace_special_char(mes_buf[i]);
						print_to_html(i,mes_buf[i]);
					}
				}
			}
		}
	}
    return (0);
}

/*int readfile(FILE *fp,char *mes_buf){//return strlen
	int len=0,t;
	char c[BUF_LEN];
	bzero(c, sizeof(c));
	if(fgets(c, sizeof(c), fp) == NULL)
		return(0);

	strcpy(mes_buf, c);
	len = strlen(c);
	return(len);
}*/