#pragma once

#include <vector>
#include "RTSQueue.h"

struct STThreadInfo
{
	HANDLE hTHID;
	void *pClass;
	bool bActive;
	int nSeqNum;
};

struct STComplete
{
	int bSuccess; 
	LPOVERLAPPED lpOverlapped;
	unsigned long bytesTrans;
	void *lpCompletion;
	void *pChild;
};

typedef void (*func_type) (STComplete *p);
typedef std::vector<STThreadInfo *>	THREADVECTOR;


class CRTSQueueHandler : public CRTSQueue
{
public :
	CRTSQueueHandler();
	virtual ~CRTSQueueHandler();
	int init(int idx);
	int start(func_type pFunction, void *pChild);

protected :
	int init(int nMax, int nThreadCnt);

private :
	HANDLE		m_hCreateThread;
	static THREAD_RET_TYPE THREAD_CALLING_CONVENTION WorkerThread(void *ptr);
	THREADVECTOR	m_WorkerThreadVector;
	void	destroyWorkerThread(HANDLE hID);
};