/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/
/********************************************************************
2012.09.17 by KEH
-------------------------------------------------------------
@ function
- thread safty list
- no sequntial

다중 add, 다중 pop
*********************************************************************/
#pragma once
#include "comLogger.h"

//#define LEN_MEM_MESSAGE		128

template <typename T>
class CMemList
{
public:

	CMemList();
	~CMemList(void);							// 객체가 소멸될때 malloc 해 놓은 chunk 메모리 자동 free
	int alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_none);	// 포인터의 객체가 어떤 타입의 메모리 할당을 받은 객체인지 미리 지정 (deep 삭제시 필요)

	T* getNext(int *pCurIdx);
	int add(T*);
	T* pop();
	bool del(T*);
	void del(int i) { if (ISINCLUDE(i, m_nMaxCount)) _del(i); }
	bool isObj(T*);
	int getObjIdx(const T*);
	bool setObj(int idx, const T* p);
	bool setObjChange(int idx, const T* p);
	void clear();

	inline int  size() { return m_nCurCount.getCount(); }
	inline int  capacity() { return m_nMaxCount; }
	inline int  lastIdx() { return m_nLastCount.getCount(); }
	inline void copyFlagArray(atomic_nr *pArray, int nCnt) { memcpy(pArray, m_nFlagArray, nCnt * sizeof(atomic_nr)); }
	inline T* getObject(int i) { return m_pArray[i]; }
	inline void setObjAllocType(E_ALLOC_TYPE type) { m_nObjAllocType = type; }
private:
	T **    m_pArray;
	atomic_nr *  m_nFlagArray;
	atomic_nr    m_nCurCount;
	int			 m_nEmptyFirstIdx;
	int			 m_nMaxCount;
	E_ALLOC_TYPE m_nObjAllocType;
	T* m_pEnd;
	//TCHAR m_szMessage[LEN_MEM_MESSAGE];

	void _del(int idx);
	inline bool ISINCLUDE_ADDRESS(T *p) { if (m_pArray[0] <= p && p <= m_pEnd) return true; return false; }
};


template <typename T>
CMemList<T>::CMemList()
{
	//memset(m_szMessage, 0, LEN_MEM_MESSAGE);
	m_pArray = NULL;
	m_nFlagArray = NULL;
	m_nObjAllocType = eAlloc_Type_none;
	m_nCurCount.init();
}

template <typename T>
CMemList<T>::~CMemList(void)
{
	if (!m_pArray) return;

	if (ISABLETODELETE(m_nObjAllocType)) {
		int idx = 0;
		T *p = getNext(&idx);
		while (p) {
			idx++;
			switch (m_nObjAllocType) {
			case eAlloc_Type_new		: delete p; break;
			case eAlloc_Type_alloc		: free(p); break;
			case eAlloc_Type_BufPool	: gs_pMMgr->delBuf((char *)p, STRING_SIZE((TCHAR *)p)); break;
			case eAlloc_Type_newArray	: delete[] p; break;
			case eAlloc_Type_none		: break;
			case eAlloc_Type_MemPool	: break;
			}
			p = getNext(&idx);
		}
	}

	if (m_pArray)        free(m_pArray);
	m_pArray = 0;

	if (m_nFlagArray)    delete[] m_nFlagArray;
	m_nFlagArray = 0;
}


template <typename T>
int CMemList<T>::alloc(int nMaxCount, E_ALLOC_TYPE type) {

	//T  ObjMem = 0;	

	m_pArray = (T **)calloc(nMaxCount, sizeof(T*));
	if (m_pArray == 0) {
		//_stprintf(m_szMessage, _T("CMemList::m_pArray allock ERROR"));
		return 0;
	}

	m_nFlagArray = new(std::nothrow)atomic_nr[nMaxCount];
	if (m_nFlagArray == 0) {
		//_stprintf(m_szMessage, _T("CMemList::m_nFlagArray allock ERROR"));
		free(m_pArray);
		return 0;
	}
	int i;
	for (i = 0; i < nMaxCount; i++)
	{
		m_nFlagArray[i].init();
	}

	m_nMaxCount = nMaxCount;
	m_nObjAllocType = type;
	m_pEnd = m_pArray[nMaxCount-1];
	//_stprintf(m_szMessage, _T("CMemList::alloc list m_nMaxCount[%d]"), m_nMaxCount);
	return 1;
}


