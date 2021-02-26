#include "comLogger.h"
#include "comBufPool.h"

static const int g_nSizeList[IDX_BUF_MAX] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152 };

CBufPool::CBufPool(void)
{
	m_nAllocCount = 0;
	m_nLastCount = 0;
	memset(g_szMessage, 0 ,LEN_MEM_MESSAGE); 
	m_pChunk = NULL;
	m_nFlagArray = NULL;
	m_nShift = 0;
	m_pEnd = NULL;
	//m_nOverCnt.init();
	//m_nOverMaxCnt.init();
}

CBufPool::~CBufPool(void)
{
    if(m_pChunk) {
        free(m_pChunk);
        m_pChunk = NULL;
    }

    if(m_nFlagArray) {
        delete [] m_nFlagArray;
        m_nFlagArray = NULL;
    }	
}

bool CBufPool::alloc(int nShift, int nMaxCount)
{
	m_nFlagArray = new (std::nothrow) atomic_nr[nMaxCount];
	if(m_nFlagArray == NULL) {
		gs_cLogger.DebugLog(LEVEL_ERROR, "new m_nFlagArray");
		return false;
	}
	for (m_nLastCount = 0; m_nLastCount < nMaxCount; m_nLastCount++)
	{
		m_nFlagArray[m_nLastCount].init();
	}

	m_pChunk = (char *)calloc(nMaxCount, getSizeByIndex(nShift));

    if(m_pChunk == NULL) {
		gs_cLogger.DebugLog(LEVEL_ERROR, "m_pChunk alloc");
		delete[] m_nFlagArray;
		m_nFlagArray = NULL;
		return false;
    }

    m_nAllocCount   = nMaxCount;
	m_nLastCount = 0;
	m_nUseCnt.init();
	m_nShift = nShift;
	m_pEnd = &m_pChunk[(m_nAllocCount-1)<< m_nShift];
	return true;
}


void CBufPool::clear()
{
	int i;
	m_nUseCnt.init();
	m_nLastCount = 0;
	for(i = 0; i < m_nAllocCount; i++)
	{
		m_nFlagArray[i].init();
	}
	memset(m_pChunk, 0, getSizeByIndex(m_nShift)*m_nAllocCount);
}


void CBufPool::delMem(char *pUsedMem)	// random push
{
	if(!ISINCLUDE_ADDRESS(pUsedMem)) {
		free(pUsedMem);
		//m_nOverCnt.atomic_decrement();
		return;
	}

	int nPos = (int)(pUsedMem - m_pChunk);
	int nIdx = nPos >> m_nShift;

    if(pUsedMem == &m_pChunk[nPos]) {
		m_nFlagArray[nIdx].init();
		m_nUseCnt.atomic_decrement();
		m_nLastCount = nIdx;
	}
}

void CBufPool::delMem(char *pUsedMem, int nCount)
{
	int i;
	int nPos = (int)(pUsedMem - m_pChunk);
	int nIdx = nPos >> m_nShift;

	if (pUsedMem != &m_pChunk[nPos]) return;
	for (i = 0; i < nCount; i++)
	{
		if (nIdx >= m_nAllocCount) return;
		//printf("CBufPool::delMem nPos[%d] nIdx[%d] nShift[%d] pMem[%p]\n", nPos, nIdx, nShift, pUsedMem);
		if (m_nFlagArray[nIdx].getCount())
		{
			m_nFlagArray[nIdx].init();
			m_nUseCnt.atomic_decrement();
		}
		nIdx++;
	}
	m_nLastCount = nIdx - nCount;
}

