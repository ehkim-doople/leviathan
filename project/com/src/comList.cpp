//#include "comList.h"
#include "comLogger.h"
#define UNIT_ADD_SIZE 10

CSList::CSList()
{
	m_pArray = NULL;
	m_nUse = 0;
	m_nLast = 0;
	m_nDelIdx = 0;
	m_nMax = 0;
	m_nObjAllocType = eAlloc_Type_none;
}

CSList::~CSList()
{
	if (!m_pArray) return;

	if (ISABLETODELETE(m_nObjAllocType)) {
		int idx = 0;
		void *p = getNext(&idx);
		while (p) {
			idx++;
			switch (m_nObjAllocType) {
			case eAlloc_Type_new:	delete p; break;
			case eAlloc_Type_alloc:	free(p); break;
			case eAlloc_Type_BufPool:	gs_pMMgr->delBuf((char *)p, STRING_SIZE((TCHAR *)p)); break;
			case eAlloc_Type_newArray:	delete[] p; break;
			case eAlloc_Type_none: break;
			case eAlloc_Type_MemPool: break;
			}
			p = getNext(&idx);
		}
	}

	free(m_pArray);
	m_pArray = 0;
}

void CSList::clear()			// list 는 남겨 놓고 내부 메모리는 가능한 부분은 파괴한다.
{
	if (ISABLETODELETE(m_nObjAllocType)) {
		int idx = 0;
		void *p = getNext(&idx);
		while (p) {
			idx++;
			switch (m_nObjAllocType) {
			case eAlloc_Type_new:	delete p; break;
			case eAlloc_Type_alloc:	free(p); break;
			case eAlloc_Type_BufPool:	gs_pMMgr->delBuf((char *)p, STRING_SIZE((TCHAR *)p)); break;
			case eAlloc_Type_newArray:	delete[] p; break;
			case eAlloc_Type_none: break;
			case eAlloc_Type_MemPool: break;
			}
			p = getNext(&idx);
		}
	}
	memset(m_pArray, 0, sizeof(void *) * m_nMax);
	m_nUse = 0;
	m_nLast = 0;
	m_nDelIdx = 0;
}


int CSList::__alloc(int nMaxCount)
{
	if (nMaxCount <= m_nMax) return m_nMax;
	void** pList = (void**)calloc(nMaxCount, sizeof(void*));
	if (!pList) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("m_nMaxCount[%d]"), nMaxCount);
		return 0;
	}
	//_stprintf(g_szMessage, _T("CList::__alloc list m_nMaxCount[%d]"), nMaxCount);
	if (m_pArray) {
		memcpy(pList, m_pArray, m_nMax * sizeof(void*));
		free(m_pArray);
	}
	m_pArray = pList;
	return nMaxCount;
}

bool CSList::add(void* Obj, int *pIdx)
{
	int nIdx, i;

	if (m_nUse >= m_nMax) {
		if (!__alloc(m_nMax + UNIT_ADD_SIZE)) {
			return false;
		}
		nIdx = m_nMax;
		m_nMax += UNIT_ADD_SIZE;
		goto ADD_SUCCESS;
	}

	if (!m_pArray[m_nDelIdx]) { nIdx = m_nDelIdx; goto ADD_SUCCESS; }

	nIdx = m_nLast;
	for (i = 0; i < m_nMax; i++)
	{
		if (nIdx >= m_nMax) nIdx = 0;
		if (!m_pArray[nIdx]) {
			goto ADD_SUCCESS;
		}
		nIdx++;
	}
	gs_cLogger.DebugLog(LEVEL_ERROR, _T("i[%d] m_nLast[%d] m_nCurCount[%d]"), nIdx, m_nLast, m_nUse);
	return false;

ADD_SUCCESS:
	m_nUse++;
	m_pArray[nIdx] = Obj;
	m_nLast++;
	//_stprintf(g_szMessage, _T("CMemList::add i[%d] m_nLast[%d] m_nCurCount[%d]"), nIdx, m_nLast, m_nUse);
	if (pIdx) *pIdx = nIdx;
	return true;
}

bool CSList::push_back(void*Obj, int *pIdx)
{
	int nIdx, i;

	if (m_nUse >= m_nMax) {
		if (!__alloc(m_nMax + UNIT_ADD_SIZE)) return false;
		nIdx = m_nMax;
		m_nMax += UNIT_ADD_SIZE;
		goto ADD_SUCCESS;
	}

	if (!m_pArray[m_nLast]) { nIdx = m_nLast;  goto ADD_SUCCESS; }

	nIdx = m_nLast;
	for (i = 0; i < m_nMax; i++)
	{
		if (nIdx >= m_nMax) nIdx = 0;
		if (!m_pArray[nIdx]) {
			goto ADD_SUCCESS;
		}
		nIdx++;
	}
	gs_cLogger.DebugLog(LEVEL_ERROR, " i[%d] m_nLast[%d] m_nCurCount[%d]", nIdx, m_nLast, m_nUse);
	return false;

ADD_SUCCESS:
	m_nUse++;
	m_pArray[nIdx] = Obj;
	m_nLast++;
	//_stprintf(g_szMessage, _T("CMemList::add i[%d] m_nLast[%d] m_nCurCount[%d]"), nIdx, m_nLast, m_nUse);
	if (pIdx) *pIdx = nIdx;
	return true;
}


bool CSList::del(void*p, bool bDeepDelete)
{
	int i;
	for (i = 0; i < m_nMax; i++) {
		if (m_pArray[i] == p) {
			if (bDeepDelete) {
				switch (m_nObjAllocType) {
				case eAlloc_Type_new:	delete p; break;
				case eAlloc_Type_alloc:	free(p); break;
				case eAlloc_Type_BufPool:	gs_pMMgr->delBuf((char *)p, STRING_SIZE((TCHAR *)p)); break;
				case eAlloc_Type_newArray:	delete[] p; break;
				case eAlloc_Type_none: break;
				case eAlloc_Type_MemPool: break;
				}
			}
			_DEL(i);
//			_stprintf(g_szMessage, _T("CMemList::del i[%d] m_nLast[%d] m_nCurCount[%d]"), i, m_nLast, m_nUse);
			return true;
		}
	}
	gs_cLogger.DebugLog(LEVEL_ERROR, " del FAIL! i[%d] m_nMax[%d]", i, m_nMax);
	return false;
}

bool CSList::del(int i, bool bDeepDelete)
{
	if (!ISINCLUDE(i, m_nMax)) {
		gs_cLogger.DebugLog(LEVEL_ERROR, "i[%d] m_nMax[%d]", i, m_nMax);
		return false;
	}
	if (bDeepDelete) {
		switch (m_nObjAllocType) {
		case eAlloc_Type_new:	delete m_pArray[i]; break;
		case eAlloc_Type_alloc:	free(m_pArray[i]); break;
		case eAlloc_Type_BufPool:	gs_pMMgr->delBuf((char *)m_pArray[i], STRING_SIZE((TCHAR *)m_pArray[i])); break;
		case eAlloc_Type_newArray:	delete[] m_pArray[i]; break;
		case eAlloc_Type_none: break;
		case eAlloc_Type_MemPool: break;
		}
	}
	_DEL(i);
//	_stprintf(g_szMessage, _T("CMemList::del i[%d] m_nLast[%d] m_nCurCount[%d]"), i, m_nLast, m_nUse);
	return true;
}