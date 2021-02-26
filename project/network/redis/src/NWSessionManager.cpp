/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/
//#include "comTypes.h"
#include "NWSessionManager.h"
#include "comLogger.h"


#define UNIT_ADD_SIZE 36

CRedisList::CRedisList()
{
	m_pArgv = NULL;
	m_pArgvLen = NULL;
	m_nUse = 0;
	m_nLast = 0;
	m_nMax = 0;
	m_nObjAllocType = eAlloc_Type_none;
	m_pRedisFullContents = NULL;
	m_nContentsIdx = 0;
}

CRedisList::~CRedisList()
{
	clear();

	if (m_pArgv)	free(m_pArgv);
	m_pArgv = 0;
	if (m_pArgvLen)	free(m_pArgvLen);
	m_pArgvLen = 0;

}

void CRedisList::clear()			// list 는 남겨 놓고 내부 메모리는 가능한 부분은 파괴한다.
{
	if(m_nUse) listClear();
	if (m_pRedisFullContents) {
		gs_pMMgr->delBufByIndex(m_pRedisFullContents, m_nContentsIdx);
		m_pRedisFullContents = NULL;
		m_nContentsIdx = 0;
	}
}

void CRedisList::listClear()
{
	int nLen;
	int idx = 0;
	if (ISABLETODELETE(m_nObjAllocType)) {
		void *p = getObj(idx);
		while (p) {
			nLen = (int)(_tcslen((TCHAR *)p)) + 1;
			gs_pMMgr->delBuf((char *)p, nLen);
			p = getObj(idx);
			idx++;
		}
	}
	memset(m_pArgv, 0, sizeof(char *) * m_nMax);
	memset(m_pArgvLen, 0, sizeof(size_t) * m_nMax);
	m_nUse = 0;
	m_nLast = 0;
}


int CRedisList::__alloc(int nMaxCount)
{
	if (nMaxCount <= m_nMax) return m_nMax;

	size_t *pSizeList;

	char** pList = (char**)calloc(nMaxCount, sizeof(char*));
	if (!pList) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("calloc pList has FAILED! m_nMaxCount[%d]"), nMaxCount);
		return 0;
	}
	pSizeList = (size_t *)calloc(nMaxCount, sizeof(size_t));
	if (!pSizeList) {
		free(pList);
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("calloc pSizeList has FAILED! m_nMaxCount[%d]"), nMaxCount);
		return 0;
	}
	if (m_pArgv) {
		memcpy(pList, m_pArgv, m_nMax * sizeof(char*));
		free(m_pArgv);
	}
	m_pArgv = pList;

	if (m_pArgvLen) {
		memcpy(pSizeList, m_pArgvLen, m_nMax * sizeof(size_t));
		free(m_pArgvLen);
	}
	m_pArgvLen = pSizeList;

	return nMaxCount;
}

bool CRedisList::add(char* Obj, int nLen, int *pIdx)
{
	int nIdx, i, nMax;

	if (m_nUse >= m_nMax) {
		nMax = m_nMax << 1;
		nMax = __alloc(nMax);
		if (!nMax) {
			return false;
		}
		nIdx = m_nMax;
		m_nMax = nMax;
		goto ADD_SUCCESS;
	}

	if (!m_pArgv[m_nLast]) { nIdx = m_nLast;  goto ADD_SUCCESS; }

	nIdx = m_nLast + 1;
	for (i = 0; i < m_nMax; i++)
	{
		if (nIdx >= m_nMax) nIdx = 0;
		if (!m_pArgv[nIdx]) {
			goto ADD_SUCCESS;
		}
		nIdx++;
	}
	gs_cLogger.DebugLog(LEVEL_ERROR, _T("i[%d] m_nLast[%d] m_nCurCount[%d]"), nIdx, m_nLast, m_nUse);
	return false;

ADD_SUCCESS:
	m_nUse++;
	m_pArgv[nIdx] = Obj;
	m_pArgvLen[nIdx] = nLen;
	m_nLast++;
	//_stprintf(g_szMessage, _T("CMemList::add i[%d] m_nLast[%d] m_nCurCount[%d]"), nIdx, m_nLast, m_nUse);
	if (pIdx) *pIdx = nIdx;
	return true;
}

bool CRedisList::new_push_back(char *p, int nLen, int *pIdx)
{
	if (!nLen) nLen = (int)strlen(p);
	char *pNew = gs_pMMgr->newBuf(nLen + 1);
	strcpy(pNew, p);
	return push_back(pNew, nLen, pIdx);
}