char * CBufPool::newMem()        // random pop
{
	int i= m_nLastCount,j;
	if(m_nUseCnt.getCount() == m_nAllocCount) goto NEW_ALLOC;
	char *pRes = NULL;
	
	for(j = 0; j < m_nAllocCount; j++)
    {
		if(i >= m_nAllocCount) {i = 0;} // 큐방식 만큼 타이트하지 않다. 대략 next 메모리 위치를 가리키기 위함이다.
		if(!m_nFlagArray[i].getCount()) 
		{
			if(!m_nFlagArray[i].atomic_compare_exchange(1,0)) 
			{
				m_nUseCnt.atomic_increment();
				m_nLastCount=i;
				pRes = &m_pChunk[i << m_nShift];
				//memset(pRes, 0, g_nSizeList[m_nShift]);
				pRes[0] = 0;
				return pRes;
			}
		}
		i++;
    }    
NEW_ALLOC:
	//if (m_nOverMaxCnt.getCount() < m_nOverCnt.getCount()) {
	//	m_nOverMaxCnt.atomic_exchange(m_nOverCnt.getCount());
	//	gs_cLogger.DebugLog(LEVEL_WARN, "update m_nOverMaxCnt[%d]", m_nOverMaxCnt.getCount());
	//}
	char * p = (char *)calloc(1, g_nSizeList[m_nShift]);
	//if (p) {
	//	m_nOverCnt.atomic_increment();
	//	gs_cLogger.DebugLog(LEVEL_TRACE, "update m_nOverCnt[%d] m_nShift[%d] m_nAllocCount[%d]", m_nOverCnt.atomic_increment(), m_nShift, m_nAllocCount);
	//}
	//else {
	//	gs_cLogger.DebugLog(LEVEL_ERROR, "calloc has Failed");
	//}
	return p;
}


bool CBufPool::newMemList(int nCount) // 선형의 연속된 메모리 공간 할당, not thread safty! must lock from outside
{
	int idx = m_nLastCount, nLastLoopCnt=0;
	int i;
	if (m_nUseCnt.getCount()+nCount > m_nAllocCount) return false;

	for (i=0; i < nCount; i++) {
		if (idx >= m_nAllocCount) break;
		if (!m_nFlagArray[idx].getCount())
		{
			if (!m_nFlagArray[idx].atomic_compare_exchange(1, 0)) {
				m_nUseCnt.atomic_increment();
				nLastLoopCnt++;
			}
			else break;
		}
		else break;
		idx++;
	}

	if (nLastLoopCnt== nCount) {
		m_nLastCount = idx;
		return true;
	}

	for (idx = m_nLastCount; idx < nLastLoopCnt; idx++) {
		if (m_nFlagArray[idx].getCount())
		{
			m_nFlagArray[idx].init();
			m_nUseCnt.atomic_decrement();
		}
	}
	return false;
}









//####################################################################################



CSBufPool::CSBufPool(void)
{
	m_nAllocCount = 0;
	memset(g_szMessage, 0, LEN_MEM_MESSAGE);
	m_pChunk = NULL;
	m_nUseCnt = 0;
	m_nFlagArray = NULL;
	m_pEnd = NULL;
	m_nShift = 0;
}


CSBufPool::~CSBufPool(void)
{
	destroy();
}


void CSBufPool::destroy()
{
	if (m_pChunk) {
		free(m_pChunk);
		m_pChunk = NULL;
	}

	if (m_nFlagArray) {
		free(m_nFlagArray);
		m_nFlagArray = NULL;
	}
}



bool CSBufPool::alloc(int nShift, int nMaxCount)
{
	int nFCount = (nMaxCount >> 3) + 1;
	m_nFlagArray = (char *)calloc(nFCount, sizeof(char));

	if (m_nFlagArray == NULL) {
		gs_cLogger.DebugLog(LEVEL_ERROR, "m_nFlagArray alloc");
		return false;
	} 
	m_nShift = nShift;
	m_pChunk = (char *)calloc(nMaxCount, getSizeByIndex(m_nShift));

	if (m_pChunk == NULL) {
		free(m_pChunk);
		m_pChunk = NULL;
		gs_cLogger.DebugLog(LEVEL_ERROR, "m_pChunk alloc");
		return false;
	}

	m_nAllocCount = nMaxCount;
	m_pEnd = &m_pChunk[(m_nAllocCount - 1)<< m_nShift];
	_stprintf(g_szMessage, _T("mem allock list m_nAllocCount[%d]"), m_nAllocCount);

	return true;
}



