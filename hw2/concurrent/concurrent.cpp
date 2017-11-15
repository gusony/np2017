#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
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
#include "semaphore.h"

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
const char userNotExistErrorStr[] = "*** Error: user #%s does not exist yet. ***\n";
const char userNotExistErrorStrD[] = "*** Error: user #%d does not exist yet. ***\n";
const char publicPipeInStr[] = "*** %s (#%d) just received from %s (#%d) by '%s' ***\n";
const char publicPipeOutStr[] = "*** %s (#%d) just piped '%s' to %s (#%d) ***\n";
const char publicPipeInErrorStr[] = "*** Error: the pipe #%d->#%d does not exist yet. ***\n";
const char publicPipeOutErrorStr[] = "*** Error: the pipe #%d->#%d already exists. ***\n";

int semServer;
int semServer2;
int semClient;
int semClient2;

char publicFiles[PUBLIC_PIPE_SIZE][PUBLIC_PIPE_SIZE][10];

int spid;

char *ctoscmd;
char *ctospara;
char *ctospara2;
char *ctospara3;

int *cflag;
int *cfd;
char *cip;
char *cport;
char *cname;

int cmdWho(int fromId);
int cmdTell(int fromId, char *cmd);
int cmdYell(const char *msg, int msgLength, int fromId, bool selfShow);
int cmdName(int fromId, char *name);

void initSEM()
{
	if((semServer = semCreate(SEM_SERVER, 0)) < 0)
		errexit("server: can't create server sem : %s\n", strerror(errno));
	if((semServer2 = semCreate(SEM_SERVER2, 0)) < 0)
		errexit("server: can't create server2 sem : %s\n", strerror(errno));
	if((semClient = semCreate(SEM_CLIENT, 1)) < 0)
		errexit("server: can't create client sem : %s\n", strerror(errno));
	if((semClient2 = semCreate(SEM_CLIENT2, 1)) < 0)
		errexit("server: can't create client sem : %s\n", strerror(errno));
}

void initSHM()
{
	ctoscmd = (char*) shmat(getShmID(SHM_CLIENT_TO_SERVER_CMD, sizeof(char) * BUFFER_SIZE), NULL, 0);
	ctospara = (char*) shmat(getShmID(SHM_CLIENT_TO_SERVER_PARA, sizeof(char) * BUFFER_SIZE), NULL, 0);
	ctospara2 = (char*) shmat(getShmID(SHM_CLIENT_TO_SERVER_PARA2,sizeof(char) * BUFFER_SIZE), NULL, 0);
	ctospara3 = (char*) shmat(getShmID(SHM_CLIENT_TO_SERVER_PARA3,sizeof(char) * BUFFER_SIZE), NULL, 0);
	cflag = (int*)shmat(getShmID(SHM_FLAG_KEY, sizeof(int) * CLIENT_NUMBERS), NULL, 0);
	cfd = (int*) shmat(getShmID(SHM_FD_KEY, sizeof(int) * CLIENT_NUMBERS), NULL, 0);
	cip = (char*) shmat(getShmID(SHM_IP_KEY, sizeof(char) * CLIENT_NUMBERS * IP_LENGTH), NULL, 0);
	cport = (char*) shmat(getShmID(SHM_PORT_KEY, sizeof(char) * CLIENT_NUMBERS * PORT_LENGTH), NULL, 0);
	cname = (char*) shmat(getShmID(SHM_NAME_KEY, sizeof(char) * CLIENT_NUMBERS * NAME_LENGTH), NULL, 0);
}

void initPublicFiles()
{
	for(int i=0;i<PUBLIC_PIPE_SIZE;++i)
	{
		for(int j=0;j<PUBLIC_PIPE_SIZE;++j)
		{
			sprintf(publicFiles[i][j], "../%d_%d", i, j);
			FILE *f = fopen(publicFiles[i][j], "r");
			if(f)
			{
				fclose(f);
				if(remove(publicFiles[i][j]))
					errexit("server: can't init public files%s\n", strerror(errno));
			}
		}
	}
}

