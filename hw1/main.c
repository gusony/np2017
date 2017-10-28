/*  
    By Junhan Lin 0646001
    Network programing Homework1
    RAS(remote acess system)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>    // for wait()
#include <arpa/inet.h>  //inet_addr
#include <unistd.h>     //write
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>

#define MAXLINE               512
#define SERV_TCP_PORT 7575
#define COMMAND_NUM  256
#define MESSAGE_LEN     10000
#define total_command_number 5000


typedef struct command{
    char   *com_str[50];
    int    para_len;
    int    output_to;
    int    output_to_bytes;
    char   output_file[30];
}command_t;

command_t cmds[5000];
int *pipe_fd[total_command_number];
int  inputflag[total_command_number];
char dir_list[100][100];
char dir_str[100]="\0";


void ptfallcmd(int total_com_num);
int  cut_pip(char inputbuf[10000],int cmd_count);
int  readline(int fd, char *ptr, int maxlen);
void start_server(int argc,char *argv[],int *sockfd);
int  fork_cmds(int newsockfd, int total_com_num, int cmd_count);
int  check_legal_cmd(char *cmd);

void ptfallcmd(int total_com_num){
    int i=0,j=0;
    printf("------------------------------\n");
    for(i=0; i<total_com_num ; i++ ){
        printf("index=%d,",i);
        printf("output_to=%d,",cmds[i].output_to);
        printf("para_len=%d,",cmds[i].para_len);
        printf("com_str=");
        for(j=0; j<cmds[i].para_len; j++) 
            printf("%s,",cmds[i].com_str[j]);
        printf("> %s,\n",cmds[i].output_file);
    }
    printf("------------------------------\n");
}

int cut_inbuf(char inputbuf[MESSAGE_LEN],int cmd_count){
    int temp_cmd_num=0;
    int flag_cmd_after_pipeline=0;
    char *t = strtok(inputbuf, " ");//printf("first t=%s\n",t);
    
    while(t != NULL && t[0]>31){
        if(t[0] == '|'){
            t[0] = '0';
            cmds[temp_cmd_num].output_to=atoi(t);
            if(cmds[temp_cmd_num].output_to==0)
                 cmds[temp_cmd_num].output_to = 1;
            temp_cmd_num++;
            flag_cmd_after_pipeline=0;
        }
        else {
            if(strcmp(t,">")==0){
                t = strtok(NULL, " ");
                strcpy(cmds[temp_cmd_num].output_file,t);
            }
            else{
                flag_cmd_after_pipeline = 1;
                cmds[temp_cmd_num].com_str[cmds[temp_cmd_num].para_len] = malloc(sizeof(t));
                strcpy(cmds[temp_cmd_num].com_str[cmds[temp_cmd_num].para_len++], t);
            }
        }
        t = strtok(NULL, " ");
    }
    
    if(flag_cmd_after_pipeline){
        temp_cmd_num++;
    }
    
    return(temp_cmd_num);
}

int  readline(int fd, char *ptr, int maxlen){
    int n,rc;
    char c;
    for(n=1; n<maxlen;n++){
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
    return(n);
}

void start_server(int argc,char *argv[],int *sockfd){
    struct sockaddr_in  serv_addr;
    
    /* 1.Socket */
    if((*sockfd = socket(AF_INET, SOCK_STREAM, 0))<0)
        printf("server : can't open stream socket\n");
    
    /* 2.bind */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //htonl :convert "ASCII IP" to "int IP"
    
    if(argc<2)
        serv_addr.sin_port = htons(SERV_TCP_PORT);
    else
        serv_addr.sin_port = htons(atoi(argv[1]));
    
    if(bind(*sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){ //bind sockfd and serv_addr
        printf("server : can't bind local address\n");
        exit(0);
    }
    
    /* 3.listen */
    listen(*sockfd, 5);
    printf("Start Server, wait client\n");
}

