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
#define BUFFER_SIZE          50
#define MESSAGE_LEN          10000
#define NAME_LENGTH          20
#define SERV_TCP_PORT        7575
#define CLIENT_NUMBERS       30


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
char DEFAULT_PATH[] = "bin:.";
char DEFAULT_NAME[] =  "(no name)";
int msockfd = 0;

//function
void initallclient(void){
    for(int i=0; i<CLIENT_NUMBERS; i++){
        clients[i]    = malloc(sizeof(client_t));
        memset(clients[i], 0, sizeof(client_t));
        strcpy(clients[i]->name,DEFAULT_NAME);
        clients[i]->id = -1;
        clients[i]->fd = -1;
        clients[i]->olcn = -1;
        clientflag[i] = false;
        strcpy(clients[i]->path, DEFAULT_PATH); //init path
    }
}
void closeclient(int id){
	close(clients[id]->fd);
    memset(clients[id], 0, sizeof(client_t));
    clients[id]->id = -1;
    clients[id]->fd = -1;
    clients[id]->olcn = -1;
    clientflag[id] = false;
    strcpy(clients[id]->path, DEFAULT_PATH);
}
void ptfallcmd(int id){
    printf("------------------------------\n");
    for(int i=clients[id]->cmd_counts; i < clients[id]->olcn +clients[id]->cmd_counts; i++ ){
    	printf("id=%2d,", id);
        printf("index=%2d,", i);
        printf("cmd_counts=%d,", clients[id]->cmd_counts);
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

    while(t != NULL && t[0]>31){
        if(t[0] == '|'){
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
    for (int i=0; i<CLIENT_NUMBERS; ++i)
        if(clientflag[i] == false)
            return(i);

    return(-1);//too many client
}
int GetIdByfd(int fd){//return id of the fd
	for(int i ;i<CLIENT_NUMBERS; i++)
		if(clients[i]->fd == fd && clientflag[i]==1)
			return(clients[i]->id);
	return(-1);
}
void cmdWho(int id){
	const char whoStr[] = "<ID>\t<nickname>\t<IP/port>\t<indicate me>\n";
	char msg[BUFFER_SIZE];
    write(clients[id]->fd, whoStr, strlen(whoStr));
    for(int i=0; i<CLIENT_NUMBERS; ++i){
        if(!clientflag[i]) continue;
        else if(clients[id]->fd == msockfd) continue;
        sprintf(msg, "%d\t%s\t%s/%s%s\n", i+1, clients[i]->name, clients[i]->ip, clients[i]->port, (i==id?"\t<-me":""));
        write(clients[id]->fd, msg, strlen(msg));
    }
}
void cmdTell(int fromId){
	char *cmd = malloc(sizeof(char)*BUFFER_SIZE);
	int *not_impotr_id;
    char toIdc[2];
    char msg[BUFFER_SIZE];
    const char tellStr[] = "*** %s told you ***: %s\n";
    const char tellFailStr[] = "*** Error: user #%s does not exist yet. ***\n";

    sscanf(clients[fromId]->inputBuffer,"tell %d %s",not_impotr_id,cmd);

    sscanf(cmd, "%s", toIdc);//cmd first string put in toIdc
    int toId = atoi(toIdc);
    if(toId > 0 && toId <= CLIENT_NUMBERS && clientflag[toId-1]){
        sprintf(msg, tellStr, clients[fromId]->name, cmd+strlen(toIdc)+1);
        write(clients[toId-1]->fd, msg, strlen(msg));
    }
    else{
        sprintf(msg, tellFailStr, toIdc);
        write(clients[fromId]->fd, msg, strlen(msg));
    }
}
void cmdYell(const char *msg, int msgLength, int fromId, bool selfShow){
	for(int i=0; i<CLIENT_NUMBERS; ++i)
        if(!clientflag[i]) continue;
        else if(!selfShow && i == fromId) continue;
        else
            write(clients[i]->fd, msg, msgLength);
}
int cmdName(int fromId){
	char *name =malloc(sizeof(char)*BUFFER_SIZE);
	sscanf(clients[fromId]->inputBuffer, "name %s",name);
    char msg[BUFFER_SIZE];
    const char nameExistStr[] = "*** User '%s' already exists. ***\n";
    const char nameStr[] = "*** User from %s/%s is named '%s'. ***\n";

    if(strlen(name) > NAME_LENGTH) name[NAME_LENGTH] = '\0';
    for(int i=0; i<CLIENT_NUMBERS; ++i)
    {
        if(clientflag[i] && !strcmp(clients[i]->name, name)){//name existed!
            sprintf(msg, nameExistStr, name);
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
    int index_count_output;
    int index_count;
    int real_do_num=0;

    int olcn = clients[id]->olcn;
    int cmd_count = clients[id]->cmd_counts;

    setenv("PATH", clients[id]->path, 1);
    /* check each commands */

    for(int cmd_index=cmd_count; cmd_index<olcn+cmd_count; cmd_index++){
        index_count_output = cmd_index + clients[id]->cmds[cmd_index].to_cmd;
        index_count        = cmd_index ;
        char * execvp_str[ clients[id]->cmds[cmd_index].para_len +1 ];
        printf("\ncmd_index=%d,cmd:%s\n",cmd_index,clients[id]->cmds[cmd_index].evec_cmd_list[0]);

        if(check_legal_cmd(clients[id]->cmds[cmd_index].evec_cmd_list[0]) ){//legad command
            printf("legal command:%s\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);

            //need output to ,so create a pipe
            if( clients[id]->cmds[cmd_index].to_cmd > 0 &&clients[id]->cmd_pipe_flag[ index_count_output ] == 0 ){
                clients[id]->cmd_pipe_flag[ index_count_output ] = 1;
                clients[id]->cmd_pipe[index_count_output] = malloc(sizeof(int)*2);
                if (pipe(clients[id]->cmd_pipe[index_count_output]) == -1){ // create pipe
                    perror( "cmd pipe error");
                    exit(EXIT_FAILURE);
                }
            }
            if(clients[id]->cmds[cmd_index].to_client > -1 && clients[clients[id]->cmds[cmd_index].to_client]->cpf[id]==false){
            	clients[clients[id]->cmds[cmd_index].to_client]->cpf[id]=true;
            	clients[clients[id]->cmds[cmd_index].to_client]->cp[id]=malloc(sizeof(int)*2);
            	if(pipe(clients[clients[id]->cmds[cmd_index].to_client]->cp[id]) == -1 ){
            		perror( "cmd pipe error");
                    exit(EXIT_FAILURE);
            	}
            }

            if(strcmp(clients[id]->cmds[cmd_index].evec_cmd_list[0],"printenv")==0){
                sprintf(temp, "%s=%s\n", clients[id]->cmds[cmd_index].evec_cmd_list[1],getenv(clients[id]->cmds[cmd_index].evec_cmd_list[1]));
                if(strcmp(getenv(clients[id]->cmds[cmd_index].evec_cmd_list[1]),"bin")==0){
                    sprintf(temp, "%s=%s:.\n", clients[id]->cmds[cmd_index].evec_cmd_list[1],getenv(clients[id]->cmds[cmd_index].evec_cmd_list[1]));
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
    			cmdTell(id);
    		}
			else if(strcmp(clients[id]->cmds[cmd_index].evec_cmd_list[0],"yell")==0){
				char *msg = malloc(sizeof(char)*BUFFER_SIZE);
				sscanf(clients[id]->inputBuffer, "yell %s",msg);
				cmdYell(msg,strlen(msg),id,true);
			}
            else {
                printf("other command\n");

                /* fork */
                cmdchildpid=fork();

                if(cmdchildpid<0){
                    perror("fork error");
                }
                else if(cmdchildpid==0){//child process

                	dup2(0, STDIN_FILENO);
                	dup2(1, STDOUT_FILENO);

                    /* set execvp_str */
                    for(j=0; j<clients[id]->cmds[cmd_index].para_len; j++){
                        execvp_str[j]   = clients[id]->cmds[cmd_index].evec_cmd_list[j];
                    }
                    execvp_str[j] = NULL;


                    if(strcmp(clients[id]->cmds[cmd_index].to_cmd_file, "\0") != 0){ // if find '>'
                        FILE * f =fopen(clients[id]->cmds[cmd_index].to_cmd_file,"w");
                        f_d = fileno(f);
                    }
                    /* set input */
                    if(clients[id]->cmd_pipe_flag[index_count] == 1){//have data in pipe for this cmd
                        printf("%s:someone output to this command \n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
                        if(dup2(clients[id]->cmd_pipe[index_count][0], STDIN_FILENO)==-1){
                            perror("dup2 output error");
                        }
                        close(clients[id]->cmd_pipe[index_count][1]);
                    }
                    else{
                    	for(int i=0;i<CLIENT_NUMBERS;i++){
                			if(clients[id]->cpf[i] && id!=i){
                				printf("client %d output to this command \n",i,clients[id]->cmds[cmd_index].evec_cmd_list[0]);
			                    if(dup2(clients[id]->cp[i][0], STDIN_FILENO)==-1){
			                        perror("dup2 output error");
			                    }
			                    close(clients[id]->cp[i][1]);
	                    	}
            			}
                    }


                    /* set output to where */
                    if(clients[id]->cmds[cmd_index].to_client>0){ // output to clients
						printf("%s:cmds[cmd_index].to_client>0\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
                        dup2(clients[clients[id]->cmds[cmd_index].to_client]->cp[id][1], STDOUT_FILENO);
                    }
                    else if( clients[id]->cmds[cmd_index].to_cmd>0 ){
                        printf("%s:cmds[cmd_index].to_cmd>0\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
                        dup2(clients[id]->cmd_pipe[index_count_output][1], STDOUT_FILENO);
                    }
                    else if(f_d > 0){
                        printf("cmd_index=%d,%s:f_d > 0\n",cmd_index,clients[id]->cmds[cmd_index].evec_cmd_list[0]);
                        dup2(f_d, STDOUT_FILENO);
                    }
                    else{
                        printf("%s:printf to socket\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
                        dup2(clients[id]->fd, STDOUT_FILENO);
                    }

                    /* always print error to client */
                    dup2(clients[id]->fd, STDERR_FILENO);

                    /* execvp */
                    if (execvp(execvp_str[0],execvp_str) <0 ){
                        perror("error on exec");
                        exit(0);
                    }
                }
                else{//parent process
                    if(clients[id]->cmd_pipe_flag [index_count]==1 ){ //close used command
                        close (clients[id]->cmd_pipe[index_count][0]);
                        close (clients[id]->cmd_pipe[index_count][1]);
                        clients[id]->cmd_pipe_flag [index_count]=0;
                    }
                    waitpid(cmdchildpid, &status,0);
                }
            }
            if(clients[id]->cmd_pipe_flag [index_count]==1 ){ //close used command
                close (clients[id]->cmd_pipe[index_count][0]);
                close (clients[id]->cmd_pipe[index_count][1]);
                clients[id]->cmd_pipe_flag [index_count]=0;
            }
            real_do_num++;
        }
        else{//illegad command
            if(cmd_index == 0){
                real_do_num++;
            }
            //illegal_flag = 0;
            sprintf(temp, "Unknown command: [%s].\n",clients[id]->cmds[cmd_index].evec_cmd_list[0]);
            write(clients[id]->fd, temp, strlen(temp));//printf("temp=%s",temp);


            if(cmd_index == 0){

                if(clients[id]->cmd_pipe_flag [index_count]==1){ //close used command
                    close (clients[id]->cmd_pipe[index_count][0]);
                    close (clients[id]->cmd_pipe[index_count][1]);
                }
            }

            return(real_do_num);
        }

        printf("cmd_index+cmd_count=%d,clients[id]->cmd_pipe_flag=%s\n",index_count,clients[id]->cmd_pipe_flag[index_count]?"true":"false" );
    }

    return(real_do_num);
}
int main(int argc,char *argv[]){

    int newsockfd = 0;
    int clilen = 0;
    int nfds = 0;
    int id = -1;
    int fd;
    char welcome_message[]="****************************************\n** Welcome to the information server. **\n****************************************\n";
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
    clientflag[msockfd]=true;

    //ready to receive client
    while(1){
        //printf("while(1),memcpy\n");
        memcpy(&rfds, &afds, sizeof(rfds));

        //select
        //printf("select\n");
        if(select(nfds, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0){
            fprintf(stderr,"server: select error ");
            continue;
            //exit(0);
        }

        //printf("fd_isset\n");
        if(FD_ISSET(msockfd, &rfds)){//new client
            clilen = sizeof(cli_addr);
            newsockfd = accept(msockfd, (struct sockaddr*)&cli_addr,(socklen_t *) &clilen);//wait client connet to this server

            if(newsockfd < 0){
                //fprintf(stderr, "Server:accept error\n");
                printf("Server:accept error\n");
                continue;
                //exit(0);
            }

            if((id = GetNewClentId()) < 0){
                //fprintf(stderr,"Server:too many client\n");
                printf("Server:too many client\n");
                continue;
                //exit(0);
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
            sprintf(msg, "*** User '(no name)' entered from %s/%s. ***\n",clients[id]->ip, clients[id]->port);
            write(newsockfd,msg,strlen(msg));
            write(newsockfd,"% ",strlen("% "));
            cmdYell(msg, strlen(msg), id, false);
        }
        else{//old client
			for(int id=0; id<CLIENT_NUMBERS; id++){ //for all client
            	if(clientflag[id]){
            		fd = clients[id]->fd;//&& read(fd,c,sizeof(c)) != -1
	            	if(fd!=msockfd && FD_ISSET(fd,&rfds) ){//not master and fd in rfds after select
	                    printf("id=%d,fd=%d\n",id,fd);
	                    strcpy(clients[id]->inputBuffer, "\0");

	                    if(readline(fd, clients[id]->inputBuffer, sizeof(char)*MESSAGE_LEN )>1){
	                        cut_inbuf(id);
	                        if(strcmp(clients[id]->inputBuffer, "exit") == 0){
	                            closeclient(id);
	                            FD_CLR(fd, &afds);
	                            printf("\n===id:%d===exit===\n",id);
	                        }
	                        else{
	                        	ptfallcmd(id);
		                        clients[id]->cmd_counts += handle_cmd(id,fd);

		                        printf("cmd_count=%d,\nexit for_cmds\n",clients[id]->cmd_counts);
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

/*
    	//memset(cmds,0,sizeof(cmds));

                    //char * inputBuffer = malloc(sizeof(char)*BUFFER_SIZE);
                    //strcpy(inputBuffer,"\0");
        if(newsockfd<0) printf("server : accept error");
        else{
            clientchildpid=fork();
            if(clientchildpid<0)  perror("fork error");
            else if(clientchildpid==0){//child process
                // child process serve accepted client


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
                close(newsockfd);
            }
        }
        */