/*
    By Junhan Lin 0646001
    Network programing Homework1
    RAS(remote acess system)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

//my define
#define MCN                  5000 //max_command_number
#define EPNM                 50   //EXEC_PARA_NUM_MAX
#define PATH_LEN             50
#define BUFFER_SIZE          100
#define MESSAGE_LEN          10000
#define NAME_LENGTH          20
#define SERV_TCP_PORT        7575
#define CLIENT_NUMBERS       31 //no use id0


typedef struct command{
    char *evec_cmd_list[EPNM];  // execvp parameter list ,the [0] is the command ,others is parameter
    char to_cmd_file[30];    // XXX > a.txt   , output to file
    int  para_len;              // default=0 ;not include the NULL at the enc
    int  to_cmd;                // default=0 ;this command output to command in this client
    int  to_cmd_digits;         // default=-1;how many digits of to_client
    int  to_client;             // default=-1;>5  output to a client
    int  from_client;           // default=-1;<6  input from a client
}cmd_t;

typedef struct client_t{
    int  id;
    char name[NAME_LENGTH];
    char ip[15];
    char port[6];
    int  fd;                    //client socket fd
    char path[PATH_LEN];
    int  *cp[CLIENT_NUMBERS];//clients pipes      ,pipe for clients to pipe data to another client
    bool cpf[CLIENT_NUMBERS];   //clients pipes flag ,if any client pipe to this client
    char inputBuffer  [MESSAGE_LEN];

    cmd_t cmds         [MCN];
    int   *cmd_pipe    [MCN];
    bool  cmd_pipe_flag[MCN];
    int   cmd_counts;           // total command counter
    int   olcn;                 // one line command numbers
}client_t;

// global var
client_t *clients[CLIENT_NUMBERS];
bool clientflag  [CLIENT_NUMBERS];
bool path_def_flag[CLIENT_NUMBERS];
char DEFAULT_PATH[] = "bin:.";
char DEFAULT_NAME[] = "(no name)";
int msockfd = 0;
const char whoStr[]                 = "<ID>\t<nickname>\t<IP/port>\t<indicate me>\n";
const char tellStr[]                = "*** %s told you ***: %s\n";
const char nameExistStr[]           = "*** User '%s' already exists. ***\n";
const char nameStr[]                = "*** User from %s/%s is named '%s'. ***\n";
const char publicPipeOutStr[]       = "*** %s (#%d) just piped '%s' to %s (#%d) ***\n";
const char reve_pipe_from[]         = "*** %s (#%d) just received from %s (#%d) by '%s' ***\n";
const char welcome_message[]        = "****************************************\n** Welcome to the information server. **\n****************************************\n";
const char enter_str[]              = "*** User '(no name)' entered from %s/%s. ***\n";
const char exitStr[]                = "*** User '%s' left. ***\n";
const char who_yell[]               = "*** %s yelled ***: %s\n";
const char tellFailStr[]            = "*** Error: user #%d does not exist yet. ***\n";
const char user_already_exist_str[] = "*** Error: the pipe #%d->#%d already exists. ***\n";
const char pipe_not_exist_str[]     = "*** Error: the pipe #%d->#%d does not exist yet. ***\n";
const char user_not_exist_str[]     = "*** Error: user #%d does not exist yet. ***\n";

int findidbyname(char *name){
    printf("findidbyname:%s\n",name);
    for(int i=1; i<CLIENT_NUMBERS;i++){
        printf("findidbyname:i=%d,name = %s\n",i,clients[i]->name);
        if(strcmp(clients[i]->name, name)==0){
            printf("---find---\n");
            return(i);
        }
    }
    return(0);
    /*
    0   : not found
    1~N : id
    */
}