int check_legal_cmd(char *cmd){
    char *test = strtok(getenv("PATH"),":");
    char *filepath=malloc(sizeof(char)*100);
    
    if(strcmp("printenv",cmd)==0 || strcmp("setenv",cmd)==0)
        return(1);
    
    while(test != NULL){
        sprintf(filepath,"%s/%s",test,cmd);
        if(access(filepath, F_OK) == 0)
            return (1);
        test = strtok(NULL,":");
    }
    return(0);
}

int fork_cmds(int newsockfd, int total_com_num, int cmd_count){
    int j,cmd_index;
    int f_d=0;
    int status;
    int cmdchildpid;
    char temp[1000];
    int index_count_output;
    int index_count;
    int real_do_num=0;
    int illegal_flag = 1;
    
    
    /* check each commands */
    for(cmd_index=0; cmd_index<total_com_num; cmd_index++){
        index_count_output = cmd_index + cmd_count + cmds[cmd_index].output_to;
        index_count        = cmd_index + cmd_count;
        char * execvp_str[ cmds[cmd_index].para_len +1 ]; 
        printf("\ncmd_index=%d,cmd:%s\n",cmd_index,cmds[cmd_index].com_str[0]);
        
        if(check_legal_cmd(cmds[cmd_index].com_str[0]) ){//legad command
            printf("legal command:%s\n",cmds[cmd_index].com_str[0]);
            
            //need output to ,so create a pipe
            if( cmds[cmd_index].output_to > 0 &&inputflag[ index_count_output ] == 0 ){
                inputflag[ index_count_output ] = 1;
                pipe_fd[index_count_output] = malloc(sizeof(int)*2);
                if (pipe(pipe_fd[index_count_output]) == -1){ // create pipe
                    perror( "cmd pipe error");
                    exit(EXIT_FAILURE);
                }
            }

            if(strcmp(cmds[cmd_index].com_str[0],"printenv")==0){
                sprintf(temp, "%s=%s\n", cmds[cmd_index].com_str[1],getenv(cmds[cmd_index].com_str[1]));
                if(strcmp(getenv(cmds[cmd_index].com_str[1]),"bin")==0){
                    sprintf(temp, "%s=%s:.\n", cmds[cmd_index].com_str[1],getenv(cmds[cmd_index].com_str[1]));
                }
                write(newsockfd, temp, strlen(temp));
            }
            else if(strcmp(cmds[cmd_index].com_str[0],"setenv")==0){
                setenv(cmds[cmd_index].com_str[1], cmds[cmd_index].com_str[2], 1);
            }
            else {
                printf("other command\n");
                /* fork */
                cmdchildpid=fork();
                
                if(cmdchildpid<0){
                    perror("fork error");
                }
                else if(cmdchildpid==0){//child process
                    /* set execvp_str */
                    for(j=0; j<cmds[cmd_index].para_len; j++){
                        execvp_str[j]   = cmds[cmd_index].com_str[j];
                    }
                    execvp_str[j] = NULL;
                    
                    
                    if(strcmp(cmds[cmd_index].output_file, "\0") != 0){ // if find '>' 
                        FILE * f =fopen(cmds[cmd_index].output_file,"w");
                        f_d = fileno(f);
                    }
                    
                    /* set input */
                    if(inputflag[index_count] == 1){//have data in pipe for this cmd
                        printf("%s:someone output to this command \n",cmds[cmd_index].com_str[0]);
                        if(dup2(pipe_fd[index_count][0], STDIN_FILENO)==-1){
                            perror("dup2 output error");
                        }
                        close(pipe_fd[index_count][1]);
                    }

                    /* set output to where */
                    if( cmds[cmd_index].output_to>0 ){
                        printf("%s:cmds[cmd_index].output_to>0\n",cmds[cmd_index].com_str[0]);
                        dup2(pipe_fd[index_count_output][1], STDOUT_FILENO);
                    }
                    else if(f_d > 0){
                        printf("cmd_index=%d,%s:f_d > 0\n",cmd_index,cmds[cmd_index].com_str[0]);
                        dup2(f_d, STDOUT_FILENO);
                    }
                    else{
                        printf("%s:printf to socket\n",cmds[cmd_index].com_str[0]);
                        dup2(newsockfd, STDOUT_FILENO);
                    }
                    
                    /* always print error to client */
                    dup2(newsockfd, STDERR_FILENO);
                    
                    /* execvp */
                    if (execvp(execvp_str[0],execvp_str) <0 ){
                        perror("error on exec");
                        exit(0);
                    }
                }
                else{//parent process
                    if(inputflag [index_count]==1 ){ //close used command
                        close (pipe_fd[index_count][0]);
                        close (pipe_fd[index_count][1]);
                        inputflag [index_count]=0;
                    }
                    waitpid(cmdchildpid, &status,WUNTRACED);
                    kill(cmdchildpid,SIGKILL);
                }
            }
            if(inputflag [index_count]==1 ){ //close used command
                close (pipe_fd[index_count][0]);
                close (pipe_fd[index_count][1]);
                inputflag [index_count]=0;
            }
            real_do_num++;
        }
        else{//illegad command
            if(cmd_index == 0){
                real_do_num++;
            }
            illegal_flag = 0;
            sprintf(temp, "Unknown command: [%s].\n",cmds[cmd_index].com_str[0]);
            write(newsockfd, temp, strlen(temp));//printf("temp=%s",temp);
            
            if(cmd_index == 0){
                if(inputflag [index_count]==1){ //close used command
                    close (pipe_fd[index_count][0]);
                    close (pipe_fd[index_count][1]);
                }
            }
            return(real_do_num);
        }
        
        printf("cmd_index+cmd_count=%d,inputflag=%d\n",index_count,inputflag[index_count]);
    }
    return(real_do_num);
}

