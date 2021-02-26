/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

/********************************************************************
	2019.09.27 by KEH
	-------------------------------------------------------------
	ring queue : 자동 리스트 메모리 증가
	다중 쓰레드 => push, 다중 쓰레드 => pop 모델
*********************************************************************/
#pragma once
#include "comTypes.h"


// TODO: complete !! 20190926
// m_nFlagArray 를 없애고 스핀락으로 교체
// void* 형으로 변경
// 리스트 size 자동 증가로 수정



class CQueueS
{
public:

	CQueueS();
	~CQueueS(void);				// 객체가 소멸될때 malloc 해 놓은 chunk 메모리 자동 free
	bool alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_new);	// 한번에 메모리를 잡아놓는다.
	bool realloc(int nMaxCount, bool bInit=false);

	bool push(void *pData); 
	void* pop();
	inline void* next(int *idx) { void *res = m_pArray[*idx]; *idx++; return res; }
	//inline void* circularNext(int *idx) { void *res = m_pArray[*idx]; *idx++; return res; }

	inline bool empty() {return (m_nLast - m_nFront) ? false : true;}

	inline TCHAR * getMessage()	{ return g_szMessage; }
    inline int  capacity()   { return m_nMax; }
	inline int  size() { int res = m_nLast - m_nFront; if (res < 0) return -res; return res; }

private:
	void**	m_pArray;
	int	m_nLast;
	int	m_nFront;
	int	m_nMax;
	E_ALLOC_TYPE m_nObjAllocType;
	SPIN_LOCK m_cLockPush; // for read (by realloc)
	COM_CRITICAL_SECTION m_CS;  // for realloc
	SPIN_LOCK  m_cLockPop; // for push
	int m_nOldMax;
	atomic_nr m_nRealloc;


};


