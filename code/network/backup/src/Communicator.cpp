#include "Communicator.h"
#include "comEnv.h"


void processLoopLapper(STComplete *p)
{
	CCommunicator *pChild = (CCommunicator *)p->pChild;
	STOVERLAPPED *lpOverlapped = (STOVERLAPPED *)p->lpOverlapped;
	switch (lpOverlapped->opCode) {
	case OP_ACCEPT: pChild->comproc_accept(p);
		break;
	case OP_SEND: pChild->comproc_send(p);
		break;
	case OP_RECV: pChild->comproc_recv(p);
		break;
	case OP_CLOSE: pChild->comproc_close(p);
		break;
	case OP_EVENT: pChild->comproc_event(p);
		break;
	}
}


CCommunicator::CCommunicator() :CRTSQueueHandler()
{
	m_pCSessMgr = NULL;
	m_pSockDelList = NULL;
	m_pSSessMgr = NULL;
	OnDisconnect = NULL;
}

CCommunicator::~CCommunicator()
{
    if(m_pCSessMgr)		{
        delete m_pCSessMgr;
        m_pCSessMgr = NULL;
    }
    if(m_pSockDelList)	{
        free(m_pSockDelList);
        m_pSockDelList = NULL;
    }
	if(m_pSSessMgr) {
		delete m_pSSessMgr;
		m_pSSessMgr = NULL;
	}
}

int CCommunicator::initConfig(int idx)
{
	int nServerSessionCnt, nClientSessionCnt, nThreadCnt;

	char szSect[16], szBuf[64];
	sprintf(szSect, "IOCP_%d", idx);

	nThreadCnt = GetPrivateProfileInt(szSect, "THR_COUNT", 1, g_pSystem);
	nServerSessionCnt = GetPrivateProfileInt(szSect, "ISSESSION", 0, g_pSystem);
	if (nServerSessionCnt) {
		nServerSessionCnt = GetPrivateProfileInt("SESSION", "SERVER_SESSION_MAX", 0, g_pSystem);
		nClientSessionCnt = GetPrivateProfileInt("SESSION", "CLIENT_SESSION_MAX", 0, g_pSystem);
		g_nSendDataCount = GetPrivateProfileInt("SESSION", "NR_SEND_SOCKETDATA", 0, g_pSystem);
		GetPrivateProfileString(_T("SESSION"), _T("RECV_BUF_SIZE"), "", szBuf, sizeof(szBuf), g_pSystem);
		g_nRecvBufSize = CEnv::getStrToInt(szBuf);
		return initSession(nClientSessionCnt, nServerSessionCnt, nThreadCnt);
	}
	else {
		printf("ISSESSION is 0\n");
		return 0;
	}
}

bool CCommunicator::init(fp_recvProcess fpProc)
{
	m_pRecvFunction = fpProc;
	if (CRTSQueueHandler::start(processLoopLapper, this)) return true;
	else {
		printf("CRTSQueueHandler::start FAIL\n");
		return 0;
	}

}

int CCommunicator::initSession(int nSessionMax, int nServerMax, int nThreadCnt)  // 0 이면, Server 가 아님
{
	if (!CRTSQueueHandler::init(nSessionMax + nServerMax + 50, nThreadCnt)) {
		printf("CRTSQueueHandler::init FAIL\n");
		return 0;
	}
	if(m_pCSessMgr != NULL) return 0;	// 이미 메모리 생성 완료
    if(0 < nSessionMax) {
	    m_pCSessMgr = new (std::nothrow) CSessionManager();
		if (!m_pCSessMgr) {
			printf("m_pCSessMgr is NULL\n");
			return 0;
		}
	    m_pCSessMgr->createMemoryPool(nSessionMax, IS_MEMTYPE_CONNLIST, 0);
	    m_pSockDelList = (CNWSocket **)calloc(nSessionMax, sizeof(CNWSocket *));
		if (!m_pSockDelList) {
			printf("m_pSockDelList is NULL\n");
			return 0;
		}
	}

    // For Server    
    if(0 < nServerMax) {
	    m_pSSessMgr = new (std::nothrow) CSessionManager();
		if (!m_pSSessMgr) {
			printf("m_pSSessMgr is NULL\n");
			return 0;
		}
		m_pSSessMgr->createMemoryPool(nServerMax, 0, SOCK_STATUS_SERVER);
    }

	return 1;
}

