#pragma once

#include "RTSQueueHandler.h"
#include "NWSocket.h"
#include "NWSessionManager.h"

enum {
	CONNECT_FAIL	= 0,
	CONNECT_SUCCESS ,
	CONNECT_ALREADY
};

typedef void (*fp_recvProcess)(CNWSocket * pSocket, int nRecvBuf);
class CCommunicator : public CRTSQueueHandler
{
public :
	CCommunicator();
	//virtual ~CCommunicator();
	~CCommunicator();
	int initConfig(int idx);
	bool init(fp_recvProcess fpProc);
	int initSession(int nSessionMax, int nServerMax, int nThreadCnt);	// 技记 包府 皋葛府 积己
	int initServer(int nPort);	// server socket 积己 棺 Listen

	//int start();

	void comproc_accept(STComplete *pComplete);
	void comproc_recv(STComplete *pComplete);
	void comproc_send(STComplete *pComplete);
	void comproc_event(STComplete *pComplete);
	void comproc_close(STComplete *pComplete);


	//virtual void recvProcess(CNWSocket * pSocket) = NULL;

	void CheckTimeTick(unsigned int nCheckTime);

	// For Client Socket
    inline void     setSessionObject(int nSIdx, void* pObj)   { m_pCSessMgr->GetSession(nSIdx)->setObject(pObj); }
    inline void *   getSessionObject(int nSIdx)               { return m_pCSessMgr->GetSession(nSIdx)->getObject(); }
    inline int		getClientCount() { return m_pCSessMgr->getConnectListCount(); }
	inline CNWSocket *	getCSocket(int nSIdx) { return m_pCSessMgr->GetNWSocket(nSIdx); }

	void Send(int nSIdx, char *pBuf, int nLen);
	void Close(int nSIdx);												
	void delClient(int nClientType, int nSIdx);
	void addClient(int nClientType, int nSIdx);


	// For Server Socket
	inline	void SetListenPortNo(int nPort)	{ m_nListenPort = nPort; }
			bool IsServerAlive(int nSevIdx);
			int  ConnectToServer(int nSevIdx, char *pIP, int nPort);
			int SendToServer(int nSevIdx, char *pBuf, int nLen);
	inline CNWSocket *	getSSocket(int nSevIdx) { return m_pSSessMgr->GetNWSocket(nSevIdx); }
	inline  void CloseToServer(int nSevIdx) { regularCloseSocket(m_pSSessMgr->GetNWSocket(nSevIdx)); }

	// 妮归殿废 牢磐其捞胶
	inline void setCallbackDisconnect(void (*OnDisconnect) (StDisconInfo sInfo)) { OnDisconnect = OnDisconnect; }

private :	
	SOCKET	m_nServerSock; // SERVER SOCKET FD
	int		m_nListenPort;
	int		m_nTimeout;

    CSessionManager * m_pCSessMgr;			// 包府且 促吝 努扼捞攫飘 家南 按眉(家南 & 技记)
    CSessionManager * m_pSSessMgr;			// 促吝 辑滚 家南 按眉(家南 & 技记)
	CNWSocket		**m_pSockDelList;		// for client disAlive Socket

    void	completeIOProc(CNWSocket::LPSOCKETDATA lpCompletion);
    void    regularCloseSocketComplete(CNWSocket *lpCompletion);
	int		createServer(void);
	void	StartServerListen();
	int	    regularCloseSocket(CNWSocket *lpCompletion);
	void	procAtonceClose(CNWSocket *lpCompletion, StDisconInfo sInfo);
	int	    procCreateSockError(CNWSocket *lpCompletion);

	void setDisconnectInfo(StDisconInfo *pInfo, CNWSocket *pSocket);

	void (*OnDisconnect) (StDisconInfo sInfo);
	fp_recvProcess m_pRecvFunction;
};



class IOCPCore
{
public :
	IOCPCore();
	~IOCPCore();

	bool init();
	inline CCommunicator *getObj(int i) { return m_pCList->getObj(i);  }
private:
	CSList<CCommunicator> *m_pCList;
};

extern IOCPCore g_cIocpCore;
