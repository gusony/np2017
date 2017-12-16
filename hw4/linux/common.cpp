#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/shm.h>
#include <string>
#include "common.h"

using namespace std;

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

void newSplit(char arr[CMDS_SIZE][BUFFER_SIZE], int &len, char *str, const char *del)
{
	len = 0;
	string s = string(str);
	int pt = s.find(del);
	while(pt != string::npos)
	{
		strcpy(arr[len++], s.substr(0, pt).data());
		s = s.substr(pt + strlen(del), s.length());
		pt = s.find(del);
	}
	strcpy(arr[len++], s.data());
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

int readUntilNull(int fd, char *str, int maxlen)
{
	int n, rc;
	char c;
	for(n = 1; n < maxlen; ++n)
	{
		if((rc = read(fd, &c, 1))==1)
		{
			if(c == '\0') break;
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

bool startWith(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void closePipe(int pipes[][2], int pipeSize)
{
	for(int i = 0; i < pipeSize; ++i)
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

int getShmID(char *key, int no, int size)
{
	key_t SK;
	int ID;

	if((SK = ftok(key, no)) < 0)
		errexit("server: can't create shared memory key with str=%s, no=%d: %s\n", key, no, strerror(errno));
	if((ID = shmget(SK, size, IPC_CREAT | IPC_EXCL | 0666)) < 0)
		if((ID = shmget(SK, size, 0)) < 0)
			errexit("server: can't get shared memory segment with str=%s, no=%d: %s\n", key, no, strerror(errno));
	return ID;
}


int getShmID(key_t key, int size)
{
	int ID;
	if((ID = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666)) < 0)
		if((ID = shmget(key, size, 0)) < 0)
			errexit("server: can't get shared memory segment with SK=%d: %s\n", key, strerror(errno));
	return ID;
}


void errexit(const char *message)
{
	fprintf(stderr, message);
	exit(0);
}

void errexit(const char *message, const char **format)
{
	fprintf(stderr, message, *format);
	exit(0);
}

void errexit(const char *message, const int format1)
{
	fprintf(stderr, message, format1);
	exit(0);
}
void errexit(const char *message, const char *format1)
{
	fprintf(stderr, message, format1);
	exit(0);
}

void errexit(const char *message, const char *format1,  const char *format2)
{
	fprintf(stderr, message, format1, format2);
	exit(0);
}

void errexit(const char *message, const int format1,  const char *format2)
{
	fprintf(stderr, message, format1, format2);
	exit(0);
}

void errexit(const char *message, const char *format1, const char *format2, const char *format3)
{
	fprintf(stderr, message, format1, format2, format3);
	exit(0);
}

void errexit(const char *message, const char *format1, const int format2, const char *format3)
{
	fprintf(stderr, message, format1, format2, format3);
	exit(0);
}

void errexit(const char *message, const int format1, const int format2, const char *format3)
{
        fprintf(stderr, message, format1, format2, format3);
        exit(0);
}
