#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>    // for wait()
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>		//write
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>

#define MAXLINE               512
#define SERV_TCP_PORT 7575
#define COMMAND_NUM  256
#define MESSAGE_LEN     10000


typedef struct command{
    char   com_str_a[256];
    char   output_str[10000];
    int    output_to;
    int    output_to_bytes;
}command_t;


char inputBuffer[10000];
char welcome_message[] = {"****************************************\n** Welcome to the information server. **\n****************************************\n"};
int sockfd, newsockfd, clilen, clientchildpid,cmdchildpid, total_com_num;
struct sockaddr_in cli_addr, serv_addr;
command_t cmds[256];
/*
//void ident_comm(void);//check the string if the string are correct command
//void cut_pip(char inputbuf[10000],command_t *head);//cut the string reading from client into command by '|'
//void read_dir(char *path);//read the dir/file list
//int  readline(int fd, char *ptr, int maxlen);//convert char array to string from client
//void create_socket(void);
*/

void cut_pip(char inputbuf[10000]){
    //command_t *point = head;
    
    int  i=0,j;
    char temp[100];
    char *test = strtok(inputbuf, "|");
    memset(cmds, 0, sizeof(cmds));
    total_com_num = 0;
    
    /*cut string by '|'*/
    while (test != NULL){
        strncpy(cmds[total_com_num++].com_str_a, test, strlen(test));
        test = strtok(NULL, "|");
    }
    
    
    /* what number after '|'*/
    i=0;
    while(cmds[i++].com_str_a[0]!='\0'){
        cmds[i-1].output_to_bytes = 0;
        
        while(47 < cmds[i].com_str_a[cmds[i-1].output_to_bytes] && cmds[i].com_str_a[cmds[i-1].output_to_bytes] < 58)
            cmds[i-1].output_to_bytes++;
        
        if(cmds[i-1].output_to_bytes >0){
            strncpy(temp, cmds[i].com_str_a, cmds[i-1].output_to_bytes);
            cmds[i-1].output_to = atoi(temp);
        }
    }
    
    /* clear the number from command */
    i=0;
    while(cmds[i++].com_str_a[0]!='\0'){
        for(j=0; cmds[i].com_str_a[cmds[i].output_to_bytes+1+j]!='\0' ; j++)
            cmds[i].com_str_a[j] = cmds[i].com_str_a[cmds[i].output_to_bytes+1+j];
        
        cmds[i].com_str_a[j-1]='\0';
    }
    /*
    i=0;
    while(cmds[i].com_str_a[0]!='\0'){
        printf("i=%d\t",i);
        printf("com_str_a=%s\t",cmds[i].com_str_a);
        printf("output_to=%d\n",cmds[i].output_to);
        i++;
    }*/
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
            //if(c>32 || c==10 || c==13)
            *ptr++ = c;
            if(c=='\n') 
                break;
        }
        else if (rc==0){
            if(n==1) return(0);
            else break;
        }
        else 
            return(-1);
    }
    *ptr = 0;
    return(n);
}

void start_server(int argc,char *argv[]){
    
    /* 1.Socket */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0)
        printf("server : can't open stream socket\n");
    
    
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
}

void exe_cmds(int cmd_index){
    
}
void fork_cmds(void){
    int i,cmd_index;
    
    /*check previout command has output to this command*/
    for(cmd_index=0; cmd_index<total_com_num; cmd_index++){
        for(i=0; i < cmd_index; i++){
            if(cmd_index == i+cmds[i].output_to){
                strcat(cmds[cmd_index].com_str_a, " ");
                strcat(cmds[cmd_index].com_str_a, cmds[i].output_str);
            }
        }
        
        /* fork */
        cmdchildpid
        cmdchildpid=fork();
        if(cmdchildpid<0)  perror("fork error");
        else if(cmdchildpid==0){
            /* exe command */
            exe_cmds(cmd_index);
            exit(1);
        }
            
            
    }
    
    i=0;
    while(cmds[i].com_str_a[0]!='\0'){
        printf("i=%d\tcom_str_a = %s\toutput_to = %d\n",i,cmds[i].com_str_a,cmds[i].output_to);
        i++;
    }
    
    
    
    /* pip output to parent process */
    /* close fork */
}


int main(int argc,char *argv[]){
    int i=0;
    strcpy(inputBuffer,"\0");
    
    
    //read_dir("./bin/");//print dir file list 
    /* prepare environment */

    char *origin_PATH = getenv("PATH");
    char *set_PATH = "./bin";
    setenv("PATH", set_PATH, 1);
    
    
    start_server(argc,argv);
    
    while(1){
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        
        if(newsockfd<0) printf("server : accept error");
        else{
            clientchildpid=fork();
            if(clientchildpid<0)  perror("fork error");
            else if(clientchildpid==0)    break;//child process 
        }
    }
    
    /* child process serve accepted client*/
    printf("newsockfd:%d\n",newsockfd);
    send(newsockfd,welcome_message,sizeof(welcome_message),0);
    while(1){
        readline(newsockfd, inputBuffer, sizeof(inputBuffer));
        
        printf("Get:%s\n",inputBuffer);
        cut_pip(inputBuffer);
        fork_cmds();
        send(newsockfd,inputBuffer,sizeof(inputBuffer),0);
    }
    return(0);
}
/*//store garbage
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
    //else
    //    printf("socket create successful ,%d\n",sockfd);
        
    //strcpy(inputBuffer, "autoremovetag test.html | number |1 list |23 number |456 balabala");
    else{//parent process
        wait(&clientchildpid);
        printf("execvp done\n\n");
        printf("parent process");
    }
    
    clientchildpid=fork();
    if(clientchildpid<0)
        perror("fork error");
    
    else if (clientchildpid == 0){ //child process
        char * execvp_str[] = {"echo", "executed by execvp",">>", "~/abc.txt",NULL};  
        if (execvp("echo",execvp_str) <0 ){  
            perror("error on exec");  
            exit(0);  
        }
    }else{ //parent process
        wait(&clientchildpid);
        printf("execvp done\n\n");
    }
    */
