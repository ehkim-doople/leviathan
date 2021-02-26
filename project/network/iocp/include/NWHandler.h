/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

#pragma once

#include <vector>
#include "RTSQueue.h"

struct STNWThreadInfo
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
typedef std::vector<STNWThreadInfo *>	THREADVECTOR;


class CNWHandler : public CRTSQueue
{
public :
	CNWHandler();
	virtual ~CNWHandler();
	bool init(int nMax, int nThreadCnt) { if (!CRTSQueue::init(nMax, nThreadCnt)) return true; return false; }
	bool start(func_type pFunction, void *pChild);
	inline void * getChild() { return m_pChild; }
	func_type m_fProcessLoop;
protected :

private :
	void *m_pChild;
	HANDLE		m_hCreateThread;
	static THREAD_RET_TYPE THREAD_CALLING_CONVENTION WorkerThread(void *ptr);
	THREADVECTOR	m_WorkerThreadVector;
	void	destroyWorkerThread(HANDLE hID);
};