template <typename T>
T*  CMemList<T>::getNext(int *pCurIdx)
{
	int i = *pCurIdx;
	if (i < m_nMaxCount)
	{
		if (m_nFlagArray[i].getCount() == 2) {
			*pCurIdx = i;
			return m_pArray[i];
		}
		else
		{
			int idx = i + 1;
			for (idx; idx < m_nMaxCount; idx++)
			{
				if (m_nFlagArray[idx].getCount() == 2) {
					*pCurIdx = idx;
					return m_pArray[idx];
				}
			}
		}
	}
	return NULL;
}

template <typename T>
bool CMemList<T>::isObj(T* Obj)
{
	int i;
	for (i = 0; i < m_nMaxCount; i++)
	{
		if (m_pArray[i] == Obj) return true;
	}

	return false;

}

template <typename T>
int CMemList<T>::getObjIdx(const T* Obj)
{
	int i;
	for (i = 0; i < m_nMaxCount; i++)
	{
		if (m_pArray[i] == Obj) return i;
	}

	return -1;
}


// 이미 Add 된 내용을 변경할때만 사용
// Add 하기전에 이 함수를 사용하면 안됨

template <typename T>
bool CMemList<T>::setObj(int idx, const T* p) {
	if (!ISINCLUDE(idx, m_nMaxCount)) return false;
	if (!m_nFlagArray[idx].getCount()) {
		m_nFlagArray[idx].setCount(1);
		m_pArray[idx] = p;
		return true;
	}
	return false;
}

template <typename T>
bool CMemList<T>::setObjChange(int idx, const T* p) {
	if (!ISINCLUDE(idx, m_nMaxCount)) return false;
	if (m_nFlagArray[idx].getCount() == 2) {
		m_pArray[idx] = p;
		return true;
	}
	return false;
}


template <typename T>
int CMemList<T>::add(T *Obj)
{
	int nAddIdx = m_nEmptyFirstIdx, i;
	bool bFind = false;

	if (0 <= nAddIdx) {
		if (!m_nFlagArray[nAddIdx].getCount()) {
			if (!m_nFlagArray[nAddIdx].atomic_compare_exchange(1, 0)) {
				m_nEmptyFirstIdx = -1;
				goto ADD_SUCCESS;
			}
		}
	}

	nAddIdx = m_nCurCount.getCount();
	for (i = 0; i < m_nMaxCount; i++) {
		if (m_nMaxCount <= m_nCurCount.getCount()) goto ADD_ERROR;
		if (nAddIdx == m_nMaxCount) nAddIdx = 0;
		if (!m_nFlagArray[nAddIdx].getCount()) {
			if (!m_nFlagArray[nAddIdx].atomic_compare_exchange(1, 0)) goto ADD_SUCCESS;
		}
		nAddIdx++;
	}

ADD_SUCCESS:
	m_pArray[nAddIdx] = Obj;
	m_nFlagArray[nAddIdx].setCount(2);
	m_nCurCount.atomic_increment();
	return nAddIdx;

ADD_ERROR :
	_stprintf(g_szMessage, _T("FULL! m_nMaxCount[%d]"), m_nMaxCount);
	comErrorPrint(g_szMessage);
	return -1;
}


template <typename T>
T* CMemList<T>::pop()
{
	T* pRes;
	int i;
	for (i = 0; i < m_nMaxCount; i++)
	{
		if (m_nFlagArray[i].getCount() == 2) {
			if (2 == m_nFlagArray[i].atomic_compare_exchange(1, 2)) {
				pRes = m_pArray[i];
				_del(i);
				return pRes;
			}
		}
	}
	return NULL;
}

