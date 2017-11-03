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


#define CLIENT_NUMBERS 5
#define CMD_LENGTH 15000
#define CMDS_SIZE 5000
#define BUFFER_SIZE 256
#define PIPE_SIZE 1001
#define BIN_FILES_SIZE 20

#define DEFAULT_PATH "bin:."
#define DEFAULT_DIR "ras"

using namespace std;

int port = 7575;
const char welcomStr[] = "****************************************\n** Welcome to the information server. **\n****************************************\n% ";

void doubleArray(char **arr, int &size)
{
	size *= 2;
	char *tmpArr[size];
	for(int i = 0; i < size / 2; ++i)
	{
		strcpy(tmpArr[i], arr[i]);
		delete [] arr[i];
	}
	delete [] arr;
	arr = tmpArr;
}


void split(char **arr, int &len, char *str, const char del)
{
	char *s = strtok(str, &del);
	len = 0;
	while(s != NULL)
	{
		*arr++ = s;
		s = strtok(NULL, &del);
		++len;
	}
}

void closePipe(int pipes[PIPE_SIZE][2])
{
	for(int i = 0; i < PIPE_SIZE; ++i)
	{
		if(pipes[i][0] >= 0) 
		{
			if(close(pipes[i][0]) < 0)
				fprintf(stderr, "client: close pipe %d error : %s\n", pipes[i][0], strerror(errno));
			pipes[i][0] = -1;
		}
        
		if(pipes[i][1] >= 0)
		{
            if(close(pipes[i][1]))
				fprintf(stderr, "client: close pipe %d error : %s\n", pipes[i][1], strerror(errno));
            pipes[i][1] = -1;
        }
	}
}

int getBinFile(char **files, int &count, int &size)
{
	DIR *dir;
	struct dirent *ent;
	char *path = getenv("PATH");
	char *paths[BIN_FILES_SIZE];
	int pathCount = 0;

	split(paths, pathCount, path, ':');

	count = 0;
	for(int i = 0; i < pathCount; ++i)
		if((dir = opendir(paths[i])) != NULL)
			while((ent = readdir(dir)) != NULL)
			{
				if(count == size)
					doubleArray(files, size);
				files[count++] = ent->d_name;
			}
		else
			return -1;
}

int readline(int fd, char *str, int maxlen)
{
	int n, rc;
	char c;
	for(n = 1; n < maxlen; ++n)
	{
		if((rc = read(fd, &c, 1))==1)
		{
			if(c == '\n') break;
			if(c == '\r') continue;
			*str++ = c;
		}
		else if(rc == 0)
		{
			if(n == 1) return 0;
			else break;
		}
		else return -1;
	}
	*str = 0;
	return n;
}

