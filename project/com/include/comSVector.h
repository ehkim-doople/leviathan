/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

/********************************************************************
	2019.09.19 by KEH
	-------------------------------------------------------------
	sequential Vector
	쓰레드에 안전하지 않은 vector
	class insert 일 경우 operator = 이 꼭 필요!
	동일 Type 의 vector 들이 공용할수 있는 memoryPool 이용 ( 그 외에는 std::vector 권장)
*********************************************************************/
#pragma once
#include "comLogger.h"

#define UNIT_VECTOR_SIZE 16

template <typename T>
class CSVector 
{
public:

	CSVector(size_t size, T& value, E_ALLOC_TYPE Alloc_Type);
	CSVector(size_t size, E_ALLOC_TYPE Alloc_Type);
	~CSVector(void);			// 객체가 소멸될때 malloc 해 놓은 chunk 메모리 자동 free

	bool alloc(size_t nMaxCount);	// 한번에 메모리를 잡아놓는다.
	bool initPool(size_t count, E_ALLOC_TYPE Alloc_Type);
	inline void printPool() { if (gs_pPool) printf("ms_nCount:%d, gs_pPool:%p\n", gs_pPool->GETMAXCOUNT(), gs_pPool); else printf("gs_pPool is NULL\n"); }

	inline int capacity() const { return m_nMaxCount; }
	inline int length() const { return m_nUseCnt; }
	inline int size() const { return m_nUseCnt; }
	inline bool empty() const { if (m_nUseCnt) return false; return true; }
	inline void initResize(size_t nSize) { m_nReSizeUnit = nSize; }
	
	inline void reserve(size_t new_cap) {if (new_cap > m_nMaxCount) alloc(new_cap);}
	void clear();

	inline T* front() {	return __next_(0); }
	inline T* begin() { return __next_(0); }
	inline const T* begin() const { return __next_(0); }
	inline const T* end() const { return m_pArray[m_nLast - 1]; }
	inline T* end() { return m_pArray[m_nLast-1]; }
	inline T* back() { return m_pArray[m_nLast - 1]; }
	inline T* at(int index) { return __chk_idx(index);  }
	inline T* operator[] (int pos) { return m_pArray[pos]; }
	inline const T* operator[](int pos) const { return m_pArray[pos]; }
	inline T* next(int *pIdx) {
		for (*pIdx; *pIdx < m_nLast; (*pIdx)++) { if (m_pArray[*pIdx]) return m_pArray[*pIdx]; }
		return NULL;
	}
	inline T* insert(size_t pos, T&& value) { T value2 = value;  return __insert(pos, &value2); }
	inline T* insert(size_t pos, T& value) { return __insert(pos, &value); }
	inline T* insert(size_t pos, T* value) { return __insert(pos, value); }
	inline T* insert(size_t pos, CSVector<T> *list) { int i = 0; T* v; while ((v=list->next(&i))) { __insert(pos, v); } return m_pArray[pos]; }

	T* erase(T* p) { size_t idx = p - m_pArray[0]; if (p == m_pArray[idx]) return erase(i);  return NULL; }
	T* erase(size_t pos);
	inline T* erase(const size_t first, const size_t last) { size_t i; for (i = first; i <= last; i++)erase(i); return m_pArray[i]; }

	inline void push_back(T&& value) { T value2 = value; __insert(m_nLast, &value2); }
	inline void push_back(T& value) { __insert(m_nLast, &value); }
	inline void push_back(T* value) { __insert(m_nLast, value); }
	inline void push_back(CSVector<T> *list) { __insert(m_nLast, list); }

	inline void pop_back()  // removes the last element 
	{
		gs_pPool->delMem(m_pArray[m_nLast-1]); m_nLast--; m_pArray[m_nLast]=0; m_nUseCnt--;
	}
	inline void deepCopy(size_t nPos, T *value) {
		if (gs_pPool->IsNotClass()) c_memcpy(m_pArray[nPos], value, sizeof(T));
		else {
			*m_pArray[nPos] = *value;  // operator=
		}
	}

	void dataCollect(); // Collect Scattered Data

	inline TCHAR * getMessage()	{ return g_szMessage; }
	static CMemPool<T>* gs_pPool;

private:
	T ** m_pArray;
	size_t m_nLast;
	size_t m_nUseCnt;
	size_t	m_nMaxCount;  // 할당카운트
	size_t m_nReSizeUnit;
	E_ALLOC_TYPE m_nObjAllocType;

	T* __insert(size_t nPos, T* value);
	T* __insert(size_t nPos, CSVector<T> *value);
	void __init(E_ALLOC_TYPE Alloc_Type);

	inline T* __chk_idx(size_t i) {
		if (0 <= i && i < m_nLast) return m_pArray[i]; return NULL;
	}
	inline T* __next_(size_t i) {
		for (i; i < m_nLast; i++) { if (m_pArray[i]) return m_pArray[i]; } return NULL;
	}
	inline bool __chk_mem(size_t nPos) {
		if (m_nMaxCount <= nPos) return alloc(nPos + UNIT_VECTOR_SIZE);
		return true;
	}
};

//template <typename T> CMemPool<T>* gs_pPool;

template <typename T>
void CSVector<T>::__init(E_ALLOC_TYPE Alloc_Type)
{
	memset(g_szMessage, 0, LEN_MEM_MESSAGE);
	m_pArray = NULL;
	m_nLast = 0;
	m_nUseCnt = 0;
	m_nReSizeUnit = 0;
	m_nMaxCount = 0;
	m_bNotClass = bNotClass;
	m_nObjAllocType = Alloc_Type;

}

