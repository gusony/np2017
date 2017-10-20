#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>		//write
#include <ctype.h>
#include <errno.h>
#include <dirent.h>


//#include <ws2tcpip.h>
//#include "winsock2.h"

#define MAXLINE 512
#define SERV_TCP_PORT 7575
typedef struct command{
    char   com_str_a[256];
    int    output_to;
    int    output_to_bytes;
    int    total_com_num;
    struct command *next;
}command_t;

void ident_comm(void);//check the string if the string are correct command
void cut_pip(char inputbuf[10000],command_t *head);//cut the string reading from client into command by '|'
void read_dir(char *path);//read the dir/file list
int  readline(int fd, char *ptr, int maxlen);//convert char array to string from client
void str_echo(int sockfd);
void create_socket(void);



void cut_pip(char inputbuf[10000],command_t *head){
    
    //char [100][100];
    //int  com_num=0;
    //int  output_to[100][2];
    
    
    command_t *point = head;
    
    int  i,j;
    char temp[100];
    char *test = strtok(inputbuf, "|");
    
    /*cut string by '|'*/
    while (test != NULL) {
        if(test!=NULL){
            head->total_com_num++;
            strncpy(point->com_str_a, test, strlen(test));
            point->next = malloc(sizeof(command_t));
            point=point->next;
            point->next=NULL;
            test = strtok(NULL, "|");
        }
    }
    
    
    /* what number after '|'*/
    point=head;
    while(point->com_str_a[0]!='\0'){
        point->output_to_bytes = 0;
        
        while(47 < point->com_str_a[point->output_to_bytes] && point->com_str_a[point->output_to_bytes] < 58)
            point->output_to_bytes++;
        
        if(point->output_to_bytes >0){
            strncpy(temp, point->com_str_a, point->output_to_bytes);
            point->output_to = atoi(temp);
        }
        if(point->next != NULL)
            point=point->next;
    }
    
    /* clear the number from command */
    point=head;
    while(point->com_str_a[0]!='\0'){
        for(j=0; point->com_str_a[point->output_to_bytes+1+j]!='\0' ; j++){
            point->com_str_a[j] = point->com_str_a[point->output_to_bytes+1+j];
        }
        point->com_str_a[j-1]='\0';
        if(point->next != NULL)
            point=point->next;
    }
}

void read_dir(char *path){
    DIR           *d;
    struct dirent *dir;
    if (d = opendir(path)){
        while ((dir = readdir(d)) != NULL){
            printf(".bin/%s\n", dir->d_name);
        }
        closedir(d);
    }
    printf("---------------\n");
}

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
    char message[] = {"****************************************\n** Welcome to the information server. **\n****************************************\n"};
    //char inputBuffer[10000] = "autoremovetag test.html | number |1 list |23 number |456 balabala";
    char inputBuffer[10000] = "autoremovetag test.html | number |1 list |23 number |456 balabala";
    //memset(&inputBuffer,0,sizeof(inputBuffer));
    
    //read_dir("./bin/");//print dir file list 
    //create_socket();
    
    command_t *head = malloc(sizeof(command_t)) ,*point;
    head->next = NULL;
    
    
    
    /* 1.Socket */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
        printf("server : can't open stream socket\n");
        return(0);
    }
    else
        printf("socket create successful ,%d\n",sockfd);
    

    /* 2.bind */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //htonl :convert "ASCII IP" to "int IP"
    if(argc<2)
        serv_addr.sin_port = htons(SERV_TCP_PORT);
    else
        serv_addr.sin_port = htons(atoi(argv[1]));
    
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0) //bind sockfd and serv_addr
        printf("server : can't bind local address\n");
    else
        printf("bind successful\n");
    
    /* 3.listen */
    listen(sockfd, 5);
    printf("listen\n");

    for(;;){
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        
        if(newsockfd<0)
            printf("server : accept error");
        else{
            printf("newsockfd:%d\n",newsockfd);
            send(newsockfd,message,sizeof(message),0);
            while(1){
                readline(newsockfd, inputBuffer, sizeof(inputBuffer));
                //read(newsockfd, inputBuffer, sizeof(inputBuffer));
                //recv(newsockfd,inputBuffer,sizeof(inputBuffer),0);
                printf("Get:%s\n",inputBuffer);
                send(newsockfd,inputBuffer,sizeof(inputBuffer),0);
                cut_pip(inputBuffer,head);
            }
        }
    }
        /*
        if(childpid = fork()<0)
            printf("server : fork error");
        
        else if(childpid == 0){
            close(sockfd);
            str_echo(newsockfd);
            exit(0);
        }
        close(newsockfd);
        */
    
}