template <typename T>
bool CMemList<T>::del(T *Obj)
{
	int i;
	for (i = 0; i < m_nMaxCount; i++)
	{
		if (m_pArray[i] == Obj) {
			_del(i);
			return true;
		}
	}
	comErrorPrint("can't find obj");
	return false;
}

template <typename T>
void CMemList<T>::_del(int i)
{
	m_nCurCount.atomic_decrement();
	m_nFlagArray[i].setCount(0);
	if(m_nEmptyFirstIdx < 0) m_nEmptyFirstIdx = i;
	else if (i < m_nEmptyFirstIdx) m_nEmptyFirstIdx = i;
}

template <typename T>
void CMemList<T>::clear()
{
	int i;
	m_nCurCount.init();
	m_nEmptyFirstIdx = 0;
	for (i = 0; i < m_nMaxCount; i++) {
		m_nFlagArray[i].init();
	}
	memset(m_pArray, 0, sizeof(m_pArray));
}



//###########################################################################################
/********************************************************************
2020.07.10 by KEH
-------------------------------------------------------------
@ function
- single thread
- no sequntial
- realloc

single push, single pop
*********************************************************************/
#define TLIST_ADD_SIZE 64

template <typename T>
class CTList
{
public:

	CTList();
	~CTList(void);			// 객체가 소멸될때 malloc 해 놓은 chunk 메모리 자동 free

							// 한번에 메모리를 잡아놓는다.
	int alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_new) { m_nObjAllocType = type;  m_nMax = __alloc(nMaxCount); return m_nMax; }
	bool add(T*p, int *pIdx = NULL);
	bool push_back(T*p, int *pIdx = NULL);
	bool IsExist(T*p);
	bool del(T*p);
	bool del(int i);
	inline T* getObj(int i) { if (ISINCLUDE(i, m_nMax)) return m_pArray[i]; return NULL; }
	inline bool isObj(T*p) { int i; for (i = 0; i < m_nMax; i++) { if (m_pArray[i] == p) { return true; } } return false; }
	inline T* getNext(int *pIdx) {
		for (*pIdx; *pIdx < m_nMax; (*pIdx)++) {
			if (m_pArray[*pIdx]) { return m_pArray[*pIdx]; }
		}
		return NULL;
	}

	// 이미 Add 된 내용을 변경할때만 사용
	inline bool setObj(int idx, T*p) { if (ISINCLUDE(idx, m_nMax)) { if (m_pArray[idx]) { if (!p) _DEL(idx); else m_pArray[idx] = p;	return true; } } return false; }
	// 특정 위치에 신규 Obj 추가
	inline bool setNewObj(int idx, T*p) { if (ISINCLUDE(idx, m_nMax)) { if (!m_pArray[idx]) { m_pArray[idx] = p; m_nUse++; m_nLast++; return true; } } return false; }

	//inline TCHAR * getMessage() { return g_szMessage; }
	inline int  size() { return m_nUse; }
	inline int  lastIdx() { return m_nLast; }
	inline int  capacity() { return m_nMax; }
	inline void setObjAllocType(E_ALLOC_TYPE type) { m_nObjAllocType = type; }

private:
	T**		m_pArray;
	int	m_nUse;
	int	m_nLast;
	int	m_nDelIdx;
	int	m_nMax;
	E_ALLOC_TYPE m_nObjAllocType;
	int __alloc(int nMaxCount);
	inline void _DEL(int i) { m_pArray[i] = 0; m_nUse--; m_nDelIdx = i; }
};

template <typename T>
CTList<T>::CTList()
{
	m_pArray = NULL;
	m_nUse = 0;
	m_nLast = 0;
	m_nDelIdx = 0;
	m_nMax = 0;
	m_nObjAllocType = eAlloc_Type_none;
}

