#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "common.h"
#include "semaphore.h"

union semun{
	int val;
	struct semid_ds *buff;
	ushort *array;
} semctlArg;

int semCreate(key_t key, int initval)
{
	register int id, semval;
	bool again = false;

	if(key == IPC_PRIVATE) 
		return -1;
	else if(key == (key_t) - 1)
		return -1;

again:
	if((id = semget(key, 3, 0666 | IPC_CREAT | IPC_EXCL)) < 0)
	{
		if((id = semget(key, 3, 0)) < 0)
			return -1;
		semRm(id);
		if((id = semget(key, 3, 0666 | IPC_CREAT | IPC_EXCL)) < 0)
			return -1;
	}
	if(semop(id, &semOpLock[0], 2) < 0)
	{
		if(errno == EINVAL && !again) 
		{
			again = true;
			goto again;
		}
		errexit("semaphore: can't lock\n");
	}

	if((semval = semctl(id, 1, GETVAL, 0)) < 0) 
		errexit("semaphore: can't GETVAL\n");
	
	if(semval == 0)
	{
		semctlArg.val = initval;
		if(semctl(id, 0, SETVAL, semctlArg) < 0)
			errexit("semaphore: can't SETVAL[0]\n");
		semctlArg.val = BIGCOUNT;
		if(semctl(id, 1, SETVAL, semctlArg) < 0)
			errexit("semaphore: can't SETVAL[1]\n");
	}

	if(semop(id, &semOpEndCreate[0], 2) < 0)
		errexit("semaphore: can't end create\n");

	return id;

}
int semOpen(key_t key)
{
	register int id;
	if(key == IPC_PRIVATE) 
	{
		printf("a\n");
		return -1;
	}
	else if(key == (key_t) - 1 ) 
	{
		printf("b\n");
		return -1;
	}

	if((id = semget(key, 3, IPC_CREAT | IPC_EXCL)) < 0)  
	{
		printf("c\n");
		return -1;
	}

	if(semop(id, &semOpOpen[0], 1) < 0)
		errexit("semaphore: can't open\n");

	return id;
}


void semOp(int id, int value)
{
	if((semOpOp[0].sem_op = value) == 0)
		errexit("semaphore: can't have value == 0\n");
	if(semop(id, &semOpOp[0], 1) < 0)
		errexit("semaphore: sem_op error id=%d, value=%d,  %s\n", id, value, strerror(errno));
}

void semWait(int id)
{
	semOp(id, -1);
}

void semSignal(int id)
{
	semOp(id, 1);
}


void semClose(int id)
{
	register int semval;
	
	if(semop(id, &semOpClose[0], 3) < 0)
		errexit("semaphore: can't semop\n");

	if((semval = semctl(id, 1, GETVAL, 0)) < 0)
		errexit("semaphore: can't GETVAL\n");

	if(semval > BIGCOUNT)
		errexit("semaphore: sem[1] > BIGCOUNT");
	else if(semval == BIGCOUNT)
		semRm(id);
	else if(semop(id, &semOpUnlock[0], 1) < 0)
		errexit("semaphore: can't unlock\n");
}

void semRm(int id)
{
	if(semctl(id, 0, IPC_RMID, 0) < 0)
		errexit("semaphore: can't IPC_RMID\n");
}
