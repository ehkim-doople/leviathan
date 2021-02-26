#include "comLogger.h"
#include "comMemManager.h"
#include "comQueue.h"

CQueueS::CQueueS()
{
	memset(g_szMessage, 0, LEN_MEM_MESSAGE);
	m_pArray = NULL;
	m_nMax = 0;
	m_nFront = 0;
	m_nLast = 0;
	SPIN_LOCK_INIT(&m_cLockPush);
	SPIN_LOCK_INIT(&m_cLockPop);
	m_nOldMax = 0;
}


CQueueS::~CQueueS(void)
{
	if (!m_pArray) return;

	void *p;
	int i;
	if (ISABLETODELETE(m_nObjAllocType)) {
		for (i = 0; i < m_nMax; i++) {
			p = m_pArray[i];
			if (p) {
				switch (m_nObjAllocType) {
				case eAlloc_Type_new:	delete p; break;
				case eAlloc_Type_alloc:	free(p); break;
				case eAlloc_Type_BufPool:	gs_pMMgr->delBuf((char *)p, STRING_SIZE((TCHAR *)p)); break;
				case eAlloc_Type_newArray:	delete[] p; break;
				case eAlloc_Type_none: break;
				case eAlloc_Type_MemPool: break;
				}
			}
		}			
	}
	free(m_pArray);
	m_pArray = 0;
	SPIN_LOCK_DESTROY(&m_cLockPush);
	SPIN_LOCK_DESTROY(&m_cLockPop);
}



bool CQueueS::alloc(int nMaxCount, E_ALLOC_TYPE type)
{
	m_pArray = (void **)calloc(nMaxCount, sizeof(void*));
	if (m_pArray == 0) {
		//gs_cLogger.DebugLog(LEVEL_ERROR, "m_pArray calloc ERROR nMaxCount[%d]", nMaxCount);
		return false;
	}

	m_nMax = nMaxCount;
	m_nObjAllocType = type;
	//gs_cLogger.PutLogQueue(LEVEL_INFO, _T("CQueue<T>::alloc nMaxCount[%d]"), nMaxCount);
	return true;
}


bool CQueueS::realloc(int nMaxCount, bool bInit)
{
	void** newPtr, **oldPtr = m_pArray;
	newPtr = (void **)calloc(nMaxCount, sizeof(void*));
	if (!newPtr) {
		printf("[%p] calloc has Failed\n", m_pArray);
		m_nRealloc.init();
		m_CS.leave();
		return false;
	}
	if (bInit) {
		m_pArray = newPtr;
		m_nMax = nMaxCount;
		memset(m_pArray, 0, nMaxCount * sizeof(void*));
	}
	else {
		memcpy(newPtr, m_pArray, m_nMax * sizeof(void*));
		printf("[%p] ################### realloc success m_nLast[%d] m_nMax[%d] nMaxCount[%d]\n", m_pArray, m_nLast, m_nMax, nMaxCount);
		SPIN_LOCK_ENTER(&m_cLockPop);
		m_pArray = newPtr;
		m_nLast = m_nMax;
		m_nMax = nMaxCount;
		SPIN_LOCK_LEAVE(&m_cLockPop);
		m_nOldMax = m_nLast;
	}
	m_nRealloc.init();
	m_CS.leave();
	if(oldPtr) free(oldPtr);
	return true;
}

bool CQueueS::push(void* pData) // multi thread (Lock required)
{
	if (m_nRealloc.getCount()) {
		m_CS.enter();
		m_CS.leave();
	}
	else if(m_pArray[m_nLast]) {
		if (m_nRealloc.atomic_compare_exchange(1, 0)) {
			m_CS.enter();
			m_CS.leave();
		}
		else {
			m_CS.enter();
			if (!realloc(m_nMax << 1)) {
				return false;
			}
		}
	}

	SPIN_LOCK_ENTER(&m_cLockPush);
	//if (m_pArray[m_nLast]) {
	//	//nTmp = m_nLast; nTmp2 = m_nMax;
	//	printf("[%p] Already data Set! m_nLast[%d] nMax[%d]\n", m_pArray, m_nLast, m_nMax);
	//}
	m_pArray[m_nLast] = pData;
	m_nLast++;
	if (m_nLast == m_nMax) m_nLast = 0;
	//printf("[%p] m_nLast[%d] nMax[%d]\n", m_pArray, nTmp, m_nMax);
	SPIN_LOCK_LEAVE(&m_cLockPush);
	//if (0 <= nTmp) {
	//	gs_cLogger.DebugLog(LEVEL_ERROR, "Already data Set! m_nLast[%d] nMax[%d]", nTmp, nTmp2);
	//}
	return true;
}

void* CQueueS::pop() // multi thread (Lock required)
{
	void* res;
	SPIN_LOCK_ENTER(&m_cLockPop);
	res = m_pArray[m_nFront];
	if (res) {
		m_pArray[m_nFront] = 0;
		//printf("[%p] m_nFront[%d] nMax[%d]\n", m_pArray, m_nFront, nMax);
		m_nFront++;
		if (m_nOldMax) {
			if(m_nFront == m_nOldMax) m_nFront = 0;
		}
		else if (m_nFront == m_nMax) m_nFront = 0;
	}
	else if (m_nOldMax) {
		//nTmp = m_nFront; nTmp2 = m_nOldMax;
		res = m_pArray[m_nOldMax]; 
		m_pArray[m_nOldMax] = 0;
		//printf("[%p] m_nFront[%d] m_nOldMax[%d]\n", m_pArray, m_nFront, m_nOldMax);
		m_nFront = m_nOldMax+1;
		m_nOldMax = 0;
	}
	SPIN_LOCK_LEAVE(&m_cLockPop);
	//if (0 <= nTmp) {
	//	gs_cLogger.DebugLog(LEVEL_INFO, "m_nFront[%d] m_nOldMax[%d]", nTmp, nTmp2);
	//}
	return res;
}

