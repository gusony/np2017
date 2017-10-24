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
    char   com_str[2000][256];
    int    para_len;
    char   output_str[10000];
    int    output_to;
    int    output_to_bytes;
    char   output_file[100];
}command_t;

command_t cmds[2000];
int *pipe_fd[2000];
int  inputflag[2000];
char dir_list[100][100];


//int cut_inbuf(char inputbuf[10000]);

void ptfallcmd(int cmd_count);
int cut_pip(char inputbuf[10000],int cmd_count);
int read_dir(char *path);
int  readline(int fd, char *ptr, int maxlen);
void start_server(int argc,char *argv[],int *sockfd);
void exe_cmds(int cmd_index,int newsockfd,int cmd_count);
void fork_cmds(int newsockfd, int total_com_num, int cmd_count,int dir_num);

void ptfallcmd(int cmd_count){
    int i=0,j;
    printf("------------------------------\n");
    while(cmds[i].com_str[0][0]!='\0'){
        printf("count=%d,",cmd_count);
        printf("index=%d,",i);
        printf("output_to=%d,",cmds[i].output_to);
        printf("para_len=%d,",cmds[i].para_len);
        printf("com_str=");
        for(j=0; j<cmds[i].para_len; j++) //cmds[i].para_len
            printf("%s,",cmds[i].com_str[j]);
        printf("\n");
        i++;
    }
}


int cut_inbuf(char inputbuf[10000],int cmd_count){
    int temp_cmd_num=0;
    int flag_cmd_after_pipeline=0;
    
    char *t = strtok(inputbuf, " ");
    printf("first t=%s\n",t);
    while(t != NULL && t[0]>31){
        printf("first t=%s\n",t);
        printf(",,,%d,,,\n",(int)t[0]);
        
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
                strcpy(cmds[temp_cmd_num].com_str[cmds[temp_cmd_num].para_len++], t);
            }
        }
        ptfallcmd( cmd_count);
        t = strtok(NULL, " ");
    }
    printf("exit t=%s\n",t);
    if(flag_cmd_after_pipeline){
        temp_cmd_num++;
    }
    
    return(temp_cmd_num);
}


int cut_pip(char inputbuf[10000],int cmd_count){
    int  i=0,j,k,space_index;
    char temp[100];
    char *test = strtok(inputbuf, "|");
    memset(cmds, 0, sizeof(cmds));
    int total_com_num = 0;
    
    
    //cut string by '|'
    while (test != NULL){
        if(test)
        strncpy(cmds[total_com_num++].com_str[0], test, strlen(test));
        test = strtok(NULL, "|");
    }
    
    // what number after '|'
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
    printf("1.\n");
    ptfallcmd(cmd_count);

    // clear the number from command 
    i=0;
    while(cmds[i++].com_str[0][0]!='\0'){
        for(j=0; cmds[i].com_str[0][cmds[i].output_to_bytes+1+j]!='\0'; j++){
            cmds[i].com_str[0][j] = cmds[i].com_str[0][cmds[i].output_to_bytes+1+j];
        }

        // 4 line after this line, to clear the space in the end of line
        if(cmds[i].com_str[0][j-1]==' '){
            cmds[i].com_str[0][j-1]='\0';
        }
        else{
            cmds[i].com_str[0][j]='\0';
        }
    }
    printf("2.\n");
    ptfallcmd(cmd_count);
    // split command and para 
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
    
    if(cmds[total_com_num-1].com_str[0][0]==0){
        cmds[total_com_num-1].com_str[0][0]='\0';
        total_com_num--;
    }
    return(total_com_num);
}
 
int  read_dir(char *path){
    DIR           *d;
    struct dirent *dir;
    int dir_num = 0;
    
    if ((d = opendir(path))){
        while ((dir = readdir(d)) != NULL)
            strcpy(dir_list[dir_num++],dir->d_name); 
        closedir(d);
    }
    
    printf("in %s:\n",path);
    for(int i=0; i<dir_num; i++)
        printf("%s\n",dir_list[i]);
    return(dir_num);
}

