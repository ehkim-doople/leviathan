/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2021 by KimEunHye												*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/
#pragma once

#include "comTList.h"

// CRedisList 는 멀티쓰레드용 접근이 아닌, 싱글쓰레드에서 Send 하기 위한 하나의 데이터 자료구조다.
class CRedisList
{
public:

	CRedisList();
	~CRedisList();			// 객체가 소멸될때 malloc 해 놓은 chunk 메모리 자동 free
						// 한번에 메모리를 잡아놓는다.
	void clear();			// list 는 남겨 놓고 내부 메모리는 가능한 부분은 파괴한다.
	inline int alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_BufPool) { m_nObjAllocType = type;  m_nMax = __alloc(nMaxCount); return m_nMax; }

	bool add(char *p, int nLen, int *pIdx = NULL);
	bool new_push_back(char *p, int nLen=0, int *pIdx = NULL);
	bool push_back(char *p, int nLen, int *pIdx = NULL);
	inline void* getObj(int i) { if (ISINCLUDE(i, m_nMax)) return m_pArgv[i]; return NULL; }
	inline void* getNext(int *pIdx) {
		for (*pIdx; *pIdx < m_nMax; (*pIdx)++) {
			if (m_pArgv[*pIdx]) { return m_pArgv[*pIdx]; }
		}
		return NULL;
	}
	inline char** getList() { return m_pArgv;	}
	inline size_t *getSizeList() { return m_pArgvLen;	}
	inline int  size() { return m_nUse; }
	inline int  lastIdx() { return m_nLast; }
	inline int  capacity() { return m_nMax; }
	inline void setObjAllocType(E_ALLOC_TYPE type) { m_nObjAllocType = type; }

	int getRedisContents(STSringBuf *p);

private:
	char**		m_pArgv;
	size_t * m_pArgvLen;
	int	m_nUse;
	int	m_nLast;
	int	m_nMax;
	E_ALLOC_TYPE m_nObjAllocType;
	char *m_pRedisFullContents;
	int m_nContentsIdx;
	int __alloc(int nMaxCount);
	inline void _DEL(int i) { m_pArgv[i] = 0; m_nUse--;}
};


#define ADD_CLEINT_COUNT 20

class CSessionManager
{
public:
	
	CSessionManager();
	~CSessionManager();

	bool createMemoryPool(int nListCnt, int nRedisListCnt);

	CRedisList * getRedisList(); // 내부적으로 pop 으로 처리한다
	inline void returnRedisList(CRedisList *pList) { pList->clear();  m_pList->add(pList); 	//printf("getRedisList add [cur/tot:%d/%d]\n", m_pList->size(), m_pList->capacity());
	} // add 가 메모리 반환이다.

private :
	CMemList<CRedisList> *m_pList;  // 멀티쓰레드에 안전한 자료구조
};
extern CSessionManager * g_pSessMgr;
