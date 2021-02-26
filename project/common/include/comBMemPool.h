/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
	2020.05.13 by KEH
	-------------------------------------------------------------
	FOR SINGLE THREAD! , not thread-safety

*********************************************************************/
#pragma once


#include "comTypes.h"
#define LEN_MEM_MESSAGE		128



template <typename MEMTYPE>
class CBMemPool 
{
public:

    // 메모리 생성
	CBMemPool(E_ALLOC_TYPE nType= eAlloc_Type_new);
	~CBMemPool(void);							// 객체가 소멸될때 malloc 해 놓은 chunk 메모리 자동 free
	inline void clear() { int i; for (i = 0; i < m_nAllocCount; i++) { m_nFlagArray[i] = 0; }m_nUseCnt = 0; }
	inline int alloc(int nMaxCount) { m_nAllocCount = __alloc(nMaxCount); return m_nAllocCount; }

	MEMTYPE *	newMem();
	MEMTYPE *	newMem(int *pKey);
	bool		delMem(MEMTYPE *);	
	bool		delMemByIdx(MEMTYPE *pUsedMem, int nKey);

	int newMemList(int nCount);
	bool delMem(MEMTYPE *p, int nCount);

    // 메모리 direct 접근
	inline void setBSlot(bool b) { m_bSlot = b; }
    inline MEMTYPE * getMem(int i)  { return &m_pChunk[i];}  // 안전성 보장된 상태에서 일괄처리 접근시 사용
	inline MEMTYPE * getUseMem(int nKey) {if(ISINCLUDE(nKey, m_nAllocCount)){ if (m_nFlagArray[nKey]) return &m_pChunk[nKey];} return NULL; }
	inline MEMTYPE* getNext(int *pIdx) {
		for (*pIdx; *pIdx < m_nAllocCount; (*pIdx)++) {
			if (m_nFlagArray[*pIdx]) { return &m_pChunk[*pIdx]; }
		}
		return NULL; 
	}

    // memory pool 사용현황 및 기타    
    inline int	GETUSECOUNT()		{ return m_nUseCnt; }	
	inline int	GETREMAINCOUNT()    { return m_nAllocCount-m_nUseCnt; }	
	inline int  GETMAXCOUNT()	    { return m_nAllocCount; }
	inline TCHAR * getMessage()	    { return g_szMessage;   }
	inline E_ALLOC_TYPE getAllocType() { return m_nObjAllocType; }
private:
	bool m_bSlot;
    int m_nUseCnt;
	int m_nAllocCount;
	E_ALLOC_TYPE m_nObjAllocType;
	char *m_nFlagArray;

    MEMTYPE * m_pChunk;
	MEMTYPE * __unit_alloc();
	int __alloc(int nMaxCount);

};

template <typename MEMTYPE>
CBMemPool<MEMTYPE>::CBMemPool(E_ALLOC_TYPE nType)
{
	m_nAllocCount = 0;
	m_pChunk = NULL;
	m_nUseCnt = 0;
	m_nFlagArray = NULL;
	m_nObjAllocType = nType;
	m_bSlot = false;
}

template <typename MEMTYPE>
CBMemPool<MEMTYPE>::~CBMemPool(void)
{
	if (m_pChunk) {
		switch (m_nObjAllocType) {
		case eAlloc_Type_new:		
		case eAlloc_Type_newArray:	delete[] m_pChunk;	break;
		case eAlloc_Type_alloc:		free(m_pChunk);		break;
		}

		m_pChunk = NULL;
	}

	if (m_nFlagArray) {
		free(m_nFlagArray);
		m_nFlagArray = NULL;
	}
}

