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
Comunicator : 1 IOCP handler, 1 RecvParsingFunctionPointer, 1 PacketParsingFunctionPointer
Comunicator List : Comunicator group
*********************************************************************/

#pragma once

#include "NWHandler.h"
#include "NWSessionManager.h"
#include "comTList.h"

enum {
	CONNECT_FAIL	= 0,
	CONNECT_SUCCESS ,
	CONNECT_ALREADY
};
typedef void(*fp_parsingProcess) (char *pData, int nSize);
typedef void (*fp_recvProcess)(void * pSocket, fp_parsingProcess fp_parsingProc);
typedef void(*fp_OnDisconnect) (StDisconInfo sInfo);

class CNWCommunicator
{
public :
	CNWCommunicator();
	~CNWCommunicator();
	int initConfig(int idx, int *pMaxCount);  // return iocpIdx
	bool init(int nClientMax, CNWHandler *p);
	bool start(fp_recvProcess fpRecvProc, fp_parsingProcess fpParsingProc);
	int initListenSocket(int nPort, const char* szIP = g_sAnyIp);	// server socket 생성 및 Listen
	bool initConnectToSocket(int * pSevIdx);
	bool  ConnectToServer(char *pIP, int nPort, int nMiliseconds, int nSIdx=0);


	void comproc_accept(STComplete *pComplete);
	void comproc_recvComplete(STComplete *pComplete);
	void comproc_sendComplete(STComplete *pComplete);
	void comproc_event(STComplete *pComplete);
	void comproc_close(STComplete *pComplete);

	// For Client Socket
	int		getClientCount();
	inline CNWSocket *	getSocket(int nSIdx=0) { return g_pSessMgr->getUsedSession(nSIdx); }

	bool Send(char *pBuf, int nLen, int nSIdx=0);
	bool Send2(char *pBuf, int nLen, CLogger *p, int nSIdx = 0);
	void Close(int nSIdx=0);
	void setClientType(int nClientType, int nSIdx=0);
	bool IsConnected(int nSIdx=0);
	bool checkConnect(int nSIdx = 0);

	// 콜백등록 인터페이스
	inline void setCallbackDisconnect(fp_OnDisconnect p) { m_pDisconnectFunction = p; }
	int timeoutProcess();
	int broadcastToAllSession(char *pBuf, int nLen);
private :	
	int		m_nListenPort;
	unsigned int m_nTimeout;
	int		m_nAcceptedMax;
	SOCKET	m_nServerSock;
	CNWHandler *m_pIOCPHandler;
	CMemList<CNWSocket> * m_pConnectList;	// 가변

    void    regularCloseSocketComplete(CNWSocket *lpCompletion);
	int	    shutdownPutQueue(CNWSocket *lpCompletion);
	void	procAtonceClose(CNWSocket *lpCompletion, StDisconInfo sInfo);
	void	setDisconnectInfo(StDisconInfo *pInfo, CNWSocket *pSocket);

	fp_OnDisconnect		m_pDisconnectFunction;
	fp_recvProcess		m_pRecvFunction;
	fp_parsingProcess	m_pParsingProc;
};



class CNWCommunicatorList
{
public :
	CNWCommunicatorList();
	~CNWCommunicatorList();
	void destroy();

	bool initConfig();
	bool initIocp(int nMax, int nThreadCnt, int nSeq);	// IOCP queue list max count, thread count, object Idx
	bool initCommunicator(int nSeq); // IocpIdx (ini file seq), client connect list max count
	bool initCommunicator(int *pIdx, int nIocpIdx, int nMax); // object idx, client connect list max count
	inline CNWCommunicator *getObj(int i) { return m_pCList->getObj(i);  }
private:
	CTList<CNWCommunicator> *m_pCList;
	CTList<CNWHandler> *m_pQHandlerList;
};
extern CNWCommunicatorList *g_pNWCList;
