//#include <iostream>
//#include <string>
//#include <map>
//#include <tuple>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <time.h>
#define SHMKEY ((key_t) 7890) /* base value for shmem key */
#define PERMS 0666

typedef struct {
    int id;
    int fd;
    int pid;
    char name[100];
    char ip[50];
    int port;
    int *pipenum_arr; //local allocate
    int *pipe_open_flag_arr; //local allocate
    int *pipefd[50000]; //local allocate
    int cmd_counter;
    int msgbuffer[1000];
} client;


int main(){
/*
    int forkid;
    int shmid;
    printf("%zu\n", sizeof(client));
    if((shmid = shmget(SHMKEY, sizeof(client) * 30, PERMS|IPC_CREAT)) < 0){
        printf("get error\n");;
    }
    if((forkid = fork()) < 0  ) printf("fork ERROR\n");
    else if(forkid == 0){//child
        sleep(1);

        client *tmp;
        tmp = (client*)shmat(shmid, (char *)0, 0);
        printf("child:%d\n",tmp[0].id);
        shmdt(tmp);
        shmctl(shmid, IPC_RMID, NULL);
        exit(0);
    }
    else {//parent
        client *tmp;
        tmp = (client*)shmat(shmid, (char *)0, 0);
        tmp[0].id = 87;
        printf("par: write finish\n");
        shmdt(tmp);
    }*/

    srand(time(NULL));
    int sum=0,a=0;
    printf("%d\n",sum);
    for(int i = 0; i<10000; i++){
    	a = rand()%2;
    	if(a > 0)
    		sum +=a;
    }
    printf("%d\n",sum);
        return 0;


}
