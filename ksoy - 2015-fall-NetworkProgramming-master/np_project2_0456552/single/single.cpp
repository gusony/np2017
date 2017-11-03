#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <string>
#include <iostream>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include "socket.h"
#include "common.h"

using namespace std;

char port[5] = "7575";
char msg[BUFFER_SIZE];
const char welcomStr[] = "****************************************\n** Welcome to the information server. **\n****************************************\n";
const char loginStr[] = "*** User '%s' entered from %s/%s. ***\n";
const char leftStr[] = "*** User '%s' left. ***\n";
const char whoStr[] = "<ID>\t<nickname>\t<IP/port>\t<indicate me>\n";
const char whoFormatStr[] = "%d\t%s\t%s/%s%s\n";
const char nameExistStr[] = "*** User '%s' already exists. ***\n";
const char nameStr[] = "*** User from %s/%s is named '%s'. ***\n";
const char yellStr[] = "*** %s yelled ***: %s\n";
const char tellStr[] = "*** %s told you ***: %s\n";
const char tellFailStr[] = "*** Error: user #%s does not exist yet. ***\n";
const char publicPipeInStr[] = "*** %s (#%d) just received via '%s' ***\n";
const char publicPipeOutStr[] = "*** %s (#%d) just piped '%s' ***\n";
const char publicPipeInErrorStr[] = "*** Error: public pipe #%d does not exist yet. ***\n";
const char publicPipeOutErrorStr[] = "*** Error: public pipe #%d already exists. ***\n";
int publicPipes[PUBLIC_PIPE_SIZE][2];
bool publicPipesFlag[PUBLIC_PIPE_SIZE] = {false};
client *clients[CLIENT_NUMBERS] = {NULL};
bool clientsFlag[CLIENT_NUMBERS] = {false};
char dirBuff[BUFFER_SIZE];

int cmdWho(int fromId);
int cmdTell(int fromId, char *cmd);
int cmdYell(const char *msg, int msgLength, int fromId, bool selfShow);
int cmdName(int fromId, char *name);
int cmdBlock(int fromId, char *cmd);

