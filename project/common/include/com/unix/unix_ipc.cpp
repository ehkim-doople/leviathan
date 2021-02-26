#include	<stdio.h>
#include	<string.h>
#include	<errno.h>
#include	<sys/msg.h>
#include	<sys/shm.h>
#include	<sys/sem.h>
#include	"wvIPC.h"


int	
WVIPC::CreateMQ(int	key, int createFlag)
{

	int	mqid;
	int	cFlag = 0;

	if (createFlag)
		cFlag |= IPC_CREAT | 0666;

	mqid = msgget((key_t)key, cFlag);

	if (mqid == -1) 
		printf("Message Queue Create Error [%s]\n", strerror(errno));

	return mqid;
}

int
WVIPC::DestroyMQ(int nMQId)
{
	if (msgctl(nMQId, IPC_RMID, NULL) == -1) {
		printf("Message Queue Destroy Error [%s]\n", strerror(errno));
		return 0;
	}

	return 1;
}



int

WVIPC::GetMQ(int nQID, long nMsgType, char *pBuf, int nBLen)
{
	size_t	nRSize;
	char	*msgPtr;

	msgPtr = new char [nBLen + sizeof(long)];
	if (msgPtr == NULL) {
		printf("Memory Allocation Error\n");
		return -1;
	}

	memset(msgPtr, 0, nBLen + sizeof(long));
	nRSize = msgrcv(nQID, msgPtr, nBLen, nMsgType, IPC_NOWAIT);
	memcpy(pBuf, msgPtr + sizeof(long), nBLen);

	delete msgPtr;
	return nRSize;
}



int
WVIPC::GetMQWithType(int nQID, long *nMsgType, char *pBuf, int nBLen)
{

	size_t	nRSize;
	char	*msgPtr;

	msgPtr = new char [nBLen + sizeof(long)];
	if (msgPtr == NULL) {
		printf("Memory Allocation Error\n");
		return -1;
	}

	memset(msgPtr, 0, nBLen + sizeof(long));
	nRSize = msgrcv(nQID, msgPtr, nBLen, *nMsgType, IPC_NOWAIT);
	memcpy(pBuf, msgPtr + sizeof(long), nBLen);
	*nMsgType = *(long *)msgPtr;

	delete msgPtr;
	return nRSize;
}





int	

WVIPC::PutMQ(int nQID, long nMsgType, char *pBuf, int nBLen)
{
	size_t	nRet;
	char	*msgPtr;
	
	msgPtr = new char [nBLen + sizeof(long)];

	if (msgPtr == NULL) {
		printf("Memory Allocation Error\n");
		return -1;
	}


	*((long *)msgPtr) = nMsgType;
	memcpy(msgPtr + sizeof(long),  pBuf, nBLen);
	nRet = msgsnd(nQID, msgPtr, nBLen, IPC_NOWAIT);

	delete msgPtr;
	return nRet;
}



void	*

WVIPC::CreateSM(int key, int nSize, int creatFlag, int *shID)
{

	long	shmaddr;
	int	shmid;
	int	shmFlag = 0;

	if (creatFlag)
		shmFlag |= IPC_CREAT | 0666;

	shmid = shmget((key_t)key, nSize, shmFlag);
	if (shmid == -1) {
		printf("Shared memory Error [%s]\n", strerror(errno));
		return NULL;
	}
	
	*shID = shmid;
	shmaddr = (long)shmat(shmid, (void *)0, 0);
	if (shmaddr == -1) {
		printf("Shared memory attach error  err:%d[%s]\n", errno, strerror(errno));
		shmaddr = 0;
	}

	return (void *)shmaddr;
}



int

WVIPC::DestroySM(int nSMId, int destroyFlag, void *pBuf)
{
	int	nRet = 1;

	if (destroyFlag == 0)
		nRet = shmdt((const void *)pBuf);
	else 
		nRet = shmctl(nSMId, IPC_RMID, NULL);


	if (nRet == -1) {
		printf("Shared memory destroy Error %x[%u] [%s]\n",
					nSMId, nSMId, strerror(errno));
		return 0;
	}
	return 1;
}

int	
WVIPC::CreateSEM(int	key, int createFlag, int nCount)
{
	int	semid;
	int	cFlag = 0;

	if (createFlag)
		cFlag |= IPC_CREAT | 0666;

	semid = semget((key_t)key, nCount, cFlag);

	if (semid == -1) 
		printf("Semaphore Create Error [%s]\n", strerror(errno));
	
	return semid;
}

int
WVIPC::DestroySEM(int nSEMID)
{
	if (nSEMID == -1)
		return 0;

	if (semctl(nSEMID, 0, IPC_RMID, NULL) == -1) {
		printf("Message Queue Destroy Error [%s]\n", strerror(errno));
		return 0;
	}

	return 1;
}

int
WVIPC::GetSEMValue(int nSEMID, int nSemNo)
{
	int	nValue;

	if (nSEMID == -1)
		return -1;

	nValue = semctl(nSEMID, nSemNo, GETVAL);
	return nValue;
}

int	
WVIPC::PutSEMValue(int nSEMID, int nValue, int nSemNo)
{
	union	semun {
		int	val;
		struct semid_ds	*buf;
		unsigned short	*array;
	} arg;

	int	nRet;

	if (nSEMID == -1)
		return -1;

	memset(&arg, 0, sizeof(arg));
	arg.val = nValue;

	nRet = semctl(nSEMID, nSemNo, SETVAL, &arg);

	return nRet;
}

int	WVIPC::EnterSEM(int nSEMID, int nSemNo)
{
	struct	sembuf sbuf;

	if (nSEMID == -1)
		return -1;

	memset(&sbuf, 0, sizeof(sbuf));
	sbuf.sem_num = nSemNo;
	sbuf.sem_op = -1;
	sbuf.sem_flg = 0x00600;
	return	semop(nSEMID, &sbuf, 1);
}

int	WVIPC::LeaveSEM(int nSEMID, int nSemNo)
{
	struct	sembuf sbuf;

	if (nSEMID == -1)
		return -1;

	memset(&sbuf, 0, sizeof(sbuf));
	sbuf.sem_num = nSemNo;
	sbuf.sem_op = 1;
	sbuf.sem_flg = 0x00600;
	return	semop(nSEMID, &sbuf, 1);
}