int command_handler(int sockfd, char *cmd, int pipes[PIPE_SIZE][2], int &pipePtr)
{
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
	if(cmdCount > 1 && cmds[cmdCount - 2][0] == '>')
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
		
		//check valid cmd
		for(int j = 0; j < binFileCount; ++j)
			if(strcmp(cmds[i], binFile[j]) == 0)
			{
				isValidCmd = true;
				break;
			}
		if(!isValidCmd)
		{
			write(sockfd, "Unknown Command: ", 17);
			write(sockfd, cmds[i], strlen(cmds[i]));
			write(sockfd, "\n", 1);
			if(i==0)
				return 1;
			else
				break;
		}
		
		//create temp pipe	
		if(pipe(tmpPipe) < 0){
            fprintf(stderr, "client: can't create temp pipe : %s\n", strerror(errno));
            return -1;
        }

		//count parameter
		while((i + paraCount < cmdCount - isPipe - isErrorPipe - isFilePipe - isAllPipe) && strcmp(cmds[i + paraCount], "|" ) != 0)
			paraCount++;
		
		isLastCmd = (i + paraCount == cmdCount - isPipe - isErrorPipe - isFilePipe - isAllPipe);
		
		//fork exec
		if((execpid = fork()) < 0)
		{
			fprintf(stderr, "client: can't fork exec : %s\n", strerror(errno));
			write(sockfd, "Command [", 9);
			write(sockfd, cmds[i], strlen(cmds[i]));
			write(sockfd, "] exec error!\n", 14);
			return 1;
		}
		else if(execpid == 0) //exec child
		{
			//set parameter
			char **paras;
			paras = new char*[paraCount + 1];
			for(int j = 0; j < paraCount; ++j)
				paras[j] = cmds[i + j];
			paras[paraCount] = 0;

			//set input pipe
			close(STDIN_FILENO);
			dup(pipes[pipePtr][0]);

			//set out pipe
			close(STDOUT_FILENO);
			if(isLastCmd)
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
			if(isErrorPipe)
				dup(pipes[(pipePtr + errorPipeTo) % PIPE_SIZE][1]);
			else if(isAllPipe)
				dup(pipes[(pipePtr + allPipeTo) % PIPE_SIZE][1]);
			else
				dup(sockfd);			
	
			//close other pipes
			close(sockfd);
			close(tmpPipe[0]);
			close(tmpPipe[1]);
			closePipe(pipes);

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
			close(pipes[pipePtr][1]);
			
			while(wait(&status) != execpid);

			pipes[pipePtr][0] = tmpPipe[0];
			pipes[pipePtr][1] = tmpPipe[1];
		}

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

void client_handler(int sockfd, char *c_ip)
{
	int cpid;
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

	//send welcome msg
	printf("client%d(%s): commucation start\n", pid, c_ip);
	write(sockfd, welcomStr, sizeof(welcomStr));

	while(1)
	{
		//get client command
		cmdLength = readline(sockfd, cmd, CMD_LENGTH);

		if(cmdLength == 0) 
		{
			printf("client%d(%s): user shutdown\n", pid, c_ip);
			return;
		}
		else if(cmdLength < 0)
		{
			write(sockfd, "Something Wrong!\n", 17);
			fprintf(stderr, "client%d(%s): readline error : %s\n", pid, c_ip, strerror(errno));
			return;
		}

		//client exit
		if(strstr(cmd, "exit") != NULL)
		{
			closePipe(pipes);
			printf("client%d(%s): user exit\n", pid, c_ip);
			return;
		}
		//client printenv
		else if(strstr(cmd, "printenv") != NULL)
		{
			char *env;
			if((env = getenv(cmd + 9)) != NULL)
			{
				write(sockfd, cmd + 9, strlen(cmd + 9));
				write(sockfd, "=", 1);
				write(sockfd, env, strlen(env));
				write(sockfd, "\n", 1);
			}
			else
				write(sockfd, "NOT FOUND!\n", 11);
		}
		//client setenv
		else if(strstr(cmd, "setenv") != NULL)
		{
			char *env[3];
			int c;
			split(env, c, cmd, ' ');
			if(env[2][strlen(env[2]) - 1] == '\n') env[2][strlen(env[2]) - 1] = '\0';
			if(setenv(env[1], env[2], 1) < 0)
			{
				fprintf(stderr, "client%d(%s): setenv error : %s\n", pid, c_ip, strerror(errno));
				write(sockfd, "set fail!\n", 10);
			}
		}
		else
			//handle client command
			if(command_handler(sockfd, cmd, pipes, pipePtr) < 0)
				return;

		write(sockfd, "% ", 2);
	}
}

void reaper(int sig)
{
	//union wait status;
	int status;
	while(wait3(&status, WNOHANG, (struct rusage *) 0) >= 0 );
}

int main(int argc, char *argv[]){
	int sockfd, newsockfd, childpid;
	struct sockaddr_in server_addr, client_addr;
	unsigned int client_len;
	char *client_ip;
	char dirBuff[BUFFER_SIZE];

	if(argc == 2)
                port = atoi(argv[1]);

        printf("server: server sarting...\n");

	//change dir
	if(chdir(DEFAULT_DIR) < 0)
	{
		fprintf(stderr, "server: can't change dir : %s\n", strerror(errno));
		exit(errno);
	}
	getcwd(dirBuff, BUFFER_SIZE);
	printf("server: current working directory : %s\n", dirBuff);

	//change path
	if(setenv("PATH", DEFAULT_PATH, 1) < 0)
	{
		fprintf(stderr, "server: can't change path : %s\n", strerror(errno));
		exit(errno);
	}
	printf("server: current working PATH = %s\n", getenv("PATH"));

	//set socket
	if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "server: can't open stream socket : %s\n", strerror(errno));
		exit(errno);
	}
	
	//init
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	//bind socket
	if(bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
	{
		fprintf(stderr, "server: can't bind local address : %s\n", strerror(errno));
		exit(errno);
	}

	//listen socket
	listen(sockfd, CLIENT_NUMBERS);
	printf("server: server port is %d, and can handle %d clients. \n", port, CLIENT_NUMBERS);

	//signal(SIGCHLD, reaper);

	while(1)
	{
		client_len = sizeof(client_addr);
		printf("server: server is waiting...\n");
		
		//wait client connect
		if((newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len)) < 0)
		{
			fprintf(stderr, "server: accept error : %s\n", strerror(errno));
			exit(errno);
		}

		//get client ip
		client_ip = inet_ntoa(client_addr.sin_addr);
		printf("server: %s is linking...%d\n", client_ip, newsockfd);

		//fork child to handle client
		if((childpid = fork()) < 0)
		{
			fprintf(stderr, "server: fork error : %s\n", strerror(errno));
			close(newsockfd);
		}
		else if(childpid == 0) //child
		{
			close(sockfd);
			client_handler(newsockfd, client_ip);
			close(newsockfd);
			printf("server: cpid = %d(%s) is quit...\n", getpid(), client_ip);
			return 0;
		}
		else
		{
			close(newsockfd);
			printf ("server: fork finished, cpid = %d(%s)\n", childpid, client_ip);
			//waitpid(childpid, 0, 0);
		}
	}

	close(sockfd);
	return 0;
}
