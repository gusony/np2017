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
    char   com_str[256][256];
    int    para_len;
    //char   para[256][256];
    
    char   output_str[10000];
    int    output_to;
    int    output_to_bytes;
}command_t;


char inputBuffer[10000];
char outputBuffer[10000];
char welcome_message[] = {"****************************************\n** Welcome to the information server. **\n****************************************\n"};
int sockfd, newsockfd, clilen, clientchildpid,cmdchildpid, total_com_num;
struct sockaddr_in cli_addr, serv_addr;
command_t cmds[256];
int pipe_fd[2];

/*
//void ident_comm(void);//check the string if the string are correct command
//void cut_pip(char inputbuf[10000],command_t *head);//cut the string reading from client into command by '|'
//void read_dir(char *path);//read the dir/file list
//int  readline(int fd, char *ptr, int maxlen);//convert char array to string from client
//void create_socket(void);
*/

void ptfallcmd(void){
    int i=0,j;
    printf("------------------------------\n");
    while(cmds[i].com_str[0][0]!='\0'){
        printf("i=%d,",i);
        printf("output_to=%d,",cmds[i].output_to);
        printf("com_str=");
        for(j=0; j<5; j++) //cmds[i].para_len
            printf("%s,",cmds[i].com_str[j]);
        printf("\n");
        i++;
    }
}
    
