#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>		//write
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

//#include <ws2tcpip.h>
//#include "winsock2.h"

#define MAXLINE 512
#define SERV_TCP_PORT 7575
int readline(int fd, char *ptr, int maxlen){
    int n,rc;
    char c;
    for(n=1; n<maxlen;n++){
        if((rc=read(fd, &c, 1))==1){
            *ptr++ = c;
            if(c=='\n') break;
        }else if (rc==0){
            if(n==1) return(0);
            else break;
        }else return(-1);
    }
    *ptr = 0;
    return(n);
}

void str_echo(int sockfd){
    int n;
    char line[MAXLINE];

    for(;;){
        n = readline(sockfd, line, MAXLINE);
        if(n==0)
            return;
        else if(n<0)
            printf("str_echo : readline error");

        if(write(sockfd, line, n) !=n)
            printf("str_echo : writen error");
    }
}

int main(int argc,char *argv[]){
    int sockfd, newsockfd, clilen, childpid;
    struct sockaddr_in cli_addr, serv_addr;
    //char pname[9999] = argv[0];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0)
        printf("server : can't open stream socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_TCP_PORT);

    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
        printf("server : can't bind local address");

    listen(sockfd, 5);

    for(;;){
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if(newsockfd<0)
            printf("server : accept error");

        if(childpid = fork()<0){
            printf("server : fork error");
        }
        else if(childpid == 0){
            close(sockfd);
            str_echo(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }
    return 0;
}