int CCommunicator::initServer(int nPort)
{
	int nRes=0;
    if(nPort) {
	    SetListenPortNo(nPort);
		nRes = createServer();
		StartServerListen();
    }
	return nRes;
}


void CCommunicator::comproc_accept(STComplete *pComplete)
{
	CNWSocket::LPSOCKETDATA lpOverlapped = reinterpret_cast<CNWSocket::LPSOCKETDATA>(pComplete->lpOverlapped);

	int nResult = lpOverlapped->lpClient->doAccept(getCRTSQueue());

	if(nResult == CERROR_NONE) {	// doAccept 안에서 bindRecv 호출
		nResult = lpOverlapped->lpClient->bindRecv(); 
		if(nResult == CERROR_NONE) {
			lpOverlapped->lpClient->SETTIMETICK(g_nTick);
			m_pCSessMgr->addSession(lpOverlapped->lpClient);
		}
		else {
			printf("bindRecv Error don't addSession\n");
			nResult = regularCloseSocket(lpOverlapped->lpClient);
		}
	}
	else if(nResult == CERROR_ACCEPT) {
		nResult = regularCloseSocket(lpOverlapped->lpClient); 
		if(nResult == CERROR_PQCS || nResult == CERROR_NOMEMORY) {
 	        StDisconInfo sInfo;
	        sInfo.nFlag = DISCONNECT_ERROR;
	        setDisconnectInfo(&sInfo, lpOverlapped->lpClient);
	        procAtonceClose(lpOverlapped->lpClient, sInfo); 
		}
	}
	
	m_pCSessMgr->prepareAccept(m_nServerSock); 
}

void CCommunicator::comproc_recv(STComplete *pComplete)
{
	CNWSocket::LPSOCKETDATA lpOverlappedEx = reinterpret_cast<CNWSocket::LPSOCKETDATA>(pComplete->lpOverlapped);
	if(!pComplete->bSuccess || pComplete->bytesTrans == 0) 
	{
		if(lpOverlappedEx->lpClient->Connected()) 
		{
			lpOverlappedEx->stCommon.opCode = OP_ERROR;
			regularCloseSocket(lpOverlappedEx->lpClient);
			gs_cLogger.PutLogQueue(LEVEL_ERROR, "OP_RECV WSAGetLastError[%d] bSuccess[%d] nBytesTrans[%d]",WSAGetLastError(), pComplete->bSuccess, pComplete->bytesTrans); 
			// WSAGetOverlappedResult
		}
		return;
	}

	// case : recv,send mode 
	lpOverlappedEx->nCurLen = pComplete->bytesTrans;
	lpOverlappedEx->lpClient->SETTIMETICK(g_nTick);
	m_pRecvFunction(lpOverlappedEx->lpClient);
	lpOverlappedEx->lpClient->bindRecv();
}

void CCommunicator::comproc_send(STComplete *pComplete)
{
	CNWSocket::LPSOCKETDATA lpOverlappedEx = reinterpret_cast<CNWSocket::LPSOCKETDATA>(pComplete->lpOverlapped);
	if(!pComplete->bSuccess || pComplete->bytesTrans == 0) 
	{
		if(lpOverlappedEx->lpClient->Connected()) 
		{
			lpOverlappedEx->stCommon.opCode = OP_ERROR;
			gs_cLogger.PutLogQueue(LEVEL_ERROR, "OP_RECV WSAGetLastError[%d] bSuccess[%d] nBytesTrans[%d]",WSAGetLastError(), pComplete->bSuccess, pComplete->bytesTrans); 
			regularCloseSocket(lpOverlappedEx->lpClient);
		}
	}
	else {
		printf("CCommunicator::comproc_send send complete\n");
		//g_cMMgr.delBuf(lpOverlappedEx->pData, lpOverlappedEx->nTotLen);
	}
}