//function
void initallclient(void){
    for(int i=0; i<CLIENT_NUMBERS; i++){
    	clients[i]    = malloc(sizeof(client_t));
        memset(clients[i], 0, sizeof(client_t));
        for (int j=0; j<CLIENT_NUMBERS; j++)
            clients[i]->cp[j] = NULL;
        strcpy(clients[i]->name,DEFAULT_NAME);
        clients[i]->id = 0;
        clients[i]->fd = -1;
        clients[i]->olcn = -1;
        clientflag[i] = false;
        path_def_flag[i] = true;
        strcpy(clients[i]->path, DEFAULT_PATH); //init path
    }
}
void closeclient(int id){

	//close(clients[id]->fd);
    shutdown(clients[id]->fd,2);

    free(clients[id]);
    clients[id] = malloc(sizeof(client_t));
    memset(clients[id], 0, sizeof(client_t));
    strcpy(clients[id]->name,DEFAULT_NAME);
    strcpy(clients[id]->path, DEFAULT_PATH); //init path
    clients[id]->id = 0;
    clients[id]->fd = -1;
    clients[id]->olcn = -1;
    clientflag[id] = false;
}
void ptfallcmd(int id){
    printf("------------------------------\n");
    for(int i=clients[id]->cmd_counts; i < clients[id]->olcn +clients[id]->cmd_counts; i++ ){
    	printf("id=%2d,", id);
        printf("cmd_counts=%2d,", clients[id]->cmd_counts);
        printf("to_cmd=%2d,",    clients[id]->cmds[i].to_cmd);
        printf("to_client=%d,",  clients[id]->cmds[i].to_client);
        printf("from_client=%d,",clients[id]->cmds[i].from_client);
        printf("para_len=%d,",   clients[id]->cmds[i].para_len);
        //printf("inputBuffer=%s,",clients[id]->inputBuffer);
        printf("com_str=");
        for(int j=0; j< clients[id]->cmds[i].para_len; j++)
            printf("%s,",        clients[id]->cmds[i].evec_cmd_list[j]);
        printf("> %s,\n",        clients[id]->cmds[i].to_cmd_file);
    }
    printf("------------------------------\n");
}
void cut_inbuf(int id){ //return(olcn)
	//make var name short
	char inputbuf[MESSAGE_LEN];
	strcpy(inputbuf,clients[id]->inputBuffer);
	int Ccounts = clients[id]->cmd_counts;
	printf("Ccounts=%d\n",Ccounts);
	int paralen = 0;
	//temp var
    int olcn=0;
    int flag_cmd_after_pipeline=0;

    char *t = strtok(inputbuf, " ");
    bool resetflag = false;

    while(t != NULL && t[0]>31){
    	if(!resetflag){
	    	clients[id]->cmds[ Ccounts + olcn ].to_client=0;
	    	clients[id]->cmds[ Ccounts + olcn ].from_client=0;
        	resetflag = true;
        }
        if(t[0] == '|'){
        	resetflag=false;
            t[0] = '0';
            paralen = 0;
            clients[id]->cmds[ Ccounts + olcn ].to_cmd = atoi(t);
            if(clients[id]->cmds[ Ccounts + olcn ].to_cmd==0)
                 clients[id]->cmds[ Ccounts + olcn ].to_cmd = 1;
            olcn++;
            flag_cmd_after_pipeline=0;
        }
        else if(t[0] == '>'){
        	if(t[1] >= 48){// >N, output to client
        		t[0]='0';
        		clients[id]->cmds[ Ccounts + olcn ].to_client = atoi(t);
        		clients[id]->cpf[atoi(t)] = true;
        	}

        	else {// > N ,output to file
        		t = strtok(NULL, " ");
        		strcpy(clients[id]->cmds[ Ccounts + olcn ].to_cmd_file, t);
        	}
        }
        else if(t[0] == '<'){
        	t[0]='0';
    		clients[id]->cmds[ Ccounts + olcn ].from_client = atoi(t);
        }
        else {
            flag_cmd_after_pipeline = 1;
            clients[id]->cmds[ Ccounts + olcn ].evec_cmd_list[paralen] = malloc(sizeof(t));
            strcpy(clients[id]->cmds[ Ccounts + olcn ].evec_cmd_list[paralen++], t);
        }
        clients[id]->cmds[Ccounts + olcn].para_len = paralen;
        t = strtok(NULL, " ");
    }

    if(flag_cmd_after_pipeline){//if legal, make sure at least olcn>=0
        olcn++;
    }

    clients[id]->olcn = olcn;
    clients[id]->cmd_counts = Ccounts;
}
int  readline(int fd, char *ptr, int maxlen){//successful ->ret(>=1), fail->ret(-1)
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
    return(n);//string length
}
void start_server(int argc,char *argv[],int *msockfd){
    struct sockaddr_in  serv_addr;

    /* 1.Socket */
    if((*msockfd = socket(AF_INET, SOCK_STREAM, 0))<0)
        printf("server : can't open stream socket\n");

    /* 2.bind */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //htonl :convert "ASCII IP" to "int IP"

    if(argc<2)
        serv_addr.sin_port = htons(SERV_TCP_PORT);
    else
        serv_addr.sin_port = htons(atoi(argv[1]));

    if(bind(*msockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){ //bind msockfd and serv_addr
        printf("server : can't bind local address\n");
        exit(0);
    }

    /* 3.listen */
    listen(*msockfd, 5);
    printf("Start Server, wait client\n");
}
int check_legal_cmd(char *cmd){
    char *test = strtok(getenv("PATH"),":");
    char *filepath=malloc(sizeof(char)*100);

    if(strcmp("printenv",cmd)==0 || strcmp("setenv",cmd)==0 || strcmp("who",cmd)==0 || strcmp("name",cmd)==0 || strcmp("tell",cmd)==0 || strcmp("yell",cmd)==0)
        return(1);

    while(test != NULL){
        sprintf(filepath,"%s/%s",test,cmd);
        if(access(filepath, F_OK) == 0)
            return (1);
        test = strtok(NULL,":");
    }
    return(0);
}
int GetNewClentId(){
    for (int i=1; i<CLIENT_NUMBERS; ++i)
        if(clientflag[i] == false)
            return(i);

    return(-1);//too many client
}
int GetIdByfd(int fd){//return id of the fd
	for(int i=1;i<CLIENT_NUMBERS; i++)
		if(clients[i]->fd == fd && clientflag[i]==1)
			return(clients[i]->id);
	return(-1);
}
void cmdWho(int id){
	char msg[BUFFER_SIZE];
    write(clients[id]->fd, whoStr, strlen(whoStr));
    for(int i=0; i<CLIENT_NUMBERS; ++i){
        if(!clientflag[i]) continue;
        sprintf(msg, "%d\t%s\t%s/%s%s\n", i, clients[i]->name, clients[i]->ip, clients[i]->port, (i==id?"\t<-me":""));
        write(clients[id]->fd, msg, strlen(msg));
    }
}
void cmdTell(int fromId,int cmd_index){
    int i,toId=0;
    char copycmd[BUFFER_SIZE];

    strcpy(copycmd,clients[fromId]->inputBuffer);
    for(i=0;i<strlen(copycmd)-5;i++){
        copycmd[i]=copycmd[i+5];
    }
    copycmd[i]='\0';
    strcat(copycmd,"\r\n");
    //printf("copycmd=%s\n",copycmd);

    char *cmd=malloc(sizeof(char)*BUFFER_SIZE);
    strcpy(cmd, copycmd);
    char toIdc[2];
    char msg[BUFFER_SIZE];
    char *toName=malloc(sizeof(char)*100);
    strcpy(toName, clients[fromId]->cmds[cmd_index].evec_cmd_list[1]);//clients[id]->cmds[cmd_index].evec_cmd_list[0],"tell"
    printf("1.toname = %s\n",toName);
    toId =  findidbyname(toName);
    printf("cmdTell:toId=%d\n",toId);

    if(toId > 0){
        printf("cmdTell:toid>0\n");
        if(toId <= CLIENT_NUMBERS && clientflag[toId]){
            printf("id=%d,name=%s\n",fromId,clients[fromId]->name);
            printf("id=%d,name=%s\n",toId,clients[toId]->name);
            printf("cmdTell:%s\n",clients[fromId]->name);
            printf("cmdTell:%s\n",clients[fromId]->cmds[cmd_index].evec_cmd_list[2]);
            //"*** %s told you ***: %s\n";
            sprintf(msg, tellStr, clients[fromId]->name, clients[fromId]->cmds[cmd_index].evec_cmd_list[2]);
            write(clients[toId]->fd, msg, strlen(msg));
        }
        else{
            //printf("cmdWho:toId = %d\n",toId);
            //printf("cmdWho:toIdc= %s\n",toIdc);
            sprintf(msg, tellFailStr, toId);
            write(clients[fromId]->fd, msg, strlen(msg));
        }
    }
    else {
        sscanf(cmd, "%s", toIdc);//cmd first string put in toIdc
        int toId = atoi(toIdc);
        if(toId > 0 && toId <= CLIENT_NUMBERS && clientflag[toId]){
            sprintf(msg, tellStr, clients[fromId]->name, cmd+strlen(toIdc)+1);
            write(clients[toId]->fd, msg, strlen(msg));
        }
        else{
        	printf("cmdWho:toId = %d\n",toId);
        	printf("cmdWho:toIdc= %s\n",toIdc);
            sprintf(msg, tellFailStr, toId);
            write(clients[fromId]->fd, msg, strlen(msg));
        }
    }
}
void cmdYell(const char *msg, int msgLength, int fromId, bool selfShow){
	for(int i=1; i<CLIENT_NUMBERS; ++i)
        if(!clientflag[i]) continue;
        else if(!selfShow && i == fromId) continue;
        else{
            write(clients[i]->fd, msg, msgLength);
            //write(clients[i]->fd, "\r\n", strlen("\n"));
        }
}
int cmdName(int fromId){
	char *name =malloc(sizeof(char)*BUFFER_SIZE);
	sscanf(clients[fromId]->inputBuffer, "name %s",name);
    char msg[BUFFER_SIZE];


    if(strlen(name) > NAME_LENGTH) name[NAME_LENGTH] = '\0';
    for(int i=0; i<CLIENT_NUMBERS; ++i)
    {
        if(clientflag[i] && !strcmp(clients[i]->name, name)){//name existed!
            sprintf(msg,nameExistStr, name);
            write(clients[fromId]->fd, msg, strlen(msg));
            return 0;
        }
    }
    strcpy(clients[fromId]->name, name);
    sprintf(msg, nameStr, clients[fromId]->ip, clients[fromId]->port, name);
    cmdYell(msg, strlen(msg), fromId, true);
    return(1);
}
int handle_cmd(int id,int fd){
    int j;
    int f_d=0;
    int status;
    int cmdchildpid;
    char temp[1000];
    int real_do_num=0;
    bool inputclientflag = false;
    bool outputclientflag = false;

    int olcn = clients[id]->olcn;
    int cmd_count = clients[id]->cmd_counts;


    setenv("PATH", clients[id]->path, 1);


    /* check each commands */
    for(int cmd_index = cmd_count; cmd_index < olcn+cmd_count; cmd_index++){
        char * execvp_str[ clients[id]->cmds[cmd_index].para_len +1 ];
        char msg[BUFFER_SIZE];

        int to_client   = clients[id]->cmds[cmd_index].to_client;
        int to_cmd      = clients[id]->cmds[cmd_index].to_cmd;
        int from_client = clients[id]->cmds[cmd_index].from_client;
        printf("from_client=%d\n",from_client);
        char to_cmd_file[30]; strcpy(to_cmd_file, clients[id]->cmds[cmd_index].to_cmd_file);
        int index_tocmd = cmd_index + to_cmd;


        //default output to socket_fd
        dup2(0, STDIN_FILENO);
        dup2(1, STDOUT_FILENO);
        dup2(clients[id]->fd, STDERR_FILENO);
        //printf("clients[2]->cpf[4]=%s\n",clients[2]->cpf[4]?"true":"false");
        if(check_legal_cmd(clients[id]->cmds[cmd_index].evec_cmd_list[0]) ){//legad command
            printf("legal command:%s\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
            //printf("start:id=%d\n",id);

            //prepare output
            printf("prepare output\n");
            if(to_client > 0 ){
            	//printf("prepare:clientflag[to_client]=%s\n",clientflag[to_client]?"true":"false");
            	//printf("prepare:clients[to_client]->cpf[id]=%s\n",clients[to_client]->cpf[id]?"true":"false");
            	if(!clientflag[to_client]){
                    sprintf(msg, user_not_exist_str, to_client);//"*** Error: the pipe #%d->#%d does not exist yet. ***\n"
                    write(clients[id]->fd, msg, strlen(msg) );
                    outputclientflag = false;
                    continue;
                }
                else if(clients[to_client]->cpf[id]){
                    sprintf(msg, user_already_exist_str ,id,to_client);
                    write(clients[id]->fd, msg, strlen(msg));
                    outputclientflag = false;
                    continue;
                }
                else{
                    //printf("prepare:clients[to_client]exist and pipe not yet exist\n");
                    outputclientflag = true;
                    //printf("A:clients[2]->cpf[4]=%s\n",clients[2]->cpf[4]?"true":"false");
                    clients[to_client]->cpf[id]=true;
                    //printf("B:clients[2]->cpf[4]=%s\n",clients[2]->cpf[4]?"true":"false");
                    clients[to_client]->cp[id]=malloc(sizeof(int)*2);
                    //printf("prepare:pipe clients[%d]->cp[%d]\n",to_client,id);
                    if(pipe(clients[to_client]->cp[id]) == -1 ){
                        perror( "cmd pipe error");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else if( to_cmd > 0 && clients[id]->cmd_pipe_flag[ index_tocmd ] == 0 ){
                clients[id]->cmd_pipe_flag[ index_tocmd ] = 1;
                clients[id]->cmd_pipe[index_tocmd] = malloc(sizeof(int)*2);
                if (pipe(clients[id]->cmd_pipe[index_tocmd]) == -1){ // create pipe
                    perror( "cmd pipe error");
                    exit(EXIT_FAILURE);
                }
            }


            //prepare input
            printf("prepare input\n");
            if(from_client >0){
            	if(!clientflag[from_client]){
            		//printf("from_client >-1\n,clientflag[from_client]=false\n");
                    sprintf(msg, pipe_not_exist_str,from_client, id);
                    write(clients[id]->fd, msg, strlen(msg) );
                    inputclientflag = false;
                    continue;
                }
                else if( !clients[id]->cpf[from_client] ){
                    sprintf(msg, pipe_not_exist_str, from_client, id);//"*** Error: the pipe #%d->#%d does not exist yet. ***\n"
                    //printf("clients[id]->cpf[from_client]=false,msg=%s\n",msg);
                    write(clients[id]->fd, msg, strlen(msg));
                    inputclientflag = false;
                    continue;
                }
                else {
                	close(clients[id]->cp[from_client][1]);
                	inputclientflag = true;
                }
            }

            if(strcmp(clients[id]->cmds[cmd_index].evec_cmd_list[0],"printenv")==0){
                sprintf(temp, "%s=%s\n", clients[id]->cmds[cmd_index].evec_cmd_list[1],getenv(clients[id]->cmds[cmd_index].evec_cmd_list[1]));
                if(strcmp(getenv(clients[id]->cmds[cmd_index].evec_cmd_list[1]),"bin")==0 ){
                    if(path_def_flag[id] == true){
                        sprintf(temp, "%s=%s:.\n", clients[id]->cmds[cmd_index].evec_cmd_list[1], getenv(clients[id]->cmds[cmd_index].evec_cmd_list[1]));
                        path_def_flag[id] = false;
                    }
                }
                write(clients[id]->fd, temp, strlen(temp));
            }
            else if(strcmp(clients[id]->cmds[cmd_index].evec_cmd_list[0],"setenv")==0){
                setenv(clients[id]->cmds[cmd_index].evec_cmd_list[1], clients[id]->cmds[cmd_index].evec_cmd_list[2], 1);
                strcpy(clients[id]->path, clients[id]->cmds[cmd_index].evec_cmd_list[2]);
            }
            else if(strcmp(clients[id]->cmds[cmd_index].evec_cmd_list[0],"who")==0){
            	cmdWho(id);
            }
        	else if(strcmp(clients[id]->cmds[cmd_index].evec_cmd_list[0],"name")==0){
        		cmdName(id);
        	}
    		else if(strcmp(clients[id]->cmds[cmd_index].evec_cmd_list[0],"tell")==0){
    			cmdTell(id,cmd_index);
    		}
			else if(strcmp(clients[id]->cmds[cmd_index].evec_cmd_list[0],"yell")==0){

                char *copymsg = malloc(sizeof(char)*BUFFER_SIZE);
                strcpy(copymsg, clients[id]->inputBuffer);

                char *temp = malloc(sizeof(char)*BUFFER_SIZE);
                temp = strtok(copymsg," ");
                temp = strtok(NULL,"\r\n");

                char *msg = malloc(sizeof(char)*BUFFER_SIZE);
                sprintf(msg, who_yell, clients[id]->name,temp);

				cmdYell(msg,strlen(msg),id,true);
			}
            else {
                printf("other command\n");
	            //printf("B_fork:outputclientflag=%s\n",outputclientflag?"true":"false");
                //printf("B_fork:inputclientflag=%s\n",inputclientflag?"true":"false");

                /* fork */
                cmdchildpid=fork();
                if(cmdchildpid<0) perror("fork error");
                else if(cmdchildpid==0){//child process
                    // set execvp_str
                    for(j=0; j<clients[id]->cmds[cmd_index].para_len; j++){
                        execvp_str[j]   = clients[id]->cmds[cmd_index].evec_cmd_list[j];
                    }
                    execvp_str[j] = NULL;

                    //output to file
                    if(strcmp(to_cmd_file, "\0") != 0){
                        FILE * f =fopen(to_cmd_file,"w");
                        f_d = fileno(f);
                    }

                    // set input
                    if(inputclientflag){
                    	strcpy(msg,"\0");
                    	sprintf(msg, reve_pipe_from, clients[id]->name, id, clients[from_client]->name, from_client, clients[id]->inputBuffer);
                    	cmdYell(msg,strlen(msg),id,true);
                    	//printf("prepare:inputclientflag=true,set input from %d\n",from_client);
                        if(dup2(clients[id]->cp[from_client][0], STDIN_FILENO)==-1){
                            perror("child:dup2 output error:from_clienta");
                        }
                        close(clients[id]->cp[from_client][1]);
                    }
                    else if(clients[id]->cmd_pipe_flag[cmd_index] == 1){
                        if(dup2(clients[id]->cmd_pipe[cmd_index][0], STDIN_FILENO)==-1){
                            perror("child:dup2 output error:to cmd");
                        }
                        close(clients[id]->cmd_pipe[cmd_index][1]);
                    }

                    // set output to where
                    if(outputclientflag){ // output to clients
                    	//printf("child:output to other client\n");
                        sprintf(msg, publicPipeOutStr, clients[id]->name, id, clients[id]->inputBuffer,clients[to_client]->name,to_client);
                        cmdYell(msg, strlen(msg), id, true);
                        if(dup2(clients[to_client]->cp[id][1], STDOUT_FILENO)==-1){
                            perror("child:dup2 output error:to_client");
                        }
                    }
                    else if( to_cmd >0 ){
                        //printf("child:%s:cmds[cmd_index].to_cmd=%d\n",clients[id]->cmds[cmd_index].evec_cmd_list[0],index_tocmd);
                        if(dup2(clients[id]->cmd_pipe[index_tocmd][1], STDOUT_FILENO)==-1){
                            perror("child:dup2 output error:to_cmd");
                        }
                    }
                    else if(f_d > 0){
                        //printf("child:cmd_index=%d,%s:f_d > 0\n",cmd_index,clients[id]->cmds[cmd_index].evec_cmd_list[0]);
                        if(dup2(f_d, STDOUT_FILENO)==-1){
                            perror("child:dup2 output error:to f_d");
                        }
                    }
                    else{
                        //printf("child:%s:printf to socket\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
                        if(dup2(clients[id]->fd, STDOUT_FILENO)==-1){
                            perror("child:dup2 output error:to socket");
                        }
                    }

                    // always print error to client
                    dup2(clients[id]->fd, STDERR_FILENO);

                    // exec cmd
                    if (execvp(execvp_str[0],execvp_str) <0 ){
                        perror("error on exec");
                        exit(0);
                    }
                }
                else{//parent process
                    if(clients[id]->cmd_pipe_flag [cmd_index]==1 ){ //close used command
                        close (clients[id]->cmd_pipe[cmd_index][0]);
                        close (clients[id]->cmd_pipe[cmd_index][1]);
                    }
                    clients[id]->cmd_pipe_flag [cmd_index]=0;

                    //printf("pare:id=%d\n",id);
                    //printf("pare:to_client=%d\n",to_client);
                    //printf("pare:from_client=%d\n",from_client);

                    //printf("pare:waitpid\n");
                    waitpid(cmdchildpid, &status,0);
                    //printf("pare:exit waitpid\n");

                    if(inputclientflag){
                    	printf("para:inputclientflag=true\n");
                    	inputclientflag = false;
                    	clients[id]->cpf[from_client] = false;
                    	printf("pare:close 0\n");
                    	close(clients[id]->cp[from_client][0]);
                    	printf("pare:close 1\n");
                    	close(clients[id]->cp[from_client][1]);
                        printf("pare:flag set false\n");
                    }
                    if(outputclientflag){
                    	//printf("para:outputclientflag=true\n");
                    	outputclientflag = false;
                    	//printf("pare:%s:cmds[cmd_index].to_client>0\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
                        clients[to_client]->cpf[id]=true;
                    }
                }
            }

            real_do_num++;
        }
        else{//illegad command
            printf("illeged command \n");
            if(cmd_index == cmd_count){
                real_do_num++;
                //printf("cmd_index=cmdcount\treal_do_num=%d\n",real_do_num);
            }

            sprintf(temp, "Unknown command: [%s].\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
            write(clients[id]->fd, temp, strlen(temp));

            if(cmd_index == cmd_count && clients[id]->cmd_pipe_flag [cmd_index]==1){
                close (clients[id]->cmd_pipe[cmd_index][0]);
                close (clients[id]->cmd_pipe[cmd_index][1]);
            }

            return(real_do_num);
        }

        //printf("cmd_index+cmd_count=%d,clients[id]->cmd_pipe_flag=%s\n",cmd_index,clients[id]->cmd_pipe_flag[cmd_index]?"true":"false" );
    }

    return(real_do_num);
}
int main(int argc,char *argv[]){

    int newsockfd = 0;
    int clilen = 0;
    int nfds = 0;
    int id = -1;
    int fd;

    struct sockaddr_in cli_addr;
    fd_set rfds, afds;

    //init
    initallclient();
	setenv("PATH", "bin", 1);//set server env

    //set server
    start_server(argc,argv,&msockfd);
    printf("msockfd=%d\n",msockfd);
    nfds = msockfd + CLIENT_NUMBERS + 500;//getdtablesize();
    FD_ZERO(&afds);
    FD_SET(msockfd, &afds);

    //ready to receive client
    while(1){
        //printf("memcpy afds rfds\n");
        memcpy(&rfds, &afds, sizeof(rfds));

        if(select(nfds, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0){
            fprintf(stderr,"server: select error ");
            continue;
        }

        if(FD_ISSET(msockfd, &rfds)){//new client
            //printf("new client enter\n");
            clilen = sizeof(cli_addr);
            newsockfd = accept(msockfd, (struct sockaddr*)&cli_addr,(socklen_t *) &clilen);//wait client connet to this server

            if(newsockfd < 0){
                printf("Server:accept error\n");
                continue;
            }

            if((id = GetNewClentId()) < 0){
                printf("Server:too many client\n");
                continue;
            }

            clientflag[id] = true;
            clients[id]->id = id;
            clients[id]->fd = newsockfd;
            strcpy(clients[id]->ip, "CGILAB");
            strcpy(clients[id]->port, "511");
            printf("server: %s(%s) is linking...id=%d\n", clients[id]->ip, clients[id]->port, id);

            //add client sock to fds
            FD_SET(newsockfd, &afds);

            //send welcome msg
            write(newsockfd,welcome_message,strlen(welcome_message));
            char msg[BUFFER_SIZE];

            sprintf(msg, enter_str ,clients[id]->ip, clients[id]->port);
            write(newsockfd,msg,strlen(msg));
            write(newsockfd,"% ",strlen("% "));
            cmdYell(msg, strlen(msg), id, false);
        }
        else{//old client
			for(int id=1; id<CLIENT_NUMBERS; id++){ //for all client
            	if(clientflag[id]){
            		fd = clients[id]->fd;
	            	if(fd!=msockfd && FD_ISSET(fd,&rfds) ){//not master and fd in rfds after select
	                    strcpy(clients[id]->inputBuffer, "\0");
	                    if(readline(fd, clients[id]->inputBuffer, sizeof(char)*MESSAGE_LEN )>1){
	                        cut_inbuf(id);
	                        if(strcmp(clients[id]->inputBuffer, "exit") == 0){
                                char msg[BUFFER_SIZE];
                                sprintf(msg, exitStr, clients[id]->name);
                                cmdYell(msg,strlen(msg),id,true);
                                //shutdown(fd,2);
                                closeclient(id);

                                //write(fd,"\n",sizeof(char));
                                //fflush(fd);
                                //close(fd);
                                clientflag[id]=false;
                                FD_CLR(fd, &afds);
                                printf("\n===id:%d===exit===\n",id);
							}
	                        else{
	                        	ptfallcmd(id);
		                        clients[id]->cmd_counts += handle_cmd(id,fd);

		                        //printf("cmd_count=%d,\nexit for_cmds\n",clients[id]->cmd_counts);
		                        write(fd,"% ",strlen("% "));
	                        }
	                    }
	                }
            	}
            }
        }

    }
    return(0);
}