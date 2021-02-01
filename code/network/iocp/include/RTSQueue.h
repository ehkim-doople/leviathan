
/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/
#pragma once

#include "comMemPool.h"
#include "NWGlobal.h"

class CRTSQueue
{
public :
	CRTSQueue();
	virtual ~CRTSQueue();

	int init(int nMax, int nThreadCnt);
	int putRTSQueue(int nOPCode, void *pCompletionKey);
	int registerSocket(HANDLE hFD, void *pCompletionKey);

	inline int deleteEvent(STOVERLAPPED *pData) {	if(m_pEventMemMgr->delMem(pData)) return CERROR_NONE; return CERROR_DEL_MEMORY; }
	inline HANDLE getRTSQueue() { return m_hRTSQueue; }
	inline CRTSQueue * getCRTSQueue() { return this; }
	inline int getThreadCount() { return m_nThreadCount; }

private :
	int m_nThreadCount;
	HANDLE m_hRTSQueue;
	CMemPool<STOVERLAPPED> * m_pEventMemMgr;
};

extern CRTSQueue *g_pErrIocpQueue;