void CCommunicator::comproc_event(STComplete *pComplete)
{
	STOVERLAPPED *lpOverlapped	= (STOVERLAPPED *)pComplete->lpOverlapped;
    StFuncData *pFuncData			= reinterpret_cast<StFuncData *>(pComplete->lpCompletion);
	pFuncData->eventProcess(pFuncData->pData);
	deleteEvent(lpOverlapped); 
}

void CCommunicator::comproc_close(STComplete *pComplete)
{
	STOVERLAPPED *lpOverlapped = (STOVERLAPPED *)pComplete->lpOverlapped;
	regularCloseSocketComplete((CNWSocket *)pComplete->lpCompletion);
	deleteEvent(lpOverlapped); 
	
	if(m_pCSessMgr) { m_pCSessMgr->prepareAccept(m_nServerSock); }
}



int	CCommunicator::createServer(void)
{
	int	so_reuseaddr	= 0;
	SOCKADDR_IN		saServer;

	m_nServerSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);		
	if(m_nServerSock < 0) 
	{
		return 0;
	}
	saServer.sin_family					= PF_INET;
	saServer.sin_addr.s_addr			= htonl(INADDR_ANY);
	saServer.sin_port					= htons((USHORT)m_nListenPort);

	if (bind (m_nServerSock, (LPSOCKADDR) &saServer, sizeof(sockaddr)) < 0) {
		printf("= ERR_SERVER_BIND\n");
		// 에러 처리
		return 0;
	}

	setsockopt(m_nServerSock, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char *)&so_reuseaddr, sizeof(so_reuseaddr));

	if (listen(m_nServerSock, 5) == -1) {
		printf("= ERR_SERVER_LISTEN\n");
		// 에러 처리
		return 0;
	}

	if(registerSocket((HANDLE)m_nServerSock,this) == CERROR_REGIOCP) return 0;
	return 1;
}

void CCommunicator::StartServerListen()
{
	m_pCSessMgr->prepareAccept(m_nServerSock,0);
}

int	CCommunicator::regularCloseSocket(CNWSocket *lpCompletion)
{
    if(lpCompletion->socketShutdown()) {
	    return putRTSQueue(OP_CLOSE, lpCompletion);
    }
    return CERROR_NONE;

}

void CCommunicator::regularCloseSocketComplete(CNWSocket *lpCompletion)
{
	StDisconInfo sInfo;
	sInfo.nFlag = DISCONNECT_NORMAL;

    if(NULL == lpCompletion) {
	    gs_cLogger.PutLogQueue(LEVEL_ERROR, "= regularCloseSocketComplete lpCompletion is NULL");
        return;
    }

	setDisconnectInfo(&sInfo, lpCompletion);
	//if(OnDisconnect) OnDisconnect(sInfo);
	procAtonceClose(lpCompletion, sInfo);
}

void CCommunicator::procAtonceClose(CNWSocket *lpCompletion, StDisconInfo sInfo)
{
    if(NULL == lpCompletion) return;
    //if(false == lpCompletion->Connected()) return;

    // gs_cLogger.PutLogQueue(LEVEL_DEBUG, "= procAtonceClose nSIdx[%d]", lpCompletion->GETSESSIONIDX());
    if(lpCompletion->closeSocket()) {
        if(sInfo.nType == SOCKETTYPE_CLIENT) {
            m_pCSessMgr->delSession(lpCompletion);
        }
        ///else if(SERVER_MAX <= sInfo.nDetail) {
        //    m_pSMSessMgr[sInfo.nDetail - SERVER_MAX]->delSession(lpCompletion);
        //}
    }
}

int	CCommunicator::procCreateSockError(CNWSocket *lpCompletion)
{
	gs_cLogger.PutLogQueue(LEVEL_ERROR, "= procCreateSockError nSIdx[%d]", lpCompletion->GETSESSIONIDX());
	lpCompletion->initCNWSocket();
	return lpCompletion->createSocket();
}