void CSBufPool::clear()
{
	memset(m_nFlagArray, 0, sizeof(m_nFlagArray));
	m_nUseCnt = 0;
	memset(m_pChunk, 0, getSizeByIndex(m_nShift)*m_nAllocCount);
	memset(g_szMessage, 0, sizeof(g_szMessage));
}




char * CSBufPool::newMem(int *pKey)        // random pop
{
	int i = m_nUseCnt, j;
	char *pRes = NULL;
	if (m_nUseCnt == m_nAllocCount) goto NEW_ALLOC;

	for (j = 0; j < m_nAllocCount; j++) {
		if (i >= m_nAllocCount) { i = 0; }
		if (!isBitSet_flag(i, m_nFlagArray)) {
			if (pKey) {
				*pKey = i;
			}
			m_nUseCnt++;
			bitSet_flag(i, m_nFlagArray);
			pRes = &m_pChunk[i << m_nShift];
			memset(pRes, 0, g_nSizeList[m_nShift]);
			return pRes;
		}
	}

NEW_ALLOC:
	if (pKey) {
		*pKey = -1;
	}
	return (char *)calloc(1, g_nSizeList[m_nShift]);
}



bool CSBufPool::delMem(char *pUsedMem)	// random push
{
	if (!ISINCLUDE_ADDRESS(pUsedMem)) {
		free(pUsedMem);
		return false;
	}
	int nPos = (int)(pUsedMem - m_pChunk);

	if (pUsedMem == &m_pChunk[nPos]) {
		bitClear_flag(nPos >> m_nShift, m_nFlagArray);
		m_nUseCnt--;
		return true;
	}
	return false;
}


bool CSBufPool::delMemByIdx(char *pUsedMem, int nIdx)
{
	if (!ISINCLUDE(nIdx, m_nAllocCount)) {
		free(pUsedMem);
		return false;
	}

	if (pUsedMem == &m_pChunk[nIdx << m_nShift]) {
		bitClear_flag(nIdx, m_nFlagArray);
		m_nUseCnt--;
		return true;
	}
	return false;
}



bool CSBufPool::newMemList(int nCount)
{
	int i;
	int idx = m_nUseCnt, nLastLoopCnt=0;

	if (m_nUseCnt == m_nAllocCount) return false;

	for (i = 0; i < nCount; i++) {
		if (idx >= m_nAllocCount) break;
		if (!isBitSet_flag(idx, m_nFlagArray))
		{
			nLastLoopCnt++;
		}
		else break;
		idx++;
	}

	if (nLastLoopCnt == nCount) {
		m_nUseCnt += nCount;
		return true;
	}

	for (idx = m_nUseCnt; idx < nLastLoopCnt; idx++) {
		if (isBitSet_flag(idx, m_nFlagArray))
		{
			bitClear_flag(idx, m_nFlagArray);
		}
	}
	return false;
}


bool CSBufPool::delMem(char *pUsedMem, int nCount)
{
	int nPos = (int)(pUsedMem - m_pChunk);
	int nIdx = nPos >> m_nShift;
	int i;

	if (pUsedMem != &m_pChunk[nPos]) return false;
	for (i = 0; i < nCount; i++)
	{
		if (nIdx >= m_nAllocCount) return false;
		//printf("CBufPool::delMem nPos[%d] nIdx[%d] nShift[%d] pMem[%p]\n", nPos, nIdx, nShift, pUsedMem);
		if (isBitSet_flag(nIdx, m_nFlagArray))
		{
			bitClear_flag(nIdx, m_nFlagArray);
			m_nUseCnt--;
		}
		nIdx++;
	}
	return true;
}