bool CRedisList::push_back(char *Obj, int nLen, int *pIdx)
{
	int nIdx, i, nMax;

	if (m_nUse >= m_nMax) {
		nMax = m_nMax << 1;
		nMax = __alloc(nMax);
		if (!nMax) {
			gs_cLogger.DebugLog(LEVEL_ERROR, " nMax[%d]", nMax);
			return false;
		}
		nIdx = m_nMax;
		m_nMax = nMax;
		goto ADD_SUCCESS;
	}
	if (!m_pArgv[m_nLast]) { nIdx = m_nLast;  goto ADD_SUCCESS; }

	nIdx = m_nLast+1;
	for (i = 0; i < m_nMax; i++)
	{
		if (nIdx >= m_nMax) nIdx = 0;
		if (!m_pArgv[nIdx]) {
			goto ADD_SUCCESS;
		}
		nIdx++;
	}
	gs_cLogger.DebugLog(LEVEL_ERROR, " i[%d] m_nLast[%d] m_nCurCount[%d]", nIdx, m_nLast, m_nUse);
	return false;

ADD_SUCCESS:
	m_nUse++;
	m_pArgv[nIdx] = Obj;
	m_pArgvLen[nIdx] = nLen;
	m_nLast++;
	//_stprintf(g_szMessage, _T("CMemList::add i[%d] m_nLast[%d] m_nCurCount[%d]"), nIdx, m_nLast, m_nUse);
	if (pIdx) *pIdx = nIdx;
	m_nContentsIdx += nLen;
	return true;
}

int CRedisList::getRedisContents(STBuf *p)
{
	int i, nSize, nPos;
	if (m_pRedisFullContents) {
		gs_cLogger.DebugLog(LEVEL_ERROR, " m_pRedisFullContents is already exist! m_nContentsIdx[%d]", m_nContentsIdx);
		gs_pMMgr->delBufByIndex(m_pRedisFullContents, m_nContentsIdx);
		m_pRedisFullContents = NULL;
		m_nContentsIdx = 0;
	}

	nSize = m_nContentsIdx + m_nUse;
	nSize += 128;

	m_pRedisFullContents = gs_pMMgr->newBuf(nSize, &m_nContentsIdx);
	if (!m_pRedisFullContents) {
		gs_cLogger.DebugLog(LEVEL_ERROR, " m_pRedisFullContents is NULL! nSize[%d] = m_nUse[%d] * 128", nSize, m_nUse);
		return 0;
	}
	// TODO make
	nPos = 0;
	for (i = 0; i < m_nLast; i++) {
		nPos += sprintf(m_pRedisFullContents+nPos, "%s ", m_pArgv[i]);
		if (m_nUse == i) break;
		else if (nPos >= nSize) {
			gs_cLogger.DebugLog(LEVEL_ERROR, " m_pRedisFullContents Size Error! nPos[%d] nSize[%d] = m_nUse[%d] * 128", nPos, nSize, m_nUse);
			break;
		}
	}
	m_pRedisFullContents[nPos] = 0;
	p->pValue = m_pRedisFullContents;
	p->nIdx = m_nContentsIdx;
	nSize = getSizeByIndex(m_nContentsIdx);
	return nSize;
}


//int g_nRecvBufSize;
CSessionManager * g_pSessMgr;
CSessionManager::CSessionManager()
{
	m_pList = NULL;
}


CSessionManager::~CSessionManager(void)
{	
	if (m_pList) delete m_pList;
}

bool CSessionManager::createMemoryPool(int nListCnt, int nRedisListCnt)
{
	CRedisList *pRList;
	m_pList = new  (std::nothrow) CMemList<CRedisList>();
	if (!m_pList->alloc(nListCnt + UNIT_ADD_SIZE, eAlloc_Type_new)) {
		return false;
	}
	int i;
	for (i = 0; i < nListCnt; i++) {
		pRList = new  (std::nothrow) CRedisList();
		if (!pRList) {
			gs_cLogger.DebugLog(LEVEL_ERROR, "new CRedisList is NULL");
			return NULL;
		}
		pRList->alloc(nRedisListCnt);
		m_pList->setObj(i, pRList);
	}
	return true;
}

CRedisList * CSessionManager::getRedisList()
{
	CRedisList *pRes = (CRedisList *)m_pList->pop();
	if (!pRes) {
		pRes = new  (std::nothrow) CRedisList();
		if (!pRes) {
			gs_cLogger.DebugLog(LEVEL_ERROR, "new CRedisList is NULL");
			return NULL;
		}
		pRes->alloc(1024);
	}
	//printf("getRedisList POP [cur/tot:%d/%d]\n", m_pList->size(), m_pList->capacity());
	return pRes;
}


void CSessionManager::returnRedisList(CRedisList *pList)
{
	pList->clear();
	if (m_pList->add(pList) < 0) {
		delete pList;
		gs_cLogger.DebugLog(LEVEL_ERROR, "m_pList->add has failed [cur/tot:%d/%d]", m_pList->size(), m_pList->capacity());
	}
}