void CCommunicator::completeIOProc(CNWSocket::LPSOCKETDATA lpOverlappedEx)//, WVLogger *pLogger)
{

	BOOL nResult = 0;
    CNWSocket * lpCompletion	= lpOverlappedEx->lpClient;
    int nSIdx = lpCompletion->GETSESSIONIDX();
	int opCode = lpOverlappedEx->stCommon.opCode;


	switch (opCode) {
		case OP_ACCEPT :
			{
				nResult = lpCompletion->doAccept(getCRTSQueue());
				if(nResult == CERROR_NONE) {	// doAccept 안에서 bindRecv 호출
                    nResult = lpCompletion->bindRecv();                    
                    if(nResult == CERROR_NONE) {
				        lpCompletion->SETTIMETICK(g_nTick);
                        m_pCSessMgr->addSession(lpCompletion);
                    }
                    else printf("bindRecv Error don't addSession\n");
				}				
			}
			break;
		case OP_RECV :
			{
                 // set alive time tick
				lpCompletion->SETTIMETICK(g_nTick);
				m_pRecvFunction(lpCompletion);
                lpCompletion->bindRecv();

                return;
			}
			break;
		case OP_SEND :
			{
//				nResult = lpCompletion->completeCheckSend(lpOverlappedEx);
//				if(nResult == RESULT_COMPLETE) {//					
//					return;
//				}
			}
			break;
		case OP_ERROR :
			{
//				lpOverlappedEx->nCount--;
				regularCloseSocket(lpCompletion);
			}
			break;
	}


	while(nResult != CERROR_NONE) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, "= ERROR completeIOProc errorCode[%s] nSIdx[%d]", 
										err_desc_socket[nResult],nSIdx);
		switch(nResult)
		{
		case CERROR_CREATE		: 
			nResult = procCreateSockError(lpCompletion); 
			return;
		case CERROR_BINDACCEPT	: 
			nResult = regularCloseSocket(lpCompletion);
			return;
		case CERROR_ACCEPT		: 
			nResult = regularCloseSocket(lpCompletion); 
			break;
		case CERROR_SENDRECV	: 	
			nResult = regularCloseSocket(lpCompletion);
			break;
        case CERROR_PQCS        :            
        case CERROR_NOMEMORY    :
 	        StDisconInfo sInfo;
	        sInfo.nFlag = DISCONNECT_ERROR;
	        setDisconnectInfo(&sInfo, lpCompletion);
	        procAtonceClose(lpCompletion, sInfo);           
            return;
		case CERROR_DISCONNECTEX: 
//			procAtonceClose(lpCompletion->lpClient);
			return;
		}
	}
}


void CCommunicator::setDisconnectInfo(StDisconInfo *pInfo, CNWSocket *pSocket)
{
    if(NULL == pSocket || NULL == pInfo) return;
	pInfo->nSIdx = pSocket->GETSESSIONIDX();
//		pInfo->nFlag = DISCONNECT_TIMEOUT;	// 이전에 미리 셋팅
	pInfo->nType   = (pSocket->GETSOCKETSTATUS() & SOCK_STATUS_SERVER) ? SOCKETTYPE_SERVER : SOCKETTYPE_CLIENT;
    if(SOCKETTYPE_SERVER == pInfo->nType) {
	    pInfo->nDetail = pSocket->GETSOCKETSTATUS() & SOCK_STATUS_SERVERTYPE;
    }
    else {
	    pInfo->nDetail = pSocket->GETSOCKETSTATUS() & SOCK_STATUS_DETAIL;
	    pInfo->nDetail = pInfo->nDetail >> SOCK_STATUS_SHIFT;
    }
}



bool CCommunicator::IsServerAlive(int nSevIdx)
{
	return m_pSSessMgr->GetNWSocket(nSevIdx)->Connected();
}

