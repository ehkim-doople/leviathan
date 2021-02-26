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
// - thread safty;  Socket related memory!
*********************************************************************/



#pragma once
#include "comMemPool.h"
#include "comBMemPool.h"
#include "comBufPool.h"
#include "NWSocket.h"




enum {
    POP_WAY_STACK   = 1,
    POP_WAY_QUEUE,
    POP_WAY_INDEX,
    POP_WAY_FAST
};



enum {
	MEMTYPE_SESSION		= 1,
	MEMTYPE_NWSOCKET	,
	MEMTYPE_SOCKETDATA	,
	MEMTYPE_QUEUEDATA
};

enum {
    IS_MEMTYPE_CONNLIST     = 0x01
};


enum ENETFLAG {
    eNET_FLAG_BALANCE_NONE = 0,
    eNET_FLAG_BALANCE_CRITICAL = 1,
    eNET_FLAG_BALANCE_BAD,
    eNET_FLAG_BALANCE_WARNING,
    eNET_FLAG_BALANCE_SMOOTH,
    eNET_FLAG_BALANCE_LOW
};

#define ADD_CLEINT_COUNT 20

class CSessionManager
{
public:
	
	CSessionManager();
	~CSessionManager(void);

	void createMemoryPool(int nMaxCnt, int nSDataCnt);

	inline int getRemainFAccept() { int n = m_pSockMemMgr->GETREMAINCOUNT(); n -= ADD_CLEINT_COUNT;  if (0 > n) return 0;  return n; }
	inline int getMaxCnt(int nType) { return m_pSockMemMgr->GETMAXCOUNT(); }
	//StFuncData * newFuncMem();			// TODO
	inline CNWSocket * newSocket(int *pKey = NULL) { int nKey; if (!pKey) pKey = &nKey; return m_pSockMemMgr->newMem(pKey);}
	inline bool delSocket(CNWSocket *p) { p->RETURN_DATABUF(); return m_pSockMemMgr->delMem(p); }
	inline CNWSocket::SOCKETDATA * newSockData() { return m_pDataMemMgr->newMem(); }


	void prepareAccept(SOCKET serverSocket, int nMax);

	/* 부가 기능 */
	void AllClose();

	inline CNWSocket * GetSession(int nSIdx) { return m_pSockMemMgr->getMem(nSIdx); }				// 세션 객체 하나로 모든 정보를 알수 있다.
	inline CNWSocket * getUsedSession(int nSIdx)	{ return m_pSockMemMgr->getUseMem(nSIdx);	}
    inline void returnSockData(CNWSocket::SOCKETDATA *pMem) { m_pDataMemMgr->delMem(pMem); }

private :
	CMemPool<CNWSocket>				* m_pSockMemMgr;
	CMemPool<CNWSocket::SOCKETDATA> * m_pDataMemMgr;

};
extern CSessionManager * g_pSessMgr;
