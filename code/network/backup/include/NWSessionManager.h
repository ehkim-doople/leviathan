/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/
#pragma once
#include "comMemPool.h"
#include "comBMemPool.h"
#include "comBufPool.h"
#include "comList.h"
#include "NWSocket.h"

//#include "NWMTSync.h"



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

class CSession
{
public:
	CSession():m_pSocket(0),m_pObject(0)	{}
	virtual ~CSession() {}
	inline void SETSOCKET(CNWSocket *pSockMgr)		{ m_pSocket = pSockMgr; }
	inline CNWSocket *	GetSocket()					{ return m_pSocket; }
	inline void setObject(void *pObj)				{ m_pObject = pObj; } // 다양한 타입의 객체들을 하나의 객체로 묶어서 등록
	inline void * getObject()						{ return m_pObject; }

private:
	CNWSocket *m_pSocket;
	void *		m_pObject;		// 다양한 형태의 객체를 세션으로 묶어서 관리하도록 한다.
};

class CSessionManager
{
public:
//	typedef std::list<CNWSocket *> STCONNECTLIST;
	
	CSessionManager();
	~CSessionManager(void);

	void createMemoryPool(int nMaxCnt, char isMemType, int socketFlag);

	int getRemainCnt(int nType);			/* ANY 타입의 스택 사이즈 리턴.*/
	int getMaxCnt(int nType);
	CNWSocket *getNewNWSocket(int nFlag, int idx=0);			// 소켓 연결에 필요한 클라이언트 소켓 인스턴스 반환
	bool addSession(CNWSocket *pClient);	// 소켓 연결 완료
	void delSession(CNWSocket *pClient);	// 소켓 연결 해제
	StFuncData * newFuncMem();	
    CNWSocket::SOCKETDATA * newSockMem(); 


	void prepareAccept(SOCKET serverSocket, int nFlag = 1);


	/* 부가 기능 */
	bool isConnected(int nSIdx);
	int broadcastToAllSession(char *pBuf, int nLen, CRTSQueue *pRTSQueue);
    void sendToAllSession(CRTSQueue *pRTCQueue);
	void AllClose();
	int checkSocketTimetick(unsigned int nCheckTime,  CNWSocket **pDelList);

	inline CSession	* GetSession(int nSIdx)		{ return m_pSessionArray[nSIdx];	}				// 세션 객체 하나로 모든 정보를 알수 있다.
	inline CNWSocket * GetNWSocket(int nSIdx)	{ return m_pSessionArray[nSIdx]->GetSocket();	}	
    inline void returnSockData(CNWSocket::SOCKETDATA *pMem) { m_pDataMemMgr->delMem(pMem); }
    inline int getConnectListCount()            { return m_pConnectList->size(); }

private :
	CMemPool<CNWSocket>				* m_pSockMemMgr;
	CBMemPool<CSession>				* m_pSessMemMgr;
	CBMemPool<CNWSocket::SOCKETDATA>* m_pDataMemMgr;
	CSBufPool						* m_pRecvMemMgr;    
    /* 
		세션포인터 인덱스 관리 -- 
		다른 메모리에서 해당 세션의 index 만으로 빠르게 세션 포인터를 얻을수 있다.
	*/
	CSession** m_pSessionArray;	
	bool m_bCreateMemSession;
    CMemList<CNWSocket> * m_pConnectList;

};