int  CCommunicator::ConnectToServer(int nSevIdx, char *pIP, int nPort)
{
	return m_pSSessMgr->GetNWSocket(nSevIdx)->ConnectTo(pIP, nPort, getCRTSQueue());	
}

int CCommunicator::SendToServer(int nSevIdx, char *pBuf, int nLen)
{
    return m_pSSessMgr->GetNWSocket(nSevIdx)->Send(pBuf, nLen, getCRTSQueue()); 
}


void CCommunicator::Send(int nSIdx, char *pBuf, int nLen)
{ 
	m_pCSessMgr->GetNWSocket(nSIdx)->Send(pBuf, nLen, getCRTSQueue());

}

void CCommunicator::Close(int nSIdx)											
{
    if(0 > nSIdx ||  m_pCSessMgr->getMaxCnt(MEMTYPE_SESSION) <= nSIdx) {
        // 에러로그
        return;
    }
	CNWSocket *pSocket = m_pCSessMgr->GetNWSocket(nSIdx);
    regularCloseSocket(pSocket);
}

void CCommunicator::delClient(int nClientType, int nSIdx)
{
	if(m_pCSessMgr == NULL) return;

    if(0 > nSIdx ||  m_pCSessMgr->getMaxCnt(MEMTYPE_SESSION) <= nSIdx ) {
        // 에러로그
        return;
    }


	int nType = nClientType << SOCK_STATUS_SHIFT;
	m_pCSessMgr->GetNWSocket(nSIdx)->DELSOCKETSTATUS(nType);


}

void CCommunicator::addClient(int nClientType, int nSIdx)
{
	if(m_pCSessMgr == NULL) return;

    if(0 > nSIdx ||  m_pCSessMgr->getMaxCnt(MEMTYPE_SESSION) <= nSIdx ) {
	    //gs_cLogger.PutLogQueue(LEVEL_ERROR, "= addClient ERROR client[%s] size[%d] nSIdx[%d]", getClientString(nClientType), m_pClientArray[nClientType].size(), nSIdx);
        return;
    }

	int nType = nClientType << SOCK_STATUS_SHIFT;
	m_pCSessMgr->GetNWSocket(nSIdx)->SETSOCKETSTATUS(nType);
	//gs_cLogger.PutLogQueue(LEVEL_DEBUG, "= addClient client[%s] size[%d] nSIdx[%d]", getClientString(nClientType), m_pClientArray[nClientType].size(), nSIdx);
}

void CCommunicator::CheckTimeTick(unsigned int nCheckTime)
{
	int i=0, nCount=0;
	CNWSocket *pSocket;
//	StDisconInfo sInfo;

	if(NULL == m_pCSessMgr) return;

	nCount = m_pCSessMgr->checkSocketTimetick(nCheckTime, m_pSockDelList);
	
	for(i=0; i < nCount; i++)
	{
		pSocket = m_pSockDelList[i];
//		sInfo.nFlag = DISCONNECT_TIMEOUT;
//		m_pTaskCore->setDisconnectInfo(&sInfo, pSocket);
//		m_pTaskCore->procAtonceClose(pSocket, sInfo);
        regularCloseSocket(pSocket);
	}
	memset(m_pSockDelList, 0, sizeof(m_pSockDelList));
}


IOCPCore g_cIocpCore;

IOCPCore::IOCPCore()
{
	m_pCList = NULL;
	winSocketInit();
}
IOCPCore::~IOCPCore()
{
	if (m_pCList) delete m_pCList;
	winSocketClean();
}

bool IOCPCore::init()
{
	if (!CEnv::initSystem()) return false;
	int i,nCount;
	CCommunicator * pObj;
	nCount = GetPrivateProfileInt(_T("IOCP"), "COUNT", 1, g_pSystem);

	if (nCount) {
		m_pCList = new CSList<CCommunicator>();
		m_pCList->alloc(nCount);
		for (i = 0; i < nCount; i++)
		{
			pObj = new CCommunicator();
			m_pCList->push_back(pObj);
		}
	}
	return true;
}
