#ifndef SEMAPHORE_H

#define SEMAPHORE_H

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#define SEM_SERVER 77320
#define SEM_SERVER2 77321
#define SEM_CLIENT 77322
#define SEM_CLIENT2 77323

static int BIGCOUNT = 100;

static struct sembuf semOpLock[2] = { 
	2, 0, 0,
	2, 1, SEM_UNDO
};

static struct sembuf semOpUnlock[1] = {
	2, -1, SEM_UNDO
};

static struct sembuf semOpEndCreate[2] = {
	1, -1, SEM_UNDO,
	2, -1, SEM_UNDO
};

static struct sembuf semOpOpen[1] = {
	1, -1, SEM_UNDO
};

static struct sembuf semOpClose[3] = {
	2, 0, 0,
	2, 1, SEM_UNDO,
	1, 1, SEM_UNDO
};

static struct sembuf semOpOp[1] = {
	0, 99, SEM_UNDO
};

int semCreate(key_t key, int initval);
int semOpen(key_t key);

void semOp(int id, int value);
void semWait(int id);
void semSignal(int id);

void semClose(int id);
void semRm(int id);

#endif