int command_handler(int sockfd, char *cmd, int pipes[PIPE_SIZE][2], int &pipePtr, int fromId)
{
	char oriCmd[CMD_LENGTH];
	char buffer[BUFFER_SIZE];
	int isPipe = 0, isErrorPipe = 0, isFilePipe = 0, isAllPipe = 0;
	int pipeTo = 0, errorPipeTo = 0, allPipeTo = 0;
	char *fileTo;
	char *cmds[CMDS_SIZE];
	int cmdCount = 0;
	bool isLastCmd = false;

	char **binFile = new char*[BIN_FILES_SIZE];
	int binFileCount = 0, binFileSize = BIN_FILES_SIZE;

	int tmpPipe[2];

	strcpy(oriCmd, cmd);

	//create current pipe
        if(pipes[pipePtr][0] < 0 &&  pipe(pipes[pipePtr]) < 0)
        {
                fprintf(stderr, "client: can't create pipe : %s\n", strerror(errno));
                return -1;
        }

	//get bin files
	if(getBinFile(binFile, binFileCount, binFileSize) < 0)
	{
		fprintf(stderr, "client: can't check bin files : %s\n", strerror(errno));
		return -1;
	}

	//split cmds
	split(cmds, cmdCount, cmd, ' ');

	//check all pipe
	if((cmds[cmdCount - 1][0] == '!' && cmds[cmdCount - 1][1] == '|') || cmds[cmdCount - 1][0] == '|' && cmds[cmdCount - 1][1] == '!')
	{
		isAllPipe = 1;
		allPipeTo = atoi(cmds[cmdCount - 1] + 2);
	}
	if(isAllPipe && pipes[(pipePtr + allPipeTo) % PIPE_SIZE][0] < 0)
		if(pipe(pipes[(pipePtr + allPipeTo) % PIPE_SIZE]) < 0)
		{
			fprintf(stderr, "client: can't create all pipe : %s\n", strerror(errno));
		return -1;
		}

	//check pipe
	if(!isAllPipe && cmds[cmdCount - 1][0] == '|' && int(cmds[cmdCount - 1][1]) != 0)
	{
		isPipe = 1;
		pipeTo = atoi(cmds[cmdCount - 1] + 1);
	}
	else if(!isAllPipe && cmdCount > 1 && cmds[cmdCount - 2][0] == '|' && int(cmds[cmdCount - 2][1]) != 0)
	{
                isPipe = 1;
                pipeTo = atoi(cmds[cmdCount - 2] + 1);
	}

	if(isPipe && pipes[(pipePtr + pipeTo) % PIPE_SIZE][0] < 0)
		if(pipe(pipes[(pipePtr + pipeTo) % PIPE_SIZE]) < 0)
		{
			fprintf(stderr, "client: can't create pipe : %s\n", strerror(errno));
			return -1;
		}
	
	//check error pipe
	if(!isAllPipe && cmds[cmdCount - 1][0] == '!')
        {
                isErrorPipe = 1;
                errorPipeTo = atoi(cmds[cmdCount - 1] + 1);
        }
        else if(!isAllPipe && cmdCount > 1 && cmds[cmdCount - 2][0] == '!')
        {
                isErrorPipe = 1;
                errorPipeTo = atoi(cmds[cmdCount - 2] + 1);
        }
	if(isErrorPipe && pipes[(pipePtr + errorPipeTo) % PIPE_SIZE][0] < 0)
                pipe(pipes[(pipePtr + errorPipeTo) % PIPE_SIZE]);

	//check output to file
	if(cmdCount > 1 && cmds[cmdCount - 2][0] == '>' && int(cmds[cmdCount - 2][1]) == 0)
	{
		isFilePipe = 2;
		fileTo = cmds[cmdCount - 1];
	}

	//parse cmd
	for(int i = 0; i < cmdCount - isPipe - isErrorPipe - isFilePipe - isAllPipe; ++i)
	{
		bool isValidCmd = false;
		int paraCount = 1;
		int execpid, status;
		int isPublicPipeIn = 0, isPublicPipeOut = 0;
		int publicPipeIn = 0, publicPipeOut = 0;

		//count parameter
		while((i + paraCount < cmdCount - isPipe - isErrorPipe - isFilePipe - isAllPipe) && strcmp(cmds[i + paraCount], "|" ) != 0)
			paraCount++;
		
		isLastCmd = (i + paraCount == cmdCount - isPipe - isErrorPipe - isFilePipe - isAllPipe);

		//check public pipe out
		if(paraCount > 1 && cmds[i + paraCount - 1][0] == '>' && cmds[i+ paraCount - 1][1] != 0)
		{
			isPublicPipeOut = 1;
			publicPipeOut = atoi(cmds[i + paraCount - 1] + 1);
		}
		else if(paraCount > 2 && cmds[i + paraCount - 2][0] == '>' && cmds[i+ paraCount - 2][1] != 0)
		{
			isPublicPipeOut = 1;
			publicPipeOut = atoi(cmds[i + paraCount - 2] + 1);
		}

		//check public pipe in
		if(paraCount > 1 && cmds[i + paraCount - 1][0] == '<' && cmds[i+ paraCount - 1][1] != 0)
		{
			isPublicPipeIn = 1;
			publicPipeIn = atoi(cmds[i + paraCount - 1] + 1);
		}
		else if(paraCount > 2 && cmds[i + paraCount - 2][0] == '<' && cmds[i+ paraCount - 2][1] != 0)
		{
			isPublicPipeIn = 1;
			publicPipeIn = atoi(cmds[i + paraCount - 2] + 1);
		}
		
		//check public pipe status
		if(isPublicPipeIn)
			if(!publicPipesFlag[publicPipeIn])
			{
				sprintf(msg, publicPipeInErrorStr, publicPipeIn);
				write(sockfd, msg, strlen(msg));
				return 1;
			}
		if(isPublicPipeOut)
			if(publicPipesFlag[publicPipeOut])
			{
				sprintf(msg, publicPipeOutErrorStr, publicPipeOut);
				write(sockfd, msg, strlen(msg));
				if(i == 0)
					return 1;
				else
					break;
			}
			else
			{
				if(pipe(publicPipes[publicPipeOut]) < 0)
					errexit("client: creat public pipe error. %s\n", strerror(errno));
			}		

		//paraCount minus public pipe
		paraCount -= (isPublicPipeIn + isPublicPipeOut);
		
		//check valid cmd
		for(int j = 0; j < binFileCount; ++j)
			if(strcmp(cmds[i], binFile[j]) == 0)
			{
				isValidCmd = true;
				break;
			}
		if(!isValidCmd)
		{
			sprintf(msg, "Unknown Command: [%s].\n", cmds[i]);
			write(sockfd, msg, strlen(msg));
			if(i==0)
				return 1;
			else
				break;
		}
		
		//create temp pipe	
		if(pipe(tmpPipe) < 0)
                {
                        fprintf(stderr, "client: can't create temp pipe : %s\n", strerror(errno));
                        return -1;
                }
		
		//show public pipe msg
		if(isPublicPipeIn)
		{
			sprintf(msg, publicPipeInStr, clients[fromId]->name, fromId + 1, oriCmd);
			cmdYell(msg, strlen(msg), fromId, true);
		}
		if(isPublicPipeOut)
		{
			sprintf(msg, publicPipeOutStr, clients[fromId]->name, fromId + 1, oriCmd);
			cmdYell(msg, strlen(msg), fromId, true);
		}

		//fork exec
		if((execpid = fork()) < 0)
		{
			fprintf(stderr, "client: can't fork exec : %s\n", strerror(errno));
			sprintf(msg, "Command [%s] exec error!\n", cmds[i]);
			write(sockfd, msg, strlen(msg));
			return 1;
		}
		else if(execpid == 0) //exec child
		{
			//set parameter
			char **paras;
			paras = new char*[paraCount + 1];
			for(int j = 0; j < paraCount; ++j)
				paras[j] = cmds[i + j];
			paras[paraCount] = char(NULL);

			//set input pipe
			close(STDIN_FILENO);
			if(isPublicPipeIn)
				dup(publicPipes[publicPipeIn][0]);
			else
				dup(pipes[pipePtr][0]);

			//set out pipe
			close(STDOUT_FILENO);
			if(isPublicPipeOut)
				dup(publicPipes[publicPipeOut][1]);
			else if(isLastCmd)
			{
				if(isPipe)
					dup(pipes[(pipePtr + pipeTo) % PIPE_SIZE][1]);
				else if(isAllPipe)
					dup(pipes[(pipePtr + allPipeTo) % PIPE_SIZE][1]);
				else if(isFilePipe)
				{
					FILE *file = fopen(fileTo, "w");
					dup(fileno(file));
				}
				else
					dup(sockfd);
			}
			else
				dup(tmpPipe[1]);

			//set err pipe
			close(STDERR_FILENO);
			if(isPublicPipeOut)
				dup(publicPipes[publicPipeOut][1]);
			else if(isErrorPipe)
				dup(pipes[(pipePtr + errorPipeTo) % PIPE_SIZE][1]);
			else if(isAllPipe)
				dup(pipes[(pipePtr + allPipeTo) % PIPE_SIZE][1]);
			else
				dup(sockfd);			
	
			//close other pipes
			close(sockfd);
			close(tmpPipe[0]);
			close(tmpPipe[1]);
			closePipe(&pipes[0][0], PIPE_SIZE);
			closePipe(&publicPipes[0][0], PUBLIC_PIPE_SIZE);

			//exec			
			if(execvp(cmds[i], paras) < 0)
			{
				printf("exec ERROR!\n");
				exit(1);
			}
		}
		else
		{
			close(pipes[pipePtr][0]);
			while(close(pipes[pipePtr][1])) printf("close in cmd pipe in again");
                        printf("current pipe(in cmd)2: %d,%d, tempPipe: %d, %d\n", pipes[pipePtr][0], pipes[pipePtr][1], tmpPipe[0], tmpPipe[1]);
			while(wait(&status) != execpid);

			pipes[pipePtr][0] = tmpPipe[0];
			pipes[pipePtr][1] = tmpPipe[1];

			
			//close public pipe
			if(isPublicPipeIn) 
			{
				close(publicPipes[publicPipeIn][0]);
				publicPipes[publicPipeIn][0] = -1;
				publicPipesFlag[publicPipeIn] = 0;
			}
			if(isPublicPipeOut)
			{
				close(publicPipes[publicPipeOut][1]);
				publicPipes[publicPipeOut][1] = -1;
				publicPipesFlag[publicPipeOut] = 1;
			}
		}

		i += paraCount + isPublicPipeIn + isPublicPipeOut;
	} 

	//close current pipe and move pipe pointer
	close(pipes[pipePtr][0]);
	close(pipes[pipePtr][1]);
	pipes[pipePtr][0] = -1;
	pipes[pipePtr][1] = -1;	
	pipePtr++;
	pipePtr %= PIPE_SIZE;
	
	return 0;	
}