template <typename MEMTYPE> 
int CBMemPool<MEMTYPE>::__alloc(int nMaxCount)
{	
	MEMTYPE* pPool;
	switch (m_nObjAllocType) {
	case eAlloc_Type_new		: 
	case eAlloc_Type_newArray	:	pPool = new (std::nothrow) MEMTYPE[nMaxCount]; break;
	case eAlloc_Type_alloc		:	pPool = (MEMTYPE *)calloc(nMaxCount, sizeof(MEMTYPE)); break;
	default: pPool=NULL;
	}	
	if (!pPool) {
		_stprintf(g_szMessage, _T("CBMemPool::pPool alloc ERROR"));
		return 0;
	}

	char* pFlag = (char*)calloc(nMaxCount, sizeof(char));
	if (!pFlag) {
		free(pPool);
		_stprintf(g_szMessage, _T("CBMemPool::pFlag alloc ERROR"));
		return 0;
	}
	memset(pFlag, 0, sizeof(pFlag));

	_stprintf(g_szMessage, _T("CBMemPool __alloc m_nAllocCount[%d]"), nMaxCount);
	m_pChunk = pPool;
	m_nFlagArray = pFlag;
	return nMaxCount;
}


template <typename MEMTYPE>
MEMTYPE * CBMemPool<MEMTYPE>::__unit_alloc()
{
	switch (m_nObjAllocType) {
	case eAlloc_Type_new		:	
	case eAlloc_Type_newArray	:	return new (std::nothrow) MEMTYPE;
	case eAlloc_Type_alloc		:	return (MEMTYPE *)calloc(1, sizeof(MEMTYPE));
	default: return NULL;
	}
}

template <typename MEMTYPE>
MEMTYPE * CBMemPool<MEMTYPE>::newMem()        // random pop
{
	int i = m_nUseCnt, j;
	if (m_nUseCnt == m_nAllocCount) {
		if (m_bSlot) return NULL;
		return __unit_alloc();
	}
	for (j = 0; j < m_nAllocCount; j++) {
		if (i >= m_nAllocCount) { i = 0; }
		if (!m_nFlagArray[i]) {
			m_nFlagArray[i] = 1;
			memset(&m_pChunk[i], 0, sizeof(MEMTYPE));
			m_nUseCnt++;
			return &m_pChunk[i];
		}
	}
	if (m_bSlot) return NULL;
	return __unit_alloc();
}

template <typename MEMTYPE> 
MEMTYPE * CBMemPool<MEMTYPE>::newMem(int *pKey)        // random pop
{
	int i= m_nUseCnt,j;

	if (m_nUseCnt == m_nAllocCount) {
		if (pKey) *pKey = -1;
		return NULL;
	}
	
	for (j = 0; j < m_nAllocCount; j++) {
		if (i >= m_nAllocCount) { i = 0; }
		if (!m_nFlagArray[i]) {
			if (pKey) {
				*pKey = i;
			}
			m_nFlagArray[i] = 1;
			memset(&m_pChunk[i], 0, sizeof(MEMTYPE));
			m_nUseCnt++;
			return &m_pChunk[i];
		}
	}

	return NULL;
}


template <typename MEMTYPE> 
bool CBMemPool<MEMTYPE>::delMem(MEMTYPE *pUsedMem)	// random push
{
	int nIdx = int(pUsedMem - m_pChunk);

	if (ISINCLUDE(nIdx, m_nAllocCount)) {
		m_nFlagArray[nIdx] = 0;
		m_nUseCnt--;
		return true;
	}
	if (m_bSlot) return false;
	switch (m_nObjAllocType) {
	case eAlloc_Type_new:	
	case eAlloc_Type_newArray:	delete pUsedMem; break;
	case eAlloc_Type_alloc:	free(pUsedMem); break;
	}
	return false;
}

template <typename MEMTYPE>
bool CBMemPool<MEMTYPE>::delMemByIdx(MEMTYPE *pUsedMem, int nIdx)
{
	if (ISINCLUDE(nIdx, m_nAllocCount)) {
		m_nFlagArray[nIdx] = 0;
		m_nUseCnt--;
		return true;
	}
	if (m_bSlot) return false;
	switch (m_nObjAllocType) {
	case eAlloc_Type_new:
	case eAlloc_Type_newArray:	delete pUsedMem; break;
	case eAlloc_Type_alloc:	free(pUsedMem); break;
	}
	return false;
}


