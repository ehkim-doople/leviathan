/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                   */
/*   Copyright 2012 by keh									  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
	2012.05.13 by KEH
	-------------------------------------------------------------

*********************************************************************/
#pragma once


#include "comTypes.h"
#define LEN_MEM_MESSAGE		128
#define MEMORY_ALLOCK_BLOCK	100

// MEMTYPE class 지원안함

template <typename MEMTYPE>
class CSharedPool
{
public:

    // 메모리 생성
	CSharedPool();
	~CSharedPool(void);							// 객체가 소멸될때 malloc 해 놓은 chunk 메모리 자동 free
    void clear();
	int alloc(int nMaxCount, void *pSharedChunk);	// 한번에 메모리를 잡아놓는다.

	MEMTYPE *	newMem(int *pKey=NULL);		// thread safty. 빈번한 사용, 반환이 발생할때 사용 
	bool		delMem(MEMTYPE *);			// thread safty. 빈번한 사용, 반환이 발생할때 사용

	inline void memSetComplete(int nKey) { if(m_nFlagArray[nKey].getCount() == 1) m_nFlagArray[nKey].setCount(2);  }

    // 메모리 direct 접근
    inline MEMTYPE * getMem(int i)          { return &m_pChunk[i]; }  // 안전성 보장된 상태에서 일괄처리 접근시 사용
    MEMTYPE *	getUseMem(int nKey);   // key 유효성, 사용여부 체크 - 기본 메모리 접근시 사용
	MEMTYPE *	getNext(int i, int *PNextIdx);


    // memory pool 사용현황 및 기타    
    inline int	GETUSECOUNT()		{ return m_nUseCnt; }	
	inline int	GETREMAINCOUNT()    { return m_nAllocCount-m_nUseCnt.getCount(); }	
	inline int  GETMAXCOUNT()	    { return m_nAllocCount; }
	inline char * getMessage()	    { return m_szMessage;   }

private:
    atomic_nr * m_nFlagArray;       // keh 20130305 add
    atomic_nr m_nUseCnt;
	int m_nAllocCount;
	char m_szMessage[LEN_MEM_MESSAGE];
    MEMTYPE *       m_pChunk;

};

template <typename MEMTYPE>
CSharedPool<MEMTYPE>::CSharedPool()
{
	m_nAllocCount = 0;
	memset(m_szMessage, 0 ,LEN_MEM_MESSAGE); 
	m_pChunk = NULL;
	m_nFlagArray = NULL;
}

template <typename MEMTYPE>
CSharedPool<MEMTYPE>::~CSharedPool(void)
{
    if(m_pChunk) {
        m_pChunk = NULL; // m_pChunk 는 실제 할당한 클래스 소멸자에서 메모리 해제한다. (여기서는 인자로 대입되었기 때문에 초기화만 진행)
    }
    if(m_nFlagArray) {
        delete [] m_nFlagArray;
        m_nFlagArray = NULL;
    }
	
}


template <typename MEMTYPE> 
int CSharedPool<MEMTYPE>::alloc(int nMaxCount, void *pSharedChunk)
{
	//int i;
	
	m_nFlagArray = new atomic_nr[nMaxCount];
	if(m_nFlagArray == NULL) {
	#ifdef USE_POSIX_LIB
			sprintf(m_szMessage, "CMemPool::m_nFlagArray allock ERROR");
	#else
			sprintf_s(m_szMessage, LEN_MEM_MESSAGE-1, "CMemPool::m_nFlagArray allock ERROR");
	#endif
		   return 0;
	}


	m_pChunk = (MEMTYPE *)pSharedChunk;

    if(m_pChunk == NULL) {
        
#ifdef USE_POSIX_LIB
		sprintf(m_szMessage, "CMemPool::m_pChunk allock ERROR");
#else
        sprintf_s(m_szMessage, LEN_MEM_MESSAGE-1, "CMemPool::m_pChunk allock ERROR");
#endif
	

		delete[] m_nFlagArray;
		m_nFlagArray = NULL;
		return 0;
    }

    m_nAllocCount   = nMaxCount;

#ifdef USE_POSIX_LIB
		sprintf(m_szMessage, "mem allock list m_nAllocCount[%d]", m_nAllocCount);
#else
        sprintf_s(m_szMessage, LEN_MEM_MESSAGE-1, "mem allock list m_nAllocCount[%d]", m_nAllocCount);
#endif

		

	return 1;
}

template <typename MEMTYPE>
void CSharedPool<MEMTYPE>::clear()
{
    memset(m_nFlagArray, 0, m_nAllocCount * sizeof(atomic_nr));
	m_nUseCnt.setCount(0);
	memset(m_szMessage, 0, sizeof(m_szMessage));
}



template <typename MEMTYPE> 
MEMTYPE * CSharedPool<MEMTYPE>::newMem(int *pKey)        // random pop
{
	int i= m_nUseCnt.getCount();
	
	
	while(m_nUseCnt.getCount() < m_nAllocCount)
    {
		if(i < m_nAllocCount) 
		{
			if(!m_nFlagArray[i].getCount()) 
			{
				if(!m_nFlagArray[i].atomic_compare_exchange(1,0)) 
				{
					m_nUseCnt.atomic_increment();
					if(pKey) 
					{ // NULL 이 아닌 즉 인자가 존재하면,
						*pKey = i;
					}
					return &m_pChunk[i];
				}
			}
			i++;
		}
		else i = 0;
    }    
    return NULL;
}


template <typename MEMTYPE> 
bool CSharedPool<MEMTYPE>::delMem(MEMTYPE *pUsedMem)	// random push
{
    int i;
    for(i = 0; i < m_nAllocCount; i++)
    {
        if(pUsedMem == &m_pChunk[i]) {
			m_nFlagArray[i].setCount(1);
			m_nFlagArray[i].atomic_exchange(0);
			m_nUseCnt.atomic_decrement();
            return true;
        }
    }
    return false;
}

template <typename MEMTYPE> // keh 20130305 add - 
MEMTYPE * CSharedPool<MEMTYPE>::getUseMem(int nKey)     
{
    if(0 <= nKey && nKey < m_nAllocCount)   // key 유효성 체크
    {
		if(m_nFlagArray[nKey].getCount() == 2) {            // 사용중 체크
            return &m_pChunk[nKey];
        }
    }

    return NULL;                            
    
}

template <typename MEMTYPE>	// keh 20130430 add						
MEMTYPE *  CSharedPool<MEMTYPE>::getNext(int i, int *PNextIdx)
{
    int nLastCount = m_nAllocCount;
    if(i < nLastCount) 
    {
        if(m_nFlagArray[i].getCount() == 2) {
            *PNextIdx = i + 1;
            return &m_pChunk[i];	
        }
        else 
        {
            int idx = i+1;
            for(idx; idx < nLastCount; idx++)
            {
				if(m_nFlagArray[idx].getCount() == 2) {
                    *PNextIdx = idx+1;
                    return &m_pChunk[idx];	
                }
            }
        }
    }
    return NULL;
}