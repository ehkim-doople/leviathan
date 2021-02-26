/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

/********************************************************************
2016.11.11 by KEH
-------------------------------------------------------------
FOR THREAD SAFTY!
*********************************************************************/
#pragma once


#include "comTypes.h"
#define LEN_MEM_MESSAGE		128



template <typename MEMTYPE>
class CMemPool 
{
public:

    // 메모리 생성
	CMemPool();
	~CMemPool(void);							
    void clear();
	int alloc(int nMaxCount, E_ALLOC_TYPE Alloc_Type = eAlloc_Type_newArray);


	MEMTYPE *	newMem(int *key);			
	MEMTYPE *	newMem();			
	bool		delMem(MEMTYPE *);	
 	bool		delMem(int key, MEMTYPE *pUsedMem);
   // 메모리 direct 접근
	inline MEMTYPE * getMem(int i) { return &m_pChunk[i]; }  // 안전성 보장된 상태에서 일괄처리 접근시 사용
    MEMTYPE *	getUseMem(int nKey);  
	MEMTYPE *	getNext(int *pos);

    // memory pool 사용현황 및 기타    
	inline int	GETUSECOUNT()		{ return m_nUseCnt.getCount(); }	
	inline int	GETREMAINCOUNT()    { return m_nAllocCount-m_nUseCnt.getCount(); }	
	inline int  GETMAXCOUNT()	    { return m_nAllocCount; }
	inline TCHAR * getMessage()	    { return m_szMessage;   }
	inline void setBStruct(bool bStruct) {m_bNotClass = bStruct;}
	inline bool ISINCLUDE_ADDRESS(MEMTYPE *p) { if(m_pChunk <= p && p < m_pChunk+m_nAllocCount) return true; return false; }
private:
    atomic_nr * m_nFlagArray;       
    atomic_nr	m_nUseCnt;
	int	m_nLastCount;		
	int m_nAllocCount;
	//TCHAR m_szMessage[LEN_MEM_MESSAGE];
	E_ALLOC_TYPE m_nObjAllocType;
    MEMTYPE *       m_pChunk;
	MEMTYPE * __unit_alloc();

};

template <typename MEMTYPE>
CMemPool<MEMTYPE>::CMemPool()
{
	m_nAllocCount = 0;
	m_nLastCount = 0;
	//memset(m_szMessage, 0 ,LEN_MEM_MESSAGE);
	m_pChunk = NULL;
	m_nFlagArray = NULL;
	m_nObjAllocType = eAlloc_Type_none;
}

template <typename MEMTYPE>
CMemPool<MEMTYPE>::~CMemPool(void)
{
	if (m_nFlagArray) {
		delete[] m_nFlagArray;
		m_nFlagArray = NULL;
	}
	
	if(m_pChunk) {
		switch (m_nObjAllocType) {
		case eAlloc_Type_new:		
		case eAlloc_Type_newArray:	delete[] m_pChunk; break;
		case eAlloc_Type_alloc:		free(m_pChunk); break;
		}
		
		m_pChunk = NULL;
	}

}

template <typename MEMTYPE> 
int CMemPool<MEMTYPE>::alloc(int nMaxCount, E_ALLOC_TYPE Alloc_Type)
{
	m_nFlagArray = new (std::nothrow) atomic_nr[nMaxCount];
	
	if(!m_nFlagArray) {
		_stprintf(g_szMessage, _T("atomic_nr[%d] is NULL"), nMaxCount);
		return 0;
	}

	m_nObjAllocType = Alloc_Type;

	switch (m_nObjAllocType) {
	case eAlloc_Type_new		:
	case eAlloc_Type_newArray	:	m_pChunk = new (std::nothrow) MEMTYPE[nMaxCount]; break;
	case eAlloc_Type_alloc		:	m_pChunk = (MEMTYPE *)calloc(nMaxCount, sizeof(MEMTYPE)); break;
	default: break;
	}


    if(!m_pChunk) {
		_stprintf(g_szMessage, _T("m_pChunk is NULL [%d]"), nMaxCount);
		delete[] m_nFlagArray;
		m_nFlagArray = NULL;
		return 0;
    }

    m_nAllocCount   = nMaxCount;
	clear();
	//_stprintf(m_szMessage, _T("mem allock list m_nAllocCount[%d]"), m_nAllocCount);
	return 1;
}



template <typename MEMTYPE>
void CMemPool<MEMTYPE>::clear()
{
	int i;
	m_nUseCnt.init();
	m_nLastCount = 0;
	for(i = 0; i < m_nAllocCount; i++)
	{
		m_nFlagArray[i].init();
	}
}