int client_handler(int fd)
{
	int id = findClientIdByFd(fd, clientsFlag, clients);
	char cmd[CMD_LENGTH];
	int cmdLength;

	//set client path
	setenv("PATH", clients[id]->path, 1);

	//get client command
	cmdLength = readline(fd, cmd, CMD_LENGTH);

	printf("get cmd: %s\n", cmd);

	if(cmdLength == 0) 
		return 0;
	else if(cmdLength < 0)
	{
		write(fd, "Something Wrong!\n", 17);
		fprintf(stderr, "client%s(%s): readline error : %s\n", clients[id]->ip, clients[id]->port, strerror(errno));
		return 0;
	}

	//client exit
	if(startWith("exit", cmd))
	{
		closePipe(&(clients[id]->pipes[0][0]), PIPE_SIZE);
		return 0;
	}
	//client name
	else if(startWith("name ",  cmd))
		cmdName(id, cmd + 5);
	//client who
	else if(startWith("who", cmd))
		cmdWho(id);
	//client yell
	else if(startWith("yell ", cmd))
	{
		sprintf(msg, yellStr, clients[id]->name, cmd + 5);
		cmdYell(msg, strlen(msg), id, true);
	}
	//client tell
	else if(startWith("tell ", cmd))
		cmdTell(id, cmd + 5);
	else if(startWith("/block ", cmd))
		cmdBlock(id, cmd + 7);
	//client printenv
	else if(startWith("printenv ", cmd))
	{
		char *env;
		if((env = getenv(cmd + 9)) != NULL)
		{
			sprintf(msg, "%s=%s\n", cmd + 9, env);
			write(fd, msg, strlen(msg));
		}
		else
			write(fd, "NOT FOUND!\n", 11);
	}
	//client setenv
	else if(startWith("setenv ", cmd))
	{
		char *env[3];
		int c;
		split(env, c, cmd, ' ');
		if(env[2][strlen(env[2]) - 1] == '\n') env[2][strlen(env[2]) - 1] = '\0';
		if(setenv(env[1], env[2], 1) < 0)
		{
			fprintf(stderr, "client%s(%s): setenv error : %s\n", clients[id]->ip, clients[id]->port, strerror(errno));
			write(fd, "set fail!\n", 10);
		}
		else
			strcpy(clients[id]->path, env[2]);
	}
	else
		//handle client command
		if(command_handler(fd, cmd, clients[id]->pipes, clients[id]->pipePtr, id) < 0)
			return 0;

	write(fd, "% ", 2);
}