template <typename MEMTYPE>
int CBMemPool<MEMTYPE>::newMemList(int nCount)
{
	int i, idx = m_nUseCnt, naIdx = 0;

	int nLastLoopCnt = 0;
	if (m_nUseCnt == m_nAllocCount) return NULL;

	for (i = 0; i < nCount; i++) {
		if (idx >= m_nAllocCount) break;
		if (!m_nFlagArray[idx])
		{
			m_nFlagArray[idx] = 1;
			nLastLoopCnt++;
		}
		else break;
		idx++;
	}

	if (nLastLoopCnt == nCount) {
		m_nUseCnt += nCount;
		return idx - nCount;
	}

	for (i = m_nUseCnt; i < nLastLoopCnt; i++) {
		if (m_nFlagArray[i])
		{
			m_nFlagArray[i] = 0;
		}
	}
	return -1;

}

template <typename MEMTYPE>
bool CBMemPool<MEMTYPE>::delMem(MEMTYPE *pUsedMem, int nCount)
{
	int nIdx = pUsedMem - m_pChunk;
	int i;

	if (pUsedMem != &m_pChunk[nIdx]) return false;
	for (i = 0; i < nCount; i++)
	{
		if (nIdx >= m_nAllocCount) return false;
		if (m_nFlagArray[nIdx])
		{
			m_nFlagArray[nIdx]=0;
			m_nUseCnt--;
		}
		nIdx++;
	}
	m_nLastCount = nIdx - nCount;
}

//###################################################

template <typename MEMTYPE>
class CBPoolSlot
{
public:
	CBPoolSlot(E_SLOT_UNIT unit= SLOT_POOL_UNIT);
	~CBPoolSlot();							
	// 한번에 메모리를 잡아놓는다.
	inline int alloc(int nMaxCount, E_ALLOC_TYPE nType = eAlloc_Type_new) {	return __alloc(nMaxCount, nType); }

	MEMTYPE *	newMem();
	bool		delMem(MEMTYPE *);

	int newMemList(int nCount, int *pSIdx);
	inline bool delMem(int nSIdx, MEMTYPE *p, int nCount) { return  m_pList[nSIdx]->delMem(p,nCount); }

	// 메모리 direct 접근
	inline MEMTYPE * getMem(int nSlot, int i) { return m_pList[nSlot]->getMem(i); }  // 안전성 보장된 상태에서 일괄처리 접근시 사용
	inline MEMTYPE* getSlotNext(int *pSlot, int *pIdx);
	inline MEMTYPE* getUseMem(int nSlot, int i) { return m_pList[nSlot]->getUseMem(i); }

	// memory pool 사용현황 및 기타    
	inline int  GETMAXCOUNT() { return m_nMax * m_pList[0]->GETMAXCOUNT(); }
	inline int  GETPOOLMAXCOUNT() { return m_pList[0]->GETMAXCOUNT(); }
	inline int  GETSLOTLASTCOUNT() { return m_nLast; }
	inline int  GETSLOTMAXCOUNT() { return m_nMax; }
	inline TCHAR * getMessage() { return g_szMessage; }
private:
	int m_nMax;		// slot max
	int m_nLast;	// slot last
	int m_nCurIdx;	// slot idx for add
	int m_nAddUnit;
	CBMemPool<MEMTYPE> ** m_pList;
	inline int realloc() { return __alloc(m_pList[0]->GETMAXCOUNT(), m_pList[0]->getAllocType());	}
	int __alloc(int nMaxCount, E_ALLOC_TYPE nType);
	//inline void getIdx(int nIdx, int *pSlot, int *pIdx) { *pSlot = div_u32_rem(nIdx, m_pList[0]->GETMAXCOUNT(), *pIdx);  *pIdx--; }
};

