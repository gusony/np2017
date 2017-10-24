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
    char   output_str[10000];
    int    output_to;
    int    output_to_bytes;
}command_t;
command_t cmds[256];


int  dir_num = 0;
int sockfd, newsockfd, clilen, clientchildpid,cmdchildpid, total_com_num, cmd_count;
char dir_list[100][100];
char inputBuffer[10000];
char welcome_message[]="****************************************\n** Welcome to the information server. **\n****************************************\n";

struct sockaddr_in cli_addr, serv_addr;
int *pipe_fd[2000];
int  inputflag[2000];


void ptfallcmd(void);
void cut_pip(char inputbuf[10000]);
void read_dir(char *path);
int  readline(int fd, char *ptr, int maxlen);
void start_server(int argc,char *argv[]);
void exe_cmds(int cmd_index);
void fork_cmds(void);

void ptfallcmd(void){
    int i=0,j;
    printf("------------------------------\n");
    while(cmds[i].com_str[0][0]!='\0'){
        printf("count=%d,",cmd_count);
        printf("index=%d,",i);
        printf("output_to=%d,",cmds[i].output_to);
        printf("com_str=");
        for(j=0; j<=cmds[i].para_len; j++) //cmds[i].para_len
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
    while(cmds[i].com_str[0][0]!='\0'){
        cmds[i].output_to_bytes = 0;
       
        if( i>0 ){
            cmds[i-1].output_to =1;
        }

        while(47 < cmds[i].com_str[0][cmds[i].output_to_bytes] && cmds[i].com_str[0][cmds[i].output_to_bytes] < 58)
            cmds[i].output_to_bytes++;
        
        if(cmds[i].output_to_bytes >0){
            strncpy(temp, cmds[i].com_str[0], cmds[i].output_to_bytes);
            cmds[i-1].output_to = atoi(temp);
        }
        i++;
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
    
    /* split command and para */
    i=0;
    while(cmds[i].com_str[0][0]!='\0'){
        j=0;k=0;space_index = 0;
        
        while(cmds[i].com_str[0][++space_index] != ' ' && space_index<strlen(cmds[i].com_str[0]));//find the first ' '
        if(cmds[i].com_str[0][space_index+1] != '\0' && cmds[i].com_str[0][space_index+1] != ' ')
            for(j=space_index;j<strlen(cmds[i].com_str[0]);j++)
                if(cmds[i].com_str[0][j] == ' '){
                    if(cmds[i].com_str[0][j+1] != '\0')
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
    dir_num = 0;
    
    if ((d = opendir(path))){
        while ((dir = readdir(d)) != NULL)
            strcpy(dir_list[dir_num++],dir->d_name); 
        closedir(d);
    }
    
    printf("in %s:\n",path);
    for(int i=0; i<dir_num; i++)
        printf("%s\n",dir_list[i]);
}

int  readline(int fd, char *ptr, int maxlen){
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
    
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){ //bind sockfd and serv_addr
        printf("server : can't bind local address\n");
        exit(0);
    }
    else
        printf("bind successful\n");
    
    
    /* 3.listen */
    listen(sockfd, 5);
}

void exe_cmds(int cmd_index){
    char * execvp_str[ cmds[cmd_index].para_len +2 ];
    int i,j;
    int f_d=0;//= (int *)NULL;
    //printf("entry exe_cmds:%s,\n",cmds[cmd_index].com_str[0]);
    
    /* set execvp_str */
    execvp_str[0] = &(cmds[cmd_index].com_str[0][0]);
    for(j=1; j<=cmds[cmd_index].para_len; j++){
        execvp_str[j] = &cmds[cmd_index].com_str[j][0];
        if(strcmp(cmds[cmd_index].com_str[j], ">") == 0){ // if find > 
            FILE * f =fopen(cmds[cmd_index].com_str[j+1],"w");
            f_d = fileno(f);
            break;
        }
    }
    execvp_str[j]=NULL;
    
    /* set input */
    if(inputflag[cmd_count+cmd_index] == 1){//have data in pipe for this cmd
        printf("%s:someone output to this command \n",cmds[cmd_index].com_str[0]);
        dup2(pipe_fd[cmd_count+cmd_index][0], STDIN_FILENO);
        close(pipe_fd[cmd_count+cmd_index][1]);
    }

    /* set output to where */
    if( cmds[cmd_index].output_to>0 ){
        printf("%s:cmds[cmd_index].output_to>0\n",cmds[cmd_index].com_str[0]);
        i=cmd_count+cmd_index+cmds[cmd_index].output_to;
        dup2(pipe_fd[i][1], STDOUT_FILENO);
    }
    else if(f_d > 0){
        printf("%s:f_d > 0\n",cmds[cmd_index].com_str[0]);
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

void fork_cmds(void){
    int i,cmd_index,j;
    int status;
    
    
    /* check each commands */
    for(cmd_index=0; cmd_index<total_com_num; cmd_index++){
        
        j = cmd_index+cmd_count;
        if( cmds[cmd_index].output_to>0 ){
            inputflag[ j+cmds[cmd_index].output_to ] = 1;
            pipe_fd[j+cmds[cmd_index].output_to] = malloc(sizeof(int)*2);
            if (pipe(pipe_fd[j+cmds[cmd_index].output_to]) == -1){ // create pipe
                perror( "cmd pipe error");
                exit(EXIT_FAILURE);
            }
        }
            
        if(     strcmp(cmds[cmd_index].com_str[0],"exit")==0) exit(0);
        else if(strcmp(cmds[cmd_index].com_str[0],"printenv")==0){
            dup2(newsockfd, STDOUT_FILENO);
            //printf("%s=%s\n",cmds[cmd_index].com_str[1],getenv(cmds[cmd_index].com_str[1]));
        }
        else if(strcmp(cmds[cmd_index].com_str[0],"setenv")==0){
            setenv(cmds[cmd_index].com_str[1], cmds[cmd_index].com_str[2], 1);
            read_dir(getenv("PATH"));
        }
        else{ //other command
            for (i=0; i<dir_num; i++){
                if(strcmp(cmds[cmd_index].com_str[0], dir_list[i])==0 ){//is a legal command
                    
                    /* fork */
                    cmdchildpid=fork();
                    
                    if(cmdchildpid<0)
                        perror("fork error");
                    else if(cmdchildpid==0){//child process
                        sleep(1);
                        exe_cmds(cmd_index);/* exe command */
                    }
                    else{//parent process
                        //waitpid(cmdchildpid, &status, WUNTRACED);// | WNOHANG);
                        waitpid(cmdchildpid, &status, WNOHANG);
                    }
                    
                    i=dir_num+1;//jump over unknown command
                }
            }
            if(i==dir_num){
                dup2(newsockfd, STDOUT_FILENO);
                printf("Unknown command: [%s]\n",cmds[cmd_index].com_str[0]);
                dup2(STDOUT_FILENO, newsockfd);
            }
        }
        
        if(inputflag [cmd_index+cmd_count]==1){
            close (pipe_fd[cmd_index+cmd_count][0]);
            close (pipe_fd[cmd_index+cmd_count][1]);
            inputflag [cmd_index+cmd_count]=0;
        }
    }
}

int main(int argc,char *argv[]){
    //int  j=0;
    strcpy(inputBuffer,"\0");
    
    /* prepare environment */
    //char *origin_PATH = getenv("PATH");
    char *set_PATH = "./bin";
    setenv("PATH", set_PATH, 1);
    read_dir(getenv("PATH")); 
    memset(inputflag,0,sizeof(inputflag)); // clear flag
    
    start_server(argc,argv);
    
    while(1){
        
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr,(socklen_t *) &clilen);
        cmd_count=0;


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
        memset(cmds,0,sizeof(cmds));
        send(newsockfd,"% ",sizeof("% "),0);
        readline(newsockfd, inputBuffer, sizeof(inputBuffer));
        cut_pip(inputBuffer);
        ptfallcmd();
        fork_cmds();
        cmd_count += total_com_num;
        printf("done:%s,",inputBuffer);
    }
    close(newsockfd);
    exit(0);
    return(0);
}

/*for(i=0; i < cmd_index+cmd_count; i++){
        printf("cmd_index=%d, cmd_count=%d,i=%d,cmds[i].output_to=%d\n",cmd_index,cmd_count,i,cmds[i].output_to);
        if(cmd_index+cmd_count == i+cmds[i].output_to){
            
            
        }
    }*/