void closeOtherClient(int id)
{
	for(int i=0;i<CLIENT_NUMBERS;++i)
		if(cfd[i] > 0 && i != id) {
			while(close(cfd[i]) < 0);
		}
}

void closeClientPublicFile(int id)
{
	printf("close public file %d\n", id);
	for(int i=0;i<CLIENT_NUMBERS;++i)
	{
		FILE *f = fopen(publicFiles[id + 1][i], "r");
		if(f)
		{
			fclose(f);
			if(remove(publicFiles[id + 1][i]))
				errexit("server: can't close public files%s\n", strerror(errno));
		}
	}
}

int command_handler(int sockfd, char *cmd, int id, int pipes[PIPE_SIZE][2], int &pipePtr)
{
	char buffer[BUFFER_SIZE];
	int isPipe = 0, isErrorPipe = 0, isFilePipe = 0, isAllPipe = 0;
	int pipeTo = 0, errorPipeTo = 0, allPipeTo = 0;
	char *fileTo;
	char cmds[CMDS_SIZE][BUFFER_SIZE];
	//char **cmds = new char*[CMDS_SIZE];
	//for(int i=0;i<CMDS_SIZE;++i) cmds[i] = new char[50];
	int cmdCount = 0;
	bool isLastCmd = false;

	char **binFile = new char*[BIN_FILES_SIZE];
	int binFileCount = 0, binFileSize = BIN_FILES_SIZE;

	int tmpPipe[2];

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
	newSplit(cmds, cmdCount, cmd, " ");

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

		//check public pipe in
		if(paraCount > 1 && cmds[i + paraCount - 1][0] == '<' && cmds[i+ paraCount - 1][1] != 0)
		{
			isPublicPipeIn = 1;
			publicPipeIn = atoi(cmds[i + paraCount - 1]+1);
		}
		else if(paraCount > 2 && cmds[i + paraCount - 2][0] == '<' && cmds[i+ paraCount - 2][1] != 0)
		{
			isPublicPipeIn = 1;
			publicPipeIn = atoi(cmds[i + paraCount - 2]+1);
		}


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

		//paraCount minus public pipe
		paraCount -= (isPublicPipeIn + isPublicPipeOut);

		//check valid cmd
		for(int j = 0; j < binFileCount; ++j)
			if(!strcmp(cmds[i], binFile[j]))
			{
				isValidCmd = true;
				break;
			}
		if(!isValidCmd)
		{
			sprintf(msg, "Unknown command: [%s].\n", cmds[i]);
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

		semWait(semClient2);

		//check public pipe status
		if(isPublicPipeIn)
		{
			FILE *f = fopen(publicFiles[id + 1][publicPipeIn], "r");
			if(!f)
			{
				sprintf(msg, publicPipeInErrorStr, publicPipeIn, id + 1);
				write(sockfd, msg, strlen(msg));
				semSignal(semClient2);
				return 1;
			}
			fclose(f);
		}
		if(isPublicPipeOut)
		{
			if(cflag[publicPipeOut - 1] == 0) {
				sprintf(msg, userNotExistErrorStrD, publicPipeOut);
				write(sockfd, msg, strlen(msg));
				semSignal(semClient2);
				return 1;
			}
			FILE *f = fopen(publicFiles[publicPipeOut][id + 1], "r");
			if(f)
			{
				fclose(f);
				sprintf(msg, publicPipeOutErrorStr, id + 1, publicPipeOut);
				write(sockfd, msg, strlen(msg));
				if(i == 0)
				{
					semSignal(semClient2);
					return 1;
				}
				else
				{
					semSignal(semClient2);
					break;
				}
			}
		}


		//show public pipe msg
		if(isPublicPipeIn)
		{
			sprintf(msg, publicPipeInStr, cname + (id * NAME_LENGTH), id + 1, cname + ((publicPipeIn - 1) * NAME_LENGTH), publicPipeIn, cmd);
			cmdYell(msg, strlen(msg), id, true);
		}
		if(isPublicPipeOut)
		{
			sprintf(msg, publicPipeOutStr, cname + (id * NAME_LENGTH), id + 1, cmd, cname + ((publicPipeOut - 1) * NAME_LENGTH), publicPipeOut);
			cmdYell(msg, strlen(msg), id, true);
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
			paras[paraCount] = (char*)NULL;

			//set input pipe
			close(STDIN_FILENO);
			if(isPublicPipeIn)
			{
				FILE *f = fopen(publicFiles[id + 1][publicPipeIn], "r");
				dup(fileno(f));
			}
			else
				dup(pipes[pipePtr][0]);

			//set out pipe
			close(STDOUT_FILENO);
			if(isPublicPipeOut)
			{
				FILE *f = fopen(publicFiles[publicPipeOut][id + 1], "w");
				dup(fileno(f));
			}
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
			{
				FILE *f = fopen(publicFiles[publicPipeOut][id + 1], "w");
				dup(fileno(f));
			}
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
			closePipe(pipes, PIPE_SIZE);

			//exec
			if(execvp(cmds[i], paras) < 0)
			{
				printf("exec ERROR! %s\n", strerror(errno));
				semSignal(semClient2);
				exit(1);
			}
		}
		else
		{
			close(pipes[pipePtr][0]);
			while(close(pipes[pipePtr][1])) printf("close pipe in again\n");

			while(wait(&status) != execpid);

			if(isPublicPipeIn)
				remove(publicFiles[id + 1][publicPipeIn]);

			semSignal(semClient2);

			pipes[pipePtr][0] = tmpPipe[0];
			pipes[pipePtr][1] = tmpPipe[1];
		}
		paraCount += isPublicPipeIn + isPublicPipeOut;
		i += paraCount;
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

void client_handler(int sockfd, int id)
{
	unsigned long pid = getpid();
	char cmd[CMD_LENGTH];
	int cmdLength;
	int pipes[PIPE_SIZE][2];
	int pipePtr = 0;

	//init pipe
	for(int i = 0; i < PIPE_SIZE; ++i)
	{
		pipes[i][0] = -1;
		pipes[i][1] = -1;
	}

	//semWait(sem);
	printf("client%lu(%s/%s): commucation start\n", pid, cip + (id * IP_LENGTH), cport + (id * PORT_LENGTH));
	char temp[BUFFER_SIZE];
	sprintf(msg, loginStr, cname + (id * NAME_LENGTH), cip + (id * IP_LENGTH), cport + (id * PORT_LENGTH));
	sprintf(temp, "%s%s%% ", welcomStr, msg);
	write(sockfd, temp, strlen(temp));
	cmdYell(msg, strlen(msg), id, false);

	while(1)
	{
		//get client command
		cmdLength = readline(sockfd, cmd, CMD_LENGTH);

		if(cmdLength == 0)
		{
			printf("client%lu(%s/%s): user shutdown\n", pid, cip + (id * IP_LENGTH), cport + (id * PORT_LENGTH));
			return;
		}
		else if(cmdLength < 0)
		{
			write(sockfd, "Something Wrong!\n", 17);
			fprintf(stderr, "client%lu(%s/%s): readline error : %s\n", pid, cip + (id * IP_LENGTH), cport + (id * PORT_LENGTH), strerror(errno));
			return;
		}

		//client exit
		if(startWith("exit", cmd))
		{
			printf("client%lu(%s/%s): user exit\n", pid, cip + (id * IP_LENGTH), cport + (id * PORT_LENGTH));
			return;
		}
		//client printenv
		else if(startWith("printenv ", cmd))
		{
			char *env;
			if((env = getenv(cmd + 9)) != NULL)
			{
				sprintf(msg, "%s=%s\n", cmd + 9, env);
				write(sockfd, msg, strlen(msg));
			}
			else
				write(sockfd, "NOT FOUND!\n", 11);
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
				fprintf(stderr, "client%lu(%s/%s): setenv error : %s\n", pid, cip + (id * IP_LENGTH), cport + (id * PORT_LENGTH), strerror(errno));
				write(sockfd, "set fail!\n", 10);
			}
		}
		//client name
		else if(startWith("name ", cmd))
			cmdName(id, cmd + 5);
		//client who
		else if(startWith("who", cmd))
			cmdWho(id);
		//client yell
		else if(startWith("yell ", cmd))
		{
			sprintf(msg, yellStr, cname + (id * NAME_LENGTH), cmd + 5);
			cmdYell(msg, strlen(msg), id, true);
		}
		//client tell
		else if(startWith("tell ", cmd))
			cmdTell(id, cmd + 5);
		else
			//handle client command
			if(command_handler(sockfd, cmd, id, pipes, pipePtr) < 0)
				return;
		write(sockfd, "% ", 2);
	}
}

int cpid[CLIENT_NUMBERS];

void reaper(int sig)
{
	int status, pid;
	bool checkOver = false;
	while((pid = waitpid(-1, &status, WNOHANG)) >= 0 && !checkOver)
	{
		for(int i=0;i<CLIENT_NUMBERS;++i)
		{
			if(cpid[i] == pid)
			{
				semWait(semClient);

				if(close(cfd[i]) < 0)
					printf("close error %s\n", strerror(errno));
				cpid[i] = 0;
				cfd[i] = 0;

				semSignal(semClient);
				break;
			}
		}
		checkOver = true;
	}
}


void mysig(int sig)
{
	semWait(semServer);
	if(!strcmp(ctoscmd, "yell"))
	{
		int id = atoi(ctospara2), selfShow = atoi(ctospara3);
		for(int i=0; i<CLIENT_NUMBERS; ++i)
			if(cfd[i] > 0 && (selfShow || i != id))
				write(cfd[i], ctospara, strlen(ctospara));
	}
	else if(!strcmp(ctoscmd, "tell"))
	{
		int toId = atoi(ctospara2);
		write(cfd[toId], ctospara, strlen(ctospara));
	}
	semSignal(semServer2);
}

int main(int argc, char *argv[], char ** envp){
	int sockfd, newsockfd, childpid;
	struct sockaddr_in client_addr;
	unsigned int client_len;
	char dirBuff[BUFFER_SIZE];
	int newId;

	spid = getpid();

    printf("server: server starting...spid=%d\n", spid);

	//if user give port
	if(argc == 2)
                strcpy(port, argv[1]);

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

	initSEM();
	initSHM();
	initPublicFiles();

	//clear shared memory
	memset(cflag, 0, sizeof(int) * CLIENT_NUMBERS);
	memset(cfd, 0, sizeof(int) * CLIENT_NUMBERS);
	memset(cip, 0, sizeof(char) * CLIENT_NUMBERS * IP_LENGTH);
	memset(cport, 0, sizeof(char) * CLIENT_NUMBERS * PORT_LENGTH);
	memset(cname, 0, sizeof(char) * CLIENT_NUMBERS * NAME_LENGTH);
	memset(cpid, 0, sizeof(cpid));

	//set socket
	sockfd = setSocket(port);

	signal(SIGCHLD, reaper);
	signal(SIGUSR1, mysig);

	while(1)
	{
		client_len = sizeof(client_addr);
		printf("server: server is waiting...\n");

		//wait client connect
		if((newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len)) < 0)
			errexit("server: accept error : %s\n", strerror(errno));

		semWait(semClient);
		//check connect numbers and get new id
		if((newId = getNewId(cfd)) < 0)
		{
			printf("server: connect full\n");
			close(newsockfd);
			semSignal(semClient);
			continue;
		}
		cfd[newId] = newsockfd;
		strcpy(cname + (newId * NAME_LENGTH), DEFAULT_NAME);
		//strcpy(cip + (newId * IP_LENGTH), inet_ntoa(client_addr.sin_addr));
		strcpy(cip + (newId * IP_LENGTH), "CGILAB");
		//sprintf(cport + (newId * PORT_LENGTH), "%u", client_addr.sin_port);
		strcpy(cport + (newId * PORT_LENGTH), "511");
		semSignal(semClient);

		//fork child to handle client
		if((childpid = fork()) < 0)
			errexit("server: fork error : %s\n", strerror(errno));
		else if(childpid == 0) //child
		{
			close(sockfd);
			initSHM();
			closeOtherClient(newId);

			client_handler(newsockfd, newId);
			closeClientPublicFile(newId);
			cflag[newId] = 0;
			sprintf(msg, leftStr, cname + (newId * NAME_LENGTH));
			cmdYell(msg, strlen(msg), newId, true);
			usleep(100);
			return 0;
		}
		else
		{
			cflag[newId] = 1;
			cpid[newId] = childpid;
			printf ("server: fork finished, cpid = %d(%s/%s),ssock=%d, id=%d\n", childpid, cip + (newId * IP_LENGTH), cport + (newId * PORT_LENGTH), newsockfd, newId);
		}
	}

	close(sockfd);
	return 0;
}