template <typename MEMTYPE>
MEMTYPE * CMemPool<MEMTYPE>::__unit_alloc()
{
	switch (m_nObjAllocType) {
	case eAlloc_Type_new:
	case eAlloc_Type_newArray	:	return new (std::nothrow) MEMTYPE;
	case eAlloc_Type_alloc		:	return (MEMTYPE *)calloc(1, sizeof(MEMTYPE));
	default: return NULL;
	}
}


template <typename MEMTYPE> 
MEMTYPE * CMemPool<MEMTYPE>::newMem()        // random pop
{
	int i= m_nLastCount,j;
	if (m_nUseCnt.getCount() == m_nAllocCount) {
		return __unit_alloc();
	}
	
	for(j = 0; j < m_nAllocCount; j++)
    {
		if(i >= m_nAllocCount) {i = 0;} // 큐방식 만큼 타이트하지 않다. 대략 next 메모리 위치를 가리키기 위함이다.
		if(!m_nFlagArray[i].getCount()) 
		{
			if(!m_nFlagArray[i].atomic_compare_exchange(1,0)) 
			{
				m_nUseCnt.atomic_increment();
				m_nLastCount=i;
				return &m_pChunk[i];
			}
		}
		i++;
    }  
	return __unit_alloc();
}


template <typename MEMTYPE> 
MEMTYPE * CMemPool<MEMTYPE>::newMem(int *nKey)        // random pop
{
	int i = m_nLastCount, j;
	if (m_nUseCnt.getCount() == m_nAllocCount) {
		*nKey = -1;
		//return __unit_alloc();
	}

	for (j = 0; j < m_nAllocCount; j++)
	{
		if (i >= m_nAllocCount) { i = 0; } // 큐방식 만큼 타이트하지 않다. 대략 next 메모리 위치를 가리키기 위함이다.
		if (!m_nFlagArray[i].getCount())
		{
			if (!m_nFlagArray[i].atomic_compare_exchange(1, 0))
			{
				m_nUseCnt.atomic_increment();
				m_nLastCount = i;
				*nKey = i;
				return &m_pChunk[i];
			}
		}
		i++;
	}
	return NULL;
}

template <typename MEMTYPE> 
bool CMemPool<MEMTYPE>::delMem(MEMTYPE *pUsedMem)	
{
	int nIdx = int(pUsedMem - m_pChunk);

	if (ISINCLUDE(nIdx, m_nAllocCount)) {
		m_nFlagArray[nIdx].init();
		m_nUseCnt.atomic_decrement();
		return true;
	}
	switch (m_nObjAllocType) {
	case eAlloc_Type_new:
	case eAlloc_Type_newArray:	delete pUsedMem; break;
	case eAlloc_Type_alloc:	free(pUsedMem); break;
	}
	return false;
}

template <typename MEMTYPE> 
bool CMemPool<MEMTYPE>::delMem(int key, MEMTYPE *pUsedMem)	
{
	if (ISINCLUDE(nKey, m_nAllocCount)) {
		m_nFlagArray[key].init();
		m_nUseCnt.atomic_decrement();
		return true;
	}
	switch (m_nObjAllocType) {
	case eAlloc_Type_new:
	case eAlloc_Type_newArray:	delete pUsedMem; break;
	case eAlloc_Type_alloc:	free(pUsedMem); break;
	}
	return false;
}

template <typename MEMTYPE> // keh 20130305 add - 
MEMTYPE * CMemPool<MEMTYPE>::getUseMem(int nKey)     
{
    if(ISINCLUDE(nKey,m_nAllocCount))   // key 유효성 체크
    {
		if(m_nFlagArray[nKey].getCount()) {            // 사용중 체크
            return &m_pChunk[nKey];
        }
    }
    return NULL;                
}

template <typename MEMTYPE>	// keh 20130430 add						
MEMTYPE *  CMemPool<MEMTYPE>::getNext(int *pos)
{
    if(*pos < m_nAllocCount)
    {
        if(m_nFlagArray[*pos].getCount()) {
            return &m_pChunk[*pos];
        }
        else 
        {
            int idx = *pos +1;
            for(idx; idx < m_nAllocCount; idx++)
            {
				if(m_nFlagArray[idx].getCount()) {
					*pos = idx;
                    return &m_pChunk[idx];	
                }
            }
        }
    }
    return NULL;
}

