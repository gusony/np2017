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
#define PORT_FILE_LEN 10
#define URL_LEN 2000
#define BUF_LEN 40000
#define MAX_CLI_NUM 5

const char *error_file = "Error : '%s' doesn't exist<br>";

typedef enum {
	F_READ,
	F_WRITE,
}status_code;


char ip[MAX_CLI_NUM][HOST_LEN], port[5][PORT_FILE_LEN], file[5][PORT_FILE_LEN];
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
        	//printf("%c<br>",c);
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
int readfile(FILE *fp,char *mes_buf){//return strlen
	int len=0,t;
	char c[BUF_LEN];
	bzero(c, sizeof(c));
	if(fgets(c, sizeof(c), fp) == NULL)
		return(0);

	/*while(c[strlen(c)] == 0 && c[strlen(c)-1] == 10 && c[strlen(c)-2] == 32){
    	t = strlen(c);
    	c[t-1] =0;
    	c[t-2] =10;
    }*/
	strcpy(mes_buf, c);
	len = strlen(c);
	return(len);
}
int connect_to_server(int i){//return fd
	struct hostent *he;
	struct sockaddr_in client_sin;
	int    client_fd;
	int one = 1;

	if(ip[i][0]=='\0' || port[i][0]=='\0' || file[i][0]=='\0')
		return(0);


	he=gethostbyname(ip[i]);
	client_fd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&client_sin,sizeof(client_sin));
	client_sin.sin_family = AF_INET;
	client_sin.sin_addr = *((struct in_addr *)he->h_addr);
	client_sin.sin_port = htons((u_short)atoi(port[i]));
	if(connect(client_fd,(struct sockaddr *)&client_sin,sizeof(client_sin)) == -1){
		printf("<script>document.all['m%d'].innerHTML += \"connect error<br>\";</script>",i);
	}

	//int flag = fcntl(client_fd, F_GETFL, 0);
	//fcntl(client_fd, F_SETFL, flag | O_NONBLOCK);

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
		//write_count[i]=0;
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
    //strcpy(query,"h1=nplinux1.cs.nctu.edu.tw&p1=7790&f1=t6.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5=");

	cut_url(query);
	gen_html();

	// connect to server
	for(int i=0; i<5; i++){
		Ssockfd[i] = connect_to_server(i);
		if(Ssockfd[i] > 0){
			host_num++;
			nfds=Ssockfd[i]+1;
			FD_SET(Ssockfd[i],&rs);
			FD_SET(Ssockfd[i],&ws);
			sprintf(mes_buf[i], error_file ,file[i]);

			if ((fp[i] = fopen(file[i], "r")) == NULL){//fp[i] = fopen(file[i], "r");
				print_to_html(i,mes_buf[i]);
			}
			status[i] = F_READ;
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
					//printf("find %%_<br>");
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
				//printf("issend[%d]=%d,read=%d,have=%d<br>",i,issend[i],ready_write_len[i],haven_write_len[i]);
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
						//printf("read the end of file[%d] ,exit<br>",i);
						host_num--;
						FD_CLR(Ssockfd[i],&rs);
						FD_CLR(Ssockfd[i],&ws);
						close(Ssockfd[i]);
						fclose(fp[i]);
						Ssockfd[i] = 0;
						continue;
					}
					else if( ready_write_len[i] > 0 ){
						//printf("1.ready_write_len[%d]=%d<br>",i,ready_write_len[i]);
						haven_write_len[i] = 0;
						haven_write_len[i] += write(Ssockfd[i], mes_buf[i],strlen(mes_buf[i]));
						//printf("1.haven_write_len[%d]=%d<br>",i,haven_write_len[i]);
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