template <typename MEMTYPE>
CBPoolSlot<MEMTYPE>::CBPoolSlot(E_SLOT_UNIT unit)
{
	m_nMax = 0;
	m_nLast = 0;
	m_nCurIdx = 0;
	m_pList = NULL;
	m_nAddUnit = unit;
}

template <typename MEMTYPE>
CBPoolSlot<MEMTYPE>::~CBPoolSlot()
{
	if (!m_pList) return;

	int i;
	for (i = 0; i < m_nLast; i++) {
		delete m_pList[i];
	}
	free(m_pList);
	m_pList = NULL;
}

template <typename MEMTYPE>
int CBPoolSlot<MEMTYPE>::__alloc(int nMaxCount, E_ALLOC_TYPE nType)
{
	CBMemPool<MEMTYPE> * pPool = new CBMemPool<MEMTYPE>(nType);
	if (!pPool) {
		_stprintf(g_szMessage, _T("CBPoolSlot:: new CBMemPool<MEMTYPE> ERROR"));
		return 0;
	}
	if (!pPool->alloc(nMaxCount)) {
		delete pPool;
		_stprintf(g_szMessage, _T("CBPoolSlot:: pPool->alloc ERROR"));
		return 0;
	}
	pPool->setBSlot(true);
	if (m_nMax == m_nLast) { // slot list alloc
		CBMemPool<MEMTYPE> **pList = (CBMemPool<MEMTYPE> **)calloc(m_nMax + SLOT_LIST_UNIT, sizeof(void *));
		if (!pList) {
			delete pPool;
			_stprintf(g_szMessage, _T("CBPoolSlot:: pList calloc ERROR"));
			return 0;
		}
		if (m_pList) {
			memcpy(pList, m_pList, sizeof(void *)*m_nMax);
			free(m_pList);
		}
		m_pList = pList;
		m_nMax += SLOT_LIST_UNIT;
	}
	m_pList[m_nLast] = pPool;
	m_nLast++;
	return m_nMax;
}

template <typename MEMTYPE>
MEMTYPE* CBPoolSlot<MEMTYPE>::getSlotNext(int *pSlot, int *pIdx) {
	MEMTYPE *p;
	for (*pSlot; *pSlot < m_nLast; (*pSlot)++)
	{
		p = m_pList[*pSlot]->getNext(pIdx);
		if (p) return p;
		*pIdx = 0;
	}
	return NULL;
}

template <typename MEMTYPE>
MEMTYPE * CBPoolSlot<MEMTYPE>::newMem()        // random pop
{
	int i;
	MEMTYPE *p = m_pList[m_nCurIdx]->newMem();
	if (p) return p;

	for (i=0; i < m_nLast; i++)
	{
		p = m_pList[i]->newMem();
		if (p) { m_nCurIdx = i;  return p; }
	}

	if(realloc()) return m_pList[i]->newMem();
	return NULL;
}

template <typename MEMTYPE>
bool CBPoolSlot<MEMTYPE>::delMem(MEMTYPE *p)	// random push
{
	int i;
	bool bRes;
	for (i = 0; i < m_nLast; i++)
	{
		bRes = m_pList[i]->delMem(p);
		if (bRes) { m_nCurIdx = i;  return true; }
	}
	return false;
}

template <typename MEMTYPE>
int CBPoolSlot<MEMTYPE>::newMemList(int nCount, int *pSIdx)
{
	int i, nIdx;
	nIdx = m_pList[m_nCurIdx]->newMem(nCount);
	if (-1 < nIdx) { *pSIdx = m_nCurIdx; return nIdx; }

	for (i = 0; i < m_nLast; i++)
	{
		nIdx = m_pList[i]->newMem(nCount);
		if (0 > *idx) continue;
		*pSIdx = i; return nIdx;
	}

	if (realloc()) {
		nIdx = m_pList[i]->newMem(nCount);
		if (-1 < nIdx) { *pSIdx = m_nCurIdx; return nIdx; }
	}
	*pSIdx = -1;
	return -1;
}

