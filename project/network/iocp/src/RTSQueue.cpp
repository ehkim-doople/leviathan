#include "RTSQueue.h"

CRTSQueue *g_pErrIocpQueue;

CRTSQueue::CRTSQueue()
{
	m_pEventMemMgr = NULL;
	m_nThreadCount = 0;
	m_hRTSQueue = NULL;
}



CRTSQueue::~CRTSQueue()
{
	if(m_pEventMemMgr) {
		delete m_pEventMemMgr;
	}
	CloseHandle(m_hRTSQueue);
	
}

int CRTSQueue::init(int nMax, int nThreadCnt)
{
	if(!nMax) return CERROR_NONE;
	m_nThreadCount = nThreadCnt;

	if(!m_pEventMemMgr) { 
		nMax = nMax << 1;
		nMax += 100; // 상수개를 더해야함
		m_pEventMemMgr = new CMemPool<STOVERLAPPED>();	
		if (m_pEventMemMgr->alloc(nMax)) {
			m_hRTSQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_nThreadCount);
			return CERROR_NONE;
		}
	}
	else {
		_stprintf(g_szMessage, _T("%s"), err_desc_socket[CERROR_ALREADY_ALLOC]);
		comErrorPrint(g_szMessage);
		return CERROR_ALREADY_ALLOC;
	}
	_stprintf(g_szMessage, _T("[%s] m_hRTSQueue[%p] nMax[%d] nThreadCnt[%d]"), err_desc_socket[CERROR_NEW_MEMORY], m_hRTSQueue, nMax, nThreadCnt);
	comErrorPrint(g_szMessage);
	return CERROR_NEW_MEMORY;
}

int CRTSQueue::putRTSQueue(int nOPCode, void *pCompletionKey)
{
	STOVERLAPPED *lpOverlapped = m_pEventMemMgr->newMem();
	if(lpOverlapped == NULL) {
		gs_cLogger.DebugLog(LEVEL_ERROR, "nTot[%d] nCur[%d]", m_pEventMemMgr->GETMAXCOUNT(), m_pEventMemMgr->GETUSECOUNT());
		return CERROR_NOMEMORY;
	}
	lpOverlapped->opCode = nOPCode;
	BOOL bSuccess = PostQueuedCompletionStatus(m_hRTSQueue, 0, (ULONG_PTR)pCompletionKey,(LPOVERLAPPED)lpOverlapped);
	if ( !bSuccess && WSAGetLastError() != ERROR_IO_PENDING ) {
		gs_cLogger.DebugLog(LEVEL_ERROR, "[%s] m_hRTSQueue[%p]", err_desc_socket[CERROR_PQCS], m_hRTSQueue);
		return CERROR_PQCS;
	}
	return CERROR_NONE;
}

int CRTSQueue::registerSocket(HANDLE hFD, void *pCompletionKey)
{
	if(CreateIoCompletionPort(hFD, m_hRTSQueue, (ULONG_PTR)pCompletionKey, 0)) return CERROR_NONE;
	return CERROR_REGIOCP;
}