void cut_pip(char inputbuf[10000]){
    int  i=0,j,k,space_index;
    char temp[100];
    char *test = strtok(inputbuf, "|");
    memset(cmds, 0, sizeof(cmds));
    total_com_num = 0;
    
    /*cut string by '|'*/
    while (test != NULL){
        strncpy(cmds[total_com_num++].com_str[0], test, strlen(test));
        test = strtok(NULL, "|");
    }
    
    /* what number after '|'*/
    i=0;
    while(cmds[i++].com_str[0][0]!='\0'){
        cmds[i].output_to_bytes = 0;
        
        while(47 < cmds[i].com_str[0][cmds[i].output_to_bytes] && cmds[i].com_str[0][cmds[i].output_to_bytes] < 58)
            cmds[i].output_to_bytes++;
        
        if(cmds[i].output_to_bytes >0){
            strncpy(temp, cmds[i].com_str[0], cmds[i].output_to_bytes);
            cmds[i-1].output_to = atoi(temp);
        }
    }
    
    
    /* clear the number from command */
    i=0;
    while(cmds[i++].com_str[0][0]!='\0'){
        for(j=0; cmds[i].com_str[0][cmds[i].output_to_bytes+1+j]!='\0'; j++)
            cmds[i].com_str[0][j] = cmds[i].com_str[0][cmds[i].output_to_bytes+1+j];
        
        /* 4 line after this line, to clear the space in the end of line*/
        if(cmds[i].com_str[0][j-1]==' ')
            cmds[i].com_str[0][j-1]='\0';
        else
            cmds[i].com_str[0][j]='\0';
    }
    
    ptfallcmd();
    /* split command and para */
    i=0;
    while(cmds[i].com_str[0][0]!='\0'){
        j=0;k=0;space_index = 0;
        
        while(cmds[i].com_str[0][++space_index] != ' ' && space_index<strlen(cmds[i].com_str[0]));//find the first ' '
        if(cmds[i].com_str[0][space_index+1] != '\0' && cmds[i].com_str[0][space_index+1] != ' ')
            for(j=space_index;j<strlen(cmds[i].com_str[0]);j++)
                if(cmds[i].com_str[0][j] == ' '){
                    cmds[i].para_len++;
                    k=0;
                }
                else
                    cmds[i].com_str[cmds[i].para_len][k++]=cmds[i].com_str[0][j];

        cmds[i].com_str[0][space_index]='\0';
        i++;
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
            //if(c>32 || c==10 || c==13)
            
            if(c=='\n') 
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

void  parse(int cmd_index, char **argv){
    char temp[256]="";
    char *line;
    /*strcat(temp,cmds[cmd_index].com_str);
    if(cmds[cmd_index].para[0] != '\0'){
        strcat(temp," ");
        strcat(temp,cmds[cmd_index].para);
    }*/
    line = &temp[0];
    
    while (*line != '\0') {       /* if not the end of line ....... */ 
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0';     /* replace white spaces with 0    */
        *argv++ = line;          /* save the argument position     */
        while (*line != '\0' && *line != ' ' && 
            *line != '\t' && *line != '\n') 
        line++;             /* skip the argument until ...    */
    }
    *argv = '\0';                 /* mark the end of argument list  */
}

void exe_cmds(int cmd_index){
    char * execvp_str[ cmds[cmd_index].para_len +2 ];
    int i;
    
    execvp_str[0] = &(cmds[cmd_index].com_str[0][0]);
    //strcpy(execvp_str[0], cmds[cmd_index].com_str[0]);
    
    for(i=1; i<=cmds[cmd_index].para_len; i++)
        execvp_str[i] = &cmds[cmd_index].com_str[i][0];
    execvp_str[i]=NULL;
    
    
    /* all commands */    
    if(     strcmp(cmds[cmd_index].com_str[0],"ls")==0){
        if (execvp("ls",execvp_str) <0 ){
            perror("error on exec");
            exit(0);
        }
    }
    else if(strcmp(cmds[cmd_index].com_str[0],"cat")==0){
        if (execvp("cat",execvp_str) <0 ){
            perror("error on exec");
            exit(0);
        }
    }
    else if(strcmp(cmds[cmd_index].com_str[0],"removetag")==0){}
    else if(strcmp(cmds[cmd_index].com_str[0],"removetag0")==0){}
    else if(strcmp(cmds[cmd_index].com_str[0],"number")==0){}
    else if(strcmp(cmds[cmd_index].com_str[0],"noop")==0){}
    else if(strcmp(cmds[cmd_index].com_str[0],"date")==0){}
    else if(strcmp(cmds[cmd_index].com_str[0],"printenv")==0){
        printf("PATH=%s\n",getenv("PATH"));
    }
    else if(strcmp(cmds[cmd_index].com_str[0],"setenv")==0){
        setenv(cmds[cmd_index].com_str[1],cmds[cmd_index].com_str[2], 1);
        //printf("PATH=%s\n",getenv(cmds[cmd_index].com_str[1]));
    }
    else{
        printf("Unknown command: [%s].\n",cmds[cmd_index].com_str[0]);
    }
        
}

void fork_cmds(void){
    int i,cmd_index;
    char temp[10000];
    char *test;
    /* check each commands */
    for(cmd_index=0; cmd_index<total_com_num; cmd_index++){
        /*check previout command if  has output to this command*/    
        /*for(i=0; i < cmd_index; i++){
            if(cmd_index == i+cmds[i].output_to){
                strcpy(temp,cmds[i].output_str);
                test = strtok(temp, " ");
                while(test!=NULL){
                    strcpy(cmds[i].com_str[cmds[i].para_len++] ,test);
                    test = strtok(NULL," ");
                }
            }
        }*/
        
        /* create pipeline */
        if (pipe(pipe_fd) == -1){/* 建立 pipe */
            fprintf(stderr, "Error: Unable to create pipe.\n");
            exit(EXIT_FAILURE);
        }
        
        /* fork */
        cmdchildpid=fork();
        
        if(cmdchildpid<0)  
            perror("fork error");
        else if(cmdchildpid==0){
            
            close(pipe_fd[0]);//close read
            //if(cmds[cmd_index].output_to>0)
                dup2(pipe_fd[1], STDOUT_FILENO);//redirect
            close(pipe_fd[1]);//close write
                
            /* exe command */
            exe_cmds(cmd_index);
            
            exit(1);
        }
        else{//parent process
            close(pipe_fd[1]);//close write
            read(pipe_fd[0],&cmds[cmd_index].output_str,sizeof(cmds[cmd_index].output_str));
            printf("1.%s,\n",cmds[cmd_index].output_str);
            if(cmds[cmd_index].output_to==0)
                strcat(outputBuffer,cmds[cmd_index].output_str);
            wait(&cmdchildpid);
            close(pipe_fd[0]);
        }
    }
}

int main(int argc,char *argv[]){
    int i=0;
    strcpy(inputBuffer,"\0");
    
    /* prepare environment */
    char *origin_PATH = getenv("PATH");
    char *set_PATH = "./bin:./";
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
        strcpy(outputBuffer,"\0");//clear buffer
        memset(cmds,0,sizeof(cmds));
        send(newsockfd,"% ",sizeof("% "),0);
        //write(newsockfd,"% ",sizeof("% "));
        readline(newsockfd, inputBuffer, sizeof(inputBuffer));
        printf("Get:%s\n",inputBuffer);
        cut_pip(inputBuffer);
        
        if(strstr(cmds[0].com_str[0],"exit")!=NULL) break;
        fork_cmds();
        send(newsockfd,outputBuffer,sizeof(outputBuffer),0);
        //write(newsockfd,outputBuffer,sizeof(outputBuffer));
    }
    close(newsockfd);
    exit(0);
    return(0);
}
/*//store garbage
    
            cmds[i].com_str[i][++space_index]!=' ' && space_index<strlen(cmds[i].com_str[0])){
            while()
        
        
        j=0;
        space_index = 0;
        for(k=0; k<strlen(cmds[i].com_str[0]); k++){
            while(cmds[i].com_str[0][++space_index] != ' ' && space_index<strlen(cmds[i].com_str[0]));//find the space in com_str
            for(j=0; j+space_index<strlen(cmds[i].com_str[0]); j++)
                cmds[i].para[j]=cmds[i].com_str[space_index+1+j];
            cmds[i].com_str[space_index]='\0';
        }
        i++;
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