int cmdWho(int fromId)
{
	write(cfd[fromId], whoStr, strlen(whoStr));
	for(int i=0; i<CLIENT_NUMBERS; ++i)
	{
		if(cfd[i] > 0)
		{
			sprintf(msg, whoFormatStr, i + 1, cname + (i * NAME_LENGTH), cip + (i * IP_LENGTH), cport + (i * PORT_LENGTH), (i == fromId?"\t<-me":""));
			write(cfd[fromId], msg, strlen(msg));
		}
	}
}

int cmdTell(int fromId, char *cmd)
{
	char toIdc[2];
	sscanf(cmd, "%s", toIdc);
	int toId = atoi(toIdc);

	if(toId > 0 && toId <= CLIENT_NUMBERS && cfd[toId - 1] > 0)
	{
		if(strlen(cmd + strlen(toIdc) + 1) > 1024)
			strcpy(&(cmd + strlen(toIdc) + 1)[1024], "\0");
		sprintf(msg, tellStr, cname + (fromId * NAME_LENGTH), cmd + strlen(toIdc) + 1);
		semWait(semClient);
		strcpy(ctoscmd, "tell");
		strcpy(ctospara, msg);
		sprintf(ctospara2, "%d", toId - 1);
		semSignal(semServer);
		kill(spid, SIGUSR1);
		semWait(semServer2);
		semSignal(semClient);
	}
	else
	{
		sprintf(msg, userNotExistErrorStr, toIdc);
		write(cfd[fromId], msg, strlen(msg));
	}
}

int cmdYell(const char *msg, int msgLength, int fromId, bool selfShow)
{
	semWait(semClient);
	strcpy(ctoscmd, "yell");
	if(msgLength > 1024)
		sprintf(ctospara, "%1024s", msg);
	else
		sprintf(ctospara, "%s", msg);
	sprintf(ctospara2, "%d", fromId);
	sprintf(ctospara3, "%d", selfShow);
	semSignal(semServer);
	kill(spid, SIGUSR1);
	semWait(semServer2);
	semSignal(semClient);
}

int cmdName(int fromId, char *name)
{
	if(strlen(name) > NAME_LENGTH) name[NAME_LENGTH] = '\0';
	for(int i=0; i<CLIENT_NUMBERS; ++i)
	{
		if(!strcmp(cname + (i * NAME_LENGTH), name))
		{
			sprintf(msg, nameExistStr, name);
			write(cfd[fromId], msg, strlen(msg));
			return 0;
		}
	}
	strcpy(cname + (fromId * NAME_LENGTH), name);
	sprintf(msg, nameStr, cip + (fromId * IP_LENGTH), cport + (fromId * PORT_LENGTH), name);
	cmdYell(msg, strlen(msg), fromId, true);
}