int  readline(int fd, char *ptr, int maxlen){
    int n,rc;
    char c;
    for(n=1; n<maxlen;n++){
        if((rc=read(fd, &c, 1))==1){
            //if(c>32 || c==10 || c==13)
            
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
    else
        printf("bind successful\n");
    
    
    /* 3.listen */
    listen(*sockfd, 5);
}

void exe_cmds(int cmd_index,int newsockfd,int cmd_count){
    char * execvp_str[ cmds[cmd_index].para_len +2 ];
    int i,j;
    int f_d=0;
    int status;
    int cmdchildpid;
    /* fork */
    cmdchildpid=fork();
    
    if(cmdchildpid<0){
        perror("fork error");
    }
    else if(cmdchildpid==0){//child process
        /* set execvp_str */
        execvp_str[0] = &(cmds[cmd_index].com_str[0][0]);
        for(j=1; j<cmds[cmd_index].para_len; j++){
            execvp_str[j] = &cmds[cmd_index].com_str[j][0];
        }
        execvp_str[j]=NULL;
        
        if(strcmp(cmds[cmd_index].output_file, "\0") != 0){ // if find '>' 
            FILE * f =fopen(cmds[cmd_index].output_file,"w");
            //printf("1.f_d=%d\n",f_d);
            f_d = fileno(f);
            //printf("2.f_d=%d\n",f_d);
            //break;
        }
        
        
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
            printf("cmd_index=%d,%s:f_d > 0\n",cmd_index,cmds[cmd_index].com_str[0]);
            dup2(f_d, STDOUT_FILENO);//maybe error
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
        if(inputflag[cmd_count+cmd_index] == 1){
            close(pipe_fd[cmd_count+cmd_index][0]);
            close(pipe_fd[cmd_count+cmd_index][1]);
        }
        //if( cmds[cmd_index].output_to>0 ){//
        //    i = cmd_count + cmd_index + cmds[cmd_index].output_to;
        //    close(pipe_fd[i][1]);
        //}
        //printf("waitpid start:%d \n",cmdchildpid);
        waitpid(cmdchildpid, &status,0);
        //printf("waitpid end:%d\n",cmdchildpid);
    }
}

void fork_cmds(int newsockfd, int total_com_num, int cmd_count, int dir_num){
    int i,cmd_index,j;
    
    char temp[1000];
    
    
    /* check each commands */
    for(cmd_index=0; cmd_index<total_com_num; cmd_index++){
        printf("cmd_index=%d",cmd_index);
        j = cmd_index+cmd_count;
        if( cmds[cmd_index].output_to>0 ){
            inputflag[ j+cmds[cmd_index].output_to ] = 1;
            pipe_fd[j+cmds[cmd_index].output_to] = malloc(sizeof(int)*2);
            if (pipe(pipe_fd[j+cmds[cmd_index].output_to]) == -1){ // create pipe
                perror( "cmd pipe error");
                exit(EXIT_FAILURE);
            }
        }
            
        if(     strcmp(cmds[cmd_index].com_str[0],"exit")==0) {
            close(newsockfd);
            printf("===exit===\n");
            exit(0);
        }
        else if(strcmp(cmds[cmd_index].com_str[0],"printenv")==0){
            sprintf(temp, "%s=%s\n", cmds[cmd_index].com_str[1],getenv(cmds[cmd_index].com_str[1]));
            write(newsockfd, temp, strlen(temp));
        }
        else if(strcmp(cmds[cmd_index].com_str[0],"setenv")==0){
            setenv(cmds[cmd_index].com_str[1], cmds[cmd_index].com_str[2], 1);
            read_dir(getenv("PATH"));
        }
        else{ //other command
            printf("check command start\n");
            for (i=0; i<dir_num; i++){
                if(strcmp(cmds[cmd_index].com_str[0], dir_list[i])==0 ){//is a legal command
                    //printf("EXEC start\n");
                    exe_cmds(cmd_index,newsockfd,cmd_count);
                    //printf("EXEC END\n");
                    
                    i=dir_num+1;//jump over unknown command
                    break;
                }
            }

            printf("check command finish,i=%d,\n",i);

            if(i==dir_num){
                sprintf(temp, "Unknown command: [%s]\n",cmds[cmd_index].com_str[0]);
                printf("temp=%s",temp);
                write(newsockfd, temp, strlen(temp));
            }
        }
        
        if(inputflag [cmd_index+cmd_count]==1){ //close used command
            close (pipe_fd[cmd_index+cmd_count][0]);
            close (pipe_fd[cmd_index+cmd_count][1]);
            inputflag [cmd_index+cmd_count]=0;
        }
    }
}

int main(int argc,char *argv[]){
    
    int sockfd=0,newsockfd=0,clilen=0,clientchildpid=0,total_com_num=0,cmd_count=0,dir_num=0;
    char welcome_message[]="****************************************\n** Welcome to the information server. **\n****************************************\n% ";
    struct sockaddr_in cli_addr;
    
    char inputBuffer[10000];
    strcpy(inputBuffer,"\0");
    /*
    //printf("%d",atoi("0234"));
    //printf("ready enter cut_inbuf\n");
    //total_com_num = cut_inbuf();
    //command_t test;
    //strcpy(test.output_file,"\0");
    //if(strcmp(test.output_file,"\0")==0)
    //    printf("NULL;");
    //else
    //    printf("test.output_file=%d,\n",(int)test.output_file);
    char temp[] = "ls |4 ";
    char *a = strtok(temp," ");
    a[0] = 'L';
    printf("start:%s,\n",a);
    a = strtok(NULL," ");
    printf("start2:%s,\n",a);
    a = strtok(NULL," ");
    printf("start3:%s,\n",a);
    */
    
    /* prepare environment */
    //char *origin_PATH = getenv("PATH");
    char *set_PATH = "./bin";
    setenv("PATH", set_PATH, 1);
    dir_num = read_dir(getenv("PATH")); 
    memset(inputflag,0,sizeof(inputflag)); // clear flag
    
    start_server(argc,argv,&sockfd);
    
    while(1){
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr,(socklen_t *) &clilen);
        

        if(newsockfd<0) printf("server : accept error");
        else{
            clientchildpid=fork();
            if(clientchildpid<0)  perror("fork error");
            else if(clientchildpid==0){//child process
                /* child process serve accepted client*/
                send(newsockfd,welcome_message,sizeof(welcome_message),0);
                
                while(1){
                    memset(cmds,0,sizeof(cmds));
                    if(readline(newsockfd, inputBuffer, sizeof(inputBuffer))>1){
                        printf("inputlen=%lu,inputstr=%s\n",strlen(inputBuffer),inputBuffer);
                        
                        total_com_num = cut_inbuf(inputBuffer,cmd_count);
                        ptfallcmd(cmd_count);
                        
                        fork_cmds(newsockfd,total_com_num,cmd_count, dir_num);
                        cmd_count += total_com_num;
                        printf("done:%s\n",inputBuffer);
                        
                        if(strcmp(inputBuffer, "exit") != 0){
                            send(newsockfd,"% ",sizeof("% "),0);
                        }
                           
                        if(strcmp(inputBuffer, "exit") == 0){
                            close(newsockfd);
                            exit(0);
                        }
                    }
                }
            }
            else { //parent
                close(newsockfd);
            }
        }   
    }
    return(0);
}