int main(int argc,char *argv[]){
    int sockfd=0,newsockfd=0,clilen=0,clientchildpid=0,total_com_num=0,cmd_count=0;
    char welcome_message[]="****************************************\n** Welcome to the information server. **\n****************************************\n% ";
    struct sockaddr_in cli_addr;
    //char *set_PATH = "bin";
    char inputBuffer[MESSAGE_LEN];
    int status;
    
    strcpy(inputBuffer,"\0"); //init inputbuffer
    memset(inputflag,0,sizeof(inputflag)); // clear flag
    setenv("PATH", "bin", 1);
    start_server(argc,argv,&sockfd);
    
    while(1){
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr,(socklen_t *) &clilen);//wait client connet to this server
        
        if(newsockfd<0) printf("server : accept error");
        else{
            clientchildpid=fork();
            if(clientchildpid<0)  perror("fork error");
            else if(clientchildpid==0){//child process
                /* child process serve accepted client*/
                write(newsockfd,welcome_message,strlen(welcome_message));
                
                while(1){
                    memset(cmds,0,sizeof(cmds));
                    if(readline(newsockfd, inputBuffer, sizeof(inputBuffer))>1){
                        
                        total_com_num = cut_inbuf(inputBuffer,cmd_count);
                        
                        if(strcmp(inputBuffer, "exit") == 0){
                            close(newsockfd);
                            printf("\n===exit===\n");
                            exit(0);
                        }

                        ptfallcmd(total_com_num);
                        
                        cmd_count += fork_cmds(newsockfd,total_com_num,cmd_count);
                        printf("cmd_count=%d,\nexit for_cmds\n",cmd_count);
                        write(newsockfd,"% ",strlen("% "));
                    }
                }
            }
            else { //parent
            	waitpid(clientchildpid, &status,WUNTRACED);
            	kill(clientchildpid,SIGKILL);
                close(newsockfd);
            }
        }   
    }
    return(0);
}

