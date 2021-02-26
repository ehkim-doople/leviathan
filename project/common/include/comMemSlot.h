/********************************************************************
	2016.12.12 by KEH
	-------------------------------------------------------------
	thread-safety Memory Slot

*********************************************************************/
#pragma once

#include "comMemPool.h"
#define MAX_MEM_SLOT		100


template <typename T>
class CMemSlot
{
public :
	CMemSlot(bool bNotClass=0);
	~CMemSlot();
	int alloc(int nMaxCount);
    void clear();
	void destroy();

	T *	newMem();		
	T *	newLMem(); // last slot memory return
	bool delMem(T *p);	

    // 메모리 direct 접근
    inline CMemPool<T> * getElement(int i)			{ return m_memSList[i]; }  // 안전성 보장된 상태에서 일괄처리 접근시 사용
	inline CMemPool<T> * getNextElement(int i)		{ if(i >= m_nUseCnt) return NULL; return m_memSList[i]; }

    // memory pool 사용현황 및 기타    
    int	GETUSECOUNT();
	int	GETREMAINCOUNT();
	int  GETMAXCOUNT();
private :
	int m_nUseCnt;
	CMemPool<T> *m_memSList[MAX_MEM_SLOT];
	COM_CRITICAL_SECTION m_cLock;
    bool m_bNotClass;
};


template <typename T>
CMemSlot<T>::CMemSlot(bool bNotClass)
{
	m_nUseCnt = 0;
	memset(m_memSList, 0, sizeof(m_memSList));
	m_bNotClass = bNotClass;
}


template <typename T>
CMemSlot<T>::~CMemSlot()
{
	destroy();
}

template <typename T>
int CMemSlot<T>::alloc(int nMaxCount)
{
	m_memSList[0] = new CMemPool<T>(m_bNotClass);
	if(m_memSList[0]) return m_memSList[0]->alloc(nMaxCount);
	m_nUseCnt = 1;
	return 0;
}

template <typename T>
void CMemSlot<T>::clear()
{
	CMemPool<T> *pMemPool;
	int i, nCount=0;
	for(i = 0 ; i < m_nUseCnt; i++) {
		pMemPool = m_memSList[i];
		pMemPool->clear();
	}
}

template <typename T>
void CMemSlot<T>::destroy()
{
	CMemPool<T> *pMemPool;
	int i, nCount=0;
	for(i = 0 ; i < m_nUseCnt; i++) {
		pMemPool = m_memSList[i];
		if(pMemPool) delete pMemPool;
		m_memSList[i] = NULL;
	}
	m_nUseCnt = 0;
}

template <typename T>
T * CMemSlot<T>::newMem()
{
	CMemPool<T> *pMemPool;
	T *pRes = NULL;
	int i;
	for(i = 0 ; i < m_nUseCnt; i++) {
		pMemPool = m_memSList[i];
		if(pMemPool->GETREMAINCOUNT()) {
			pRes = pMemPool->newMem(p);
			if(pRes) return pRes;
		}
	}

	if(m_nUseCnt == MAX_MEM_SLOT) {
		printf("slot max! m_nUseCnt[%d] max[%d]\n", m_nUseCnt, MAX_MEM_SLOT);
		pRes = new T;
		return pRes;
	}

	m_cLock.enter();
	if(!m_memSList[m_nUseCnt]) {

		pMemPool = new CMemPool<T>(m_bNotClass);
		if(pMemPool) {
			if(pMemPool->alloc(nMaxCount)) {
				pRes = pMemPool->newMem();
				m_memSList[m_nUseCnt] = pMemPool;
			}
			else {
				delete pMemPool;
				printf("alloc CMemPool FAIL! maxCount[%d]\n", nMaxCount);
			}
		}
		else {
			printf("new CMemPool FAIL!\n");
		}
	}
	else {
		pRes = m_memSList[m_nUseCnt]->newMem();
		m_nUseCnt++;
	}
	
	m_cLock.leave();
	return pRes;
}

template <typename T>
T *	CMemSlot<T>::newLMem()
{
	T *pRes = m_memSList[m_nUseCnt-1]->newMem();

	if(!pRes) {
		return newMem();
	}

	return pRes;
}

template <typename T>
void CMemSlot<T>::delMem(T *p)
{
	CMemPool<T> *pMemPool;
	int i;
	for(i = 0 ; i < m_nUseCnt; i++) {
		pMemPool = m_memSList[i];
		if(pMemPool->ISINCLUDE(p)) {
			if(!pMemPool->delMem(p)) delete p;
		}
	}
}

template <typename T>
int	CMemSlot<T>::GETUSECOUNT()				
{ 
	CMemPool<T> *pMemPool;
	int i, nCount=0;
	for(i = 0 ; i < m_nUseCnt; i++) {
		pMemPool = m_memSList[i];
		nCount += pMemPool->GETUSECOUNT();
	}
	return nCount;
}

template <typename T>
int	CMemSlot<T>::GETREMAINCOUNT()			
{ 
	CMemPool<T> *pMemPool;
	int i, nCount=0;
	for(i = 0 ; i < m_nUseCnt; i++) {
		pMemPool = m_memSList[i];
		nCount += pMemPool->GETREMAINCOUNT();
	}
	return nCount;
}

template <typename T>
int CMemSlot<T>::GETMAXCOUNT()	   
{ 
	CMemPool<T> *pMemPool = m_memSList[0];
	return pMemPool->GETMAXCOUNT() * m_nUseCnt;
}