template <typename T>CSVector<T>::CSVector(E_ALLOC_TYPE Alloc_Type) { __init(Alloc_Type); }
template <typename T>CSVector<T>::CSVector(size_t size, T& value, E_ALLOC_TYPE Alloc_Type) { __init(Alloc_Type); alloc(size); push_back(value); }
template <typename T>CSVector<T>::CSVector(size_t size, E_ALLOC_TYPE Alloc_Type) { __init(Alloc_Type); alloc(size); }




template <typename T>
CSVector<T>::~CSVector(void)
{
    if(m_pArray) free(m_pArray);
    m_pArray = 0;
}

template <typename T>
T* CSVector<T>::__insert(size_t nPos, T *value)
{
	if (!__chk_mem(nPos)) return NULL;

	if (m_pArray[nPos]) {
		m_pArray[m_nLast] = gs_pPool->newMem();
		self_shiftR(m_pArray[m_nLast], sizeof(void *), (m_nLast - nPos) * sizeof(void *));
		m_nLast++;
	}
	else {
		if(nPos == m_nLast) m_nLast++; // push_back
		m_pArray[nPos] = gs_pPool->newMem();
	}
	m_nUseCnt++;	
	deepCopy(nPos,value);
	return m_pArray[nPos];
}


template <typename T>
T* CSVector<T>::__insert(size_t nPos, CSVector<T> *list)
{
	size_t nCnt = list->length(), i,j=nPos;
	if (!__chk_mem(m_nLast+nCnt)) return NULL;
	
	// Collect Scattered Data
	dataCollect();  

	// new Data alloc & data Move
	for (i = m_nLast; i < nCnt+m_nLast; i++) {
		m_pArray[i] = gs_pPool->newMem();	// new Data alloc
		m_pArray[i] = m_pArray[j];			// address Move
		j++;
	}

	i = 0;
	T* pVal;
	while ((pVal = list->next(&i)))
	{
		deepCopy(i + nPos, pVal);
		i++;
	}
	m_nUseCnt += nCnt;
	m_nLast += nCnt;
	return m_pArray[nPos];
}

template <typename T>
void CSVector<T>::dataCollect() // Collect Scattered Data
{
	if (!m_nUseCnt) return;
	int nPos=0, nNext=0,i;

	nNext = nPos + 1;
	while (nNext < m_nLast)
	{
		while (m_pArray[nPos]) {
			if (nPos == m_nLast) return;
			nPos++;
		}

		while (!m_pArray[nNext]) {
			nNext++;
			if (nNext == m_nLast) {
				if (m_pArray[nNext]) {
					m_pArray[nPos] = m_pArray[nNext]; // address copy
					m_pArray[nNext] = NULL;
					m_nLast = nPos+1;
				}
				return;
			}
		}

		m_pArray[nPos] = m_pArray[nNext]; // address copy
		m_pArray[nNext] = NULL;
		nPos++; nNext++;
	}

	m_nLast = nPos;
}

template <typename T>
T* CSVector<T>::erase(size_t pos)
{
	if (m_nLast <= pos) return NULL;
	if (m_pArray[pos]) {
		gs_pPool->delMem(m_pArray[pos]);
		m_pArray[pos] = 0;
		m_nUseCnt--;
	}

	if (m_nLast - 1 == pos) { m_nLast--; return NULL; }
	while (++pos < m_nLast) {
		if(m_pArray[pos]) return m_pArray[pos];
	}
	return NULL;
}


template <typename T> 
bool CSVector<T>::alloc(size_t nMaxCount)
{
	T **pOri = m_pArray;
	//T  ObjMem = 0;	
	if (pOri) {
		if (m_nMaxCount >= nMaxCount) return false;
	}

	m_pArray = (T **)calloc(nMaxCount, sizeof(T*));
    if(!m_pArray) {
		_stprintf(g_szMessage, _T("calloc ERROR m_nMaxCount[%d]"), m_nMaxCount);
		return false;
    }

	if (pOri) {
		memcpy(m_pArray, pOri, m_nMaxCount * sizeof(T*));
		free(pOri);
	}
	m_nMaxCount = nMaxCount;
	//----------------------------------------------------------------

	if (!gs_pPool) { 
		if (!initPool(nMaxCount << 1, m_nObjAllocType)) {
			_stprintf(g_szMessage, _T("initPool ERROR m_nMaxCount[%d]"), nMaxCount << 1);
			return false;
		}
	}
	return true;
}

template <typename T>
void CSVector<T>::clear()
{
	int i;
    memset(m_pArray, 0, sizeof(m_pArray));
	for(i = 0; i < m_nMaxCount; i++){
		if (m_pArray[i]) {
			gs_pPool->delMem(m_pArray[i]);
			m_pArray[i] = 0;
		}
	}
	m_nUseCnt = 0;
	m_nLast = 0;
}


template <typename T>
bool CSVector<T>::initPool(size_t count, E_ALLOC_TYPE Alloc_Type)
{
	if (gs_pPool) {
		if (bNotClass != m_bNotClass) {
			_stprintf(g_szMessage, _T("bNotClass[%s] != m_bNotClass[%s]"), bNotClass?"true":"false", m_bNotClass?"true":"false");
			return false;
		}
		return true;
	}
	gs_pPool = new CMemPool<T>();
	if (!gs_pPool) {
		_stprintf(g_szMessage, _T("new CMemPool<T>"));
		return false;
	}
	if (!gs_pPool->alloc(count, Alloc_Type)) {
		_stprintf(g_szMessage, _T("gs_pPool->alloc count[%d]"), count);
		return false;
	}
	return true;
}