int main(int argc, char *argv[], char** envp){
	struct sockaddr_in client_addr;
	unsigned int client_len;
	fd_set rfds, afds;
	int nfds;
	int msock;
	unsigned int alen;

	//if user give port
	if(argc == 2)
                strcpy(port, argv[1]);

	//init public Pipes
	for(int i=0; i<PUBLIC_PIPE_SIZE; ++i)
	{
		publicPipes[i][0] = -1;
		publicPipes[i][1] = -1;
	}

	//remove environment
	for(char **env = envp; *env!=0; env++)
	{
		string s = string(*env);
		const char *c = s.substr(0, s.find('=')).data();
		if(strcmp(c, "PATH") && strcmp(c, "PWD"))
			unsetenv(c);
	}

	//change dir
	if(chdir(DEFAULT_DIR) < 0)
		errexit("server: can't change dir : %s\n", strerror(errno));
	getcwd(dirBuff, BUFFER_SIZE);
	printf("server: current working directory : %s\n", dirBuff);

	//change path
	if(setenv("PATH", DEFAULT_PATH, 1) < 0)
		errexit("server: can't change path : %s\n", strerror(errno));
	printf("server: current working PATH = %s\n", getenv("PATH"));

	//init
	msock = setSocket(port);
	nfds = msock + CLIENT_NUMBERS + 500;//getdtablesize();
	FD_ZERO(&afds);
	FD_SET(msock, &afds);
	
        printf("server: server sarting...\n");

	while(1)
	{
		memcpy(&rfds, &afds, sizeof(rfds));

		//select
		if(select(nfds, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0)
			errexit("server: select error :%d %s\n",(char*)errno, strerror(errno));
		
		//new connect
		if(FD_ISSET(msock, &rfds))
		{
			int ssock, id;			
			alen = sizeof(client_addr);

			//accpet client
			ssock = accept(msock, (struct sockaddr *) &client_addr, &alen);
			if(ssock < 0)
				errexit("server: accept error : %s\n", strerror(errno));

			//set client structure
			if((id = getNewClientId(clientsFlag)) < 0)
				errexit("server: too many connect\n");
			client *cli = new client();
			cli->id = id;
			cli->fd = ssock;
			//strcpy(cli->ip, inet_ntoa(client_addr.sin_addr));
			//sprintf(cli->port, "%u", client_addr.sin_port);
			strcpy(cli->ip, "CGILAB");
			strcpy(cli->port, "511");
			clients[id] = cli;
			clientsFlag[id] = true;
				
			printf("server: %s(%s) is linking...id=%d\n", clients[id]->ip, clients[id]->port, id);

			//add client sock to fds
			FD_SET(ssock, &afds);

			//send welcome msg
			sprintf(msg, loginStr, clients[id]->name, clients[id]->ip, clients[id]->port);
			cmdYell(msg, strlen(msg), id, false);
			char temp[BUFFER_SIZE];
			sprintf(temp, "%s%s%% ", welcomStr, msg);
			write(ssock, temp, strlen(temp));
			
		}
		else //get command
			for(int fd=0 ; fd<nfds ; ++fd)
				if(fd != msock && FD_ISSET(fd, &rfds))
					if(client_handler(fd) == 0)
					{
						int id = findClientIdByFd(fd, clientsFlag, clients);
						sprintf(msg, leftStr, clients[id]->name);
						cmdYell(msg, strlen(msg), id, true);
						if(id >=0)
							printf("server: %s(%s) is quit... id=%d\n", clients[id]->ip, clients[id]->port, id);
						deleteClient(id, clientsFlag, clients);
						close(fd);
						FD_CLR(fd, &afds);
					}
	}

	close(msock);
	return 0;
}


int cmdWho(int fromId)
{
	write(clients[fromId]->fd, whoStr, strlen(whoStr));
	for(int i=0; i<CLIENT_NUMBERS; ++i)
	{
		if(!clientsFlag[i]) continue;
		sprintf(msg, whoFormatStr, i+1, clients[i]->name, clients[i]->ip, clients[i]->port, (i==fromId?"\t<-me":""));
		write(clients[fromId]->fd, msg, strlen(msg));
	}
}

int cmdTell(int fromId, char *cmd)
{
	char toIdc[2];
	sscanf(cmd, "%s", toIdc);
	int toId = atoi(toIdc);
	if(toId > 0 && toId <= CLIENT_NUMBERS && clientsFlag[toId-1])
	{
		if( !(clients[toId - 1]->block[fromId]))
		{
			sprintf(msg, tellStr, clients[fromId]->name, cmd+strlen(toIdc)+1);
			write(clients[toId-1]->fd, msg, strlen(msg));
		}
	}
	else
	{
		sprintf(msg, tellFailStr, toIdc);
		write(clients[fromId]->fd, msg, strlen(msg));
	}
}

int cmdYell(const char *msg, int msgLength, int fromId, bool selfShow)
{
	for(int i=0; i<CLIENT_NUMBERS; ++i)
		if(!clientsFlag[i]) continue;
		else if(!selfShow && i == fromId) continue;
		else if(clients[i]->block[fromId]) continue;
		else 
			write(clients[i]->fd, msg, msgLength);

}

int cmdName(int fromId, char *name)
{
	if(strlen(name) > NAME_LENGTH) name[NAME_LENGTH] = '\0';
	for(int i=0; i<CLIENT_NUMBERS; ++i)
	{
		if(clientsFlag[i] && !strcmp(clients[i]->name, name))
		{
			sprintf(msg, nameExistStr, name);
			write(clients[fromId]->fd, msg, strlen(msg));
			return 0;
		}
	}
	strcpy(clients[fromId]->name, name);
	sprintf(msg, nameStr, clients[fromId]->ip, clients[fromId]->port, name);
	cmdYell(msg, strlen(msg), fromId, true);
}

int cmdBlock(int fromId, char *cmd)
{
        char toIdc[2];
        sscanf(cmd, "%s", toIdc);
        int toId = atoi(toIdc);
	clients[fromId]->block[toId - 1] = 1;
}