template <typename T>
CTList<T>::~CTList(void)
{
	if (!m_pArray) return;

	if (ISABLETODELETE(m_nObjAllocType)) {
		int idx = 0;
		T *p = getNext(&idx);
		while (p) {
			idx++;
			switch (m_nObjAllocType) {
				case eAlloc_Type_new		:	delete p; break;
				case eAlloc_Type_alloc		:	free(p); break;
				case eAlloc_Type_BufPool	:	gs_pMMgr->delBuf((char *)p, STRING_SIZE((TCHAR *)p)); break;
				case eAlloc_Type_newArray	:	delete[] p; break;
				case eAlloc_Type_none		:	break;
				case eAlloc_Type_MemPool	:	break;
			}
			p = getNext(&idx);
		}
	}

	if (m_pArray)	free(m_pArray);
	m_pArray = 0;
}

template <typename T>
int CTList<T>::__alloc(int nMaxCount)
{
	T** pList = (T**)calloc(nMaxCount, sizeof(T*));
	if (!pList) {
		_stprintf(g_szMessage, "pList calloc is NULL");
		return 0;
	}
	//_stprintf(g_szMessage, _T("CTList::__alloc list m_nMaxCount[%d]"), nMaxCount);
	if (m_pArray) {
		memcpy(pList, m_pArray, m_nMax * sizeof(T*));
		free(m_pArray);
	}
	m_pArray = pList;
	return nMaxCount;
}


template <typename T>
bool CTList<T>::add(T* Obj, int *pIdx)
{
	int nIdx, i;

	if (m_nUse >= m_nMax) {
		if (!__alloc(m_nMax + TLIST_ADD_SIZE)) {
			return false;
		}
		nIdx = m_nMax;
		m_nMax += TLIST_ADD_SIZE;
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
	//_stprintf(m_szMessage, _T("add FAIL! m_nLast[%d] m_nCurCount[%d] m_nMax[%d]"), m_nLast, m_nUse, m_nMax);
	return false;

ADD_SUCCESS:
	m_nUse++;
	m_pArray[nIdx] = Obj;
	m_nLast++;
	//_stprintf(g_szMessage, _T("CMemList::add i[%d] m_nLast[%d] m_nCurCount[%d]"), nIdx, m_nLast, m_nUse);
	if (pIdx) *pIdx = nIdx;
	return true;
}

template <typename T>
bool CTList<T>::IsExist(T*p)
{
	int i;
	for (i = 0; i < m_nMax; i++)
	{
		if (m_pArray[i]) {
			if (m_pArray[i] == p) {
				return true;
			}
		}
	}
	return false;
}

template <typename T>
bool CTList<T>::push_back(T*Obj, int *pIdx)
{
	int nIdx, i;

	if (m_nUse >= m_nMax) {
		if (!__alloc(m_nMax + TLIST_ADD_SIZE)) return false;
		nIdx = m_nMax;
		m_nMax += TLIST_ADD_SIZE;
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
	//_stprintf(m_szMessage, _T("add FAIL! m_nLast[%d] m_nCurCount[%d] m_nMax[%d]"), m_nLast, m_nUse, m_nMax);
	return false;

ADD_SUCCESS:
	m_nUse++;
	m_pArray[nIdx] = Obj;
	m_nLast++;
	//_stprintf(g_szMessage, _T("CMemList::add i[%d] m_nLast[%d] m_nCurCount[%d]"), nIdx, m_nLast, m_nUse);
	if (pIdx) *pIdx = nIdx;
	return true;
}


template <typename T>
bool CTList<T>::del(T*p)
{
	int i;
	for (i = 0; i < m_nMax; i++) {
		if (m_pArray[i] == p) {
			_DEL(i);
			//_stprintf(g_szMessage, _T("CMemList::del i[%d] m_nLast[%d] m_nCurCount[%d]"), i, m_nLast, m_nUse);
			return true;
		}
	}
	_stprintf(g_szMessage, " del FAIL! i[%d] m_nMax[%d]", i, m_nMax);
	return false;
}

template <typename T>
bool CTList<T>::del(int i)
{
	if (!ISINCLUDE(i, m_nMax)) {
		_stprintf(g_szMessage, "i[%d] m_nMax[%d]", i, m_nMax);
		return false;
	}
	_DEL(i);
	//_stprintf(g_szMessage, _T("CMemList::del i[%d] m_nLast[%d] m_nCurCount[%d]"), i, m_nLast, m_nUse);
	return true;
}
