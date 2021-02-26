#include "NWCommunicator.h"
#include "comEnv.h"

//static CSessionManager * g_pIocpSessMgr;
void processLoopLapper(STComplete *p)
{
	CNWCommunicator *pChild = (CNWCommunicator *)p->pChild;
	STOVERLAPPED *lpOverlapped = (STOVERLAPPED *)p->lpOverlapped;
	switch (lpOverlapped->opCode) {
	case OP_ACCEPT: pChild->comproc_accept(p);
		break;
	case OP_SEND: pChild->comproc_sendComplete(p);
		break;
	case OP_RECV: pChild->comproc_recvComplete(p);
		break;
	case OP_CLOSE: pChild->comproc_close(p);
		break;
	case OP_EVENT: pChild->comproc_event(p);
		break;
	}
}


CNWCommunicator::CNWCommunicator()
{
	m_pDisconnectFunction = NULL;
	m_pRecvFunction = NULL;
	m_pParsingProc = NULL;
	m_nTimeout = 0;

	m_pConnectList = NULL;
	m_nListenPort = 0;
	m_nAcceptedMax = 0;
	m_nServerSock = 0;
	m_pIOCPHandler = NULL;

}

CNWCommunicator::~CNWCommunicator()
{

 	if (m_pConnectList) {
		delete m_pConnectList;
		m_pConnectList = NULL;
	}
}

int CNWCommunicator::initConfig(int idx, int *pMaxCount)
{
	char szSect[16];
	sprintf(szSect, "COMUNICATOR_%d", idx);

	*pMaxCount = GetPrivateProfileInt(szSect, "CLIENT_SESSION_MAX", 0, g_pSystem);
	return GetPrivateProfileInt(szSect, "IOCP_IDX", 0, g_pSystem);
}

bool CNWCommunicator::init(int nClientMax, CNWHandler *p)
{
	m_nAcceptedMax = nClientMax;
	if (0 < m_nAcceptedMax)
	{
		m_pConnectList = new (std::nothrow) CMemList<CNWSocket>();
		if (!m_pConnectList->alloc(m_nAcceptedMax + ADD_LIST_COUNT)) {
			sprintf(g_szMessage, "m_pConnectList->alloc(%d) has Faild", m_nAcceptedMax + ADD_LIST_COUNT);
			comErrorPrint(g_szMessage);
			return false;
		}
	}
	m_pIOCPHandler = p;
	return true;
}

bool CNWCommunicator::start(fp_recvProcess fpRecvProc, fp_parsingProcess fpParsingProc)
{
	m_pRecvFunction = fpRecvProc;
	m_pParsingProc = fpParsingProc;
	return m_pIOCPHandler->start(processLoopLapper, this);
}


int CNWCommunicator::initListenSocket(int nPort, const char* szIP)
{
	int nSevSIdx=-1;
	CNWSocket *pSocket = g_pSessMgr->newSocket(&nSevSIdx);
	if (!pSocket) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("newSocket is NULL! nPort[%d] szIP[%s]"),nPort, szIP);
		return -1;
	}

	if (pSocket->SockServerInit(nPort, szIP) != CERROR_NONE) {
		goto INITLISEND_ERROR;
	}
	if (m_pIOCPHandler->registerSocket((HANDLE)pSocket->GETSOCKET(), this)) { // error print in function
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("registerSocket has Failed! nPort[%d] szIP[%s]"), nPort, szIP);
		goto INITLISEND_ERROR;
	}
	else {
		m_nServerSock = pSocket->GETSOCKET();
		g_pSessMgr->prepareAccept(m_nServerSock, m_nAcceptedMax);
		return nSevSIdx;
	}


	INITLISEND_ERROR:
	pSocket->closeSocket(false);
	g_pSessMgr->delSocket(pSocket);
	return -1;
}


void CNWCommunicator::comproc_accept(STComplete *pComplete)
{
	CNWSocket::LPSOCKETDATA lpOverlapped = reinterpret_cast<CNWSocket::LPSOCKETDATA>(pComplete->lpOverlapped);;
	CNWSocket *pClient = lpOverlapped->lpClient;
	int nResult = pClient->doAccept(m_pIOCPHandler->getCRTSQueue());

	if(nResult == CERROR_NONE) {	// doAccept 안에서 bindRecv 호출
		nResult = pClient->bindRecv();
		if(nResult == CERROR_NONE) {
			pClient->SETTIMETICK(GetTickCount());

			if (m_pConnectList->add(pClient))
			{
				gs_cLogger.PutLogQueue(LEVEL_TRACE, "addSession nSIdx[%d] ListCount[%d]", pClient->GETSESSIONIDX(), m_pConnectList->size());
			}
			else {
				gs_cLogger.DebugLog(LEVEL_ERROR, "addSession nSIdx[%d] ListCount[%d]", pClient->GETSESSIONIDX(), m_pConnectList->size());
				pClient->socketShutdown();
			}
		}
		else {
			gs_cLogger.DebugLog(LEVEL_ERROR, "bindRecv error nSIdx[%d] fd[%d]", lpOverlapped->lpClient->GETSESSIONIDX(), lpOverlapped->lpClient->GETSOCKET());
			pClient->socketShutdown();
		}
	}
}



void CNWCommunicator::comproc_recvComplete(STComplete *pComplete)
{
	CNWSocket::LPSOCKETDATA lpOverlappedEx = reinterpret_cast<CNWSocket::LPSOCKETDATA>(pComplete->lpOverlapped);
	if(!pComplete->bSuccess || pComplete->bytesTrans == 0) 
	{
		lpOverlappedEx->stCommon.opCode = OP_ERROR;
		gs_cLogger.PutLogQueue(LEVEL_ERROR, "OP_RECVCOPLETE WSAGetLastError[%d] bSuccess[%d] nBytesTrans[%d]", WSAGetLastError(), pComplete->bSuccess, pComplete->bytesTrans);
		shutdownPutQueue(lpOverlappedEx->lpClient);
		return;
	}

	// case : recv,send mode 
	lpOverlappedEx->nCurLen = pComplete->bytesTrans;
	lpOverlappedEx->lpClient->SETTIMETICK(GETTICKCOUNT());
	if (m_pRecvFunction) {
		m_pRecvFunction(lpOverlappedEx->lpClient, m_pParsingProc);
	}
	//lpOverlappedEx->lpClient->bindRecv();
	//lpOverlappedEx->lpClient->RecvPacket();
	// recv 는 function 내부에서 호출
}

void CNWCommunicator::comproc_sendComplete(STComplete *pComplete)
{
	CNWSocket::LPSOCKETDATA lpOverlappedEx = reinterpret_cast<CNWSocket::LPSOCKETDATA>(pComplete->lpOverlapped);
	CNWSocket *pSocket = lpOverlappedEx->lpClient;
	if(!pComplete->bSuccess || pComplete->bytesTrans == 0)
	{
		if(pSocket->Connected())
		{
			pSocket->SETSOCKETSTATUS(SOCK_STATUS_CLOSE_ERROR);
			gs_cLogger.DebugLog(LEVEL_ERROR, "nSIdx[%d] nFD[%u] WSAGetLastError[%d] bSuccess[%d] nBytesTrans[%d]", 
				pSocket->GETSESSIONIDX(), (UINT)pSocket->GETSOCKET(),
				WSAGetLastError(), pComplete->bSuccess, pComplete->bytesTrans); 
			shutdownPutQueue(pSocket);
		}
	}
	else if(lpOverlappedEx->pObj) {
		CLogger *pLogger = (CLogger *)lpOverlappedEx->pObj;
		pLogger->Log(LEVEL_INFO, lpOverlappedEx->pData);
	}

	gs_cLogger.PutLogQueue(LEVEL_TRACE, "SendComplete");


	// CNWSocket::LPSOCKETDATA return
	gs_pMMgr->delBuf(lpOverlappedEx->pData, lpOverlappedEx->nTotLen+1);
	g_pSessMgr->returnSockData(lpOverlappedEx);
}

void CNWCommunicator::comproc_event(STComplete *pComplete)
{
	STOVERLAPPED *lpOverlapped	= (STOVERLAPPED *)pComplete->lpOverlapped;
    StFuncData *pFuncData			= reinterpret_cast<StFuncData *>(pComplete->lpCompletion);
	pFuncData->eventProcess(pFuncData->pData);
	m_pIOCPHandler->deleteEvent(lpOverlapped);
}

void CNWCommunicator::comproc_close(STComplete *pComplete)
{
	STOVERLAPPED *lpOverlapped = (STOVERLAPPED *)pComplete->lpOverlapped;
	CNWSocket * pSocket = (CNWSocket *)pComplete->lpCompletion;

	CNWSocket::LPSOCKETDATA lpSendData = pSocket->GETSENDDATA();
	if (lpSendData) {
		if (lpSendData->pData) {
			gs_pMMgr->delBuf(lpSendData->pData, lpSendData->nTotLen+1);
			lpSendData->pData = NULL;
			lpSendData->nTotLen = 0;
		}
		pSocket->INITSENDDATA();
		g_pSessMgr->returnSockData(lpSendData);
	}
	regularCloseSocketComplete(pSocket);
	m_pIOCPHandler->deleteEvent(lpOverlapped);
	
	if(pSocket->GETSOCKETSTATUS() & SOCK_STATUS_ACCEPTED) pSocket->bindAccept(m_nServerSock);
}


int	CNWCommunicator::getClientCount() { return m_pConnectList->size(); }



int	CNWCommunicator::shutdownPutQueue(CNWSocket *lpCompletion)
{
	if (lpCompletion->socketShutdown()) {
		return m_pIOCPHandler->putRTSQueue(OP_CLOSE, lpCompletion);
	}
	return CERROR_NONE;
}

void CNWCommunicator::regularCloseSocketComplete(CNWSocket *lpCompletion)
{
	StDisconInfo sInfo;
	sInfo.nFlag = DISCONNECT_NORMAL;
	setDisconnectInfo(&sInfo, lpCompletion);
	if(m_pDisconnectFunction) m_pDisconnectFunction(sInfo);
	procAtonceClose(lpCompletion, sInfo);
}

void CNWCommunicator::procAtonceClose(CNWSocket *lpCompletion, StDisconInfo sInfo)
{
	// 가장 먼저 connectionList 에서 삭제
	UINT nFD = (UINT)lpCompletion->GETSOCKET();

	if (lpCompletion->ISSOCKETSTATUS(SOCK_STATUS_ACCEPTED)) {
		if (m_pConnectList->del(lpCompletion)) {
			gs_cLogger.PutLogQueue(LEVEL_TRACE, "FD[%u] delSession nSIdx[%d] ListCount[%d]", nFD, lpCompletion->GETSESSIONIDX(), m_pConnectList->size());
		}
		else {
			gs_cLogger.DebugLog(LEVEL_ERROR, "FD[%u] delSession nSIdx[%d] ListCount[%d]", nFD, lpCompletion->GETSESSIONIDX(), m_pConnectList->size());
		}
	}

	// 사용자 함수 호출
	setDisconnectInfo(&sInfo, lpCompletion);
	if (m_pDisconnectFunction) m_pDisconnectFunction(sInfo);

	lpCompletion->closeSocket();

	// 마지막 : 메모리 반환 (고정 소켓들은 메모리 반환을 하지 않는다.)
	if (lpCompletion->ISSOCKETSTATUS(SOCK_STATUS_ACCEPTED)) g_pSessMgr->delSocket(lpCompletion);
}





void CNWCommunicator::setDisconnectInfo(StDisconInfo *pInfo, CNWSocket *pSocket)
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
	pInfo->pObj = pSocket->getObject();
}



bool CNWCommunicator::IsConnected(int nSIdx)
{
	CNWSocket * pSocket = g_pSessMgr->getUsedSession(nSIdx);
	if (pSocket) {
		return pSocket->Connected();
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx:[%d] Invalid"),nSIdx);
	}

	return false;
}

bool CNWCommunicator::checkConnect(int nSIdx)
{
	CNWSocket * pSocket = g_pSessMgr->getUsedSession(nSIdx);
	if (pSocket) {
		if (pSocket->Connected()) return true;
		if (pSocket->ISSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR)) return false;
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx:[%d] Invalid"), nSIdx);
	}

	return false;
}


bool CNWCommunicator::initConnectToSocket(int * pSevIdx)
{
	CNWSocket * pSocket = g_pSessMgr->newSocket(pSevIdx);
	if (pSocket) {
		return true;
	}
	return false;
}
bool  CNWCommunicator::ConnectToServer(char *pIP, int nPort, int nMiliseconds, int nSevIdx)
{
	CNWSocket * pSocket = g_pSessMgr->getUsedSession(nSevIdx);
	if (pSocket) {
		return pSocket->ConnectTo(pIP, nPort, nMiliseconds, m_pIOCPHandler->getCRTSQueue());
	}
	else {
		_stprintf(g_szMessage, _T("g_pSessMgr->getUsedSession is NULL nIdx[%d]"), nSevIdx);
		comPutError2(g_szMessage, g_szSystemError);
	}

	return false;
}

bool CNWCommunicator::Send(char *pBuf, int nLen, int nSIdx)
{ 
	CNWSocket *pSock = g_pSessMgr->getUsedSession(nSIdx);
	CNWSocket::LPSOCKETDATA pSockData;
	if (pSock) {
		pSockData = g_pSessMgr->newSockData();
		if (pSockData) {
			pSockData->pObj = NULL;
			if (pSock->Send(pBuf, nLen, pSockData) == CERROR_NONE) return true;
		}
		else {
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx[%d] pSockData is NULL"), nSIdx);
		}
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx[%d] pSock is NULL"), nSIdx);
	}
	return false;
}

bool CNWCommunicator::Send2(char *pBuf, int nLen, CLogger *p, int nSIdx)
{
	CNWSocket *pSock = g_pSessMgr->getUsedSession(nSIdx);
	CNWSocket::LPSOCKETDATA pSockData;
	if (pSock) {
		pSockData = g_pSessMgr->newSockData();
		if (pSockData) {
			pSockData->pObj = p;
			if (pSock->Send(pBuf, nLen, pSockData) == CERROR_NONE) return true;
		}
		else {
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx[%d] pSockData is NULL"), nSIdx);
		}
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx[%d] pSock is NULL"), nSIdx);
	}
	return false;
}

void CNWCommunicator::Close(int nSIdx)											
{
	CNWSocket *pSocket = g_pSessMgr->getUsedSession(nSIdx);
	if (pSocket) {
		pSocket->SETSOCKETSTATUS(SOCK_STATUS_CLOSE_NORMAL);
		shutdownPutQueue(pSocket);
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx[%d] pSock is NULL"), nSIdx);
	}
}

void CNWCommunicator::setClientType(int nClientType, int nSIdx)
{
	CNWSocket *pSocket = g_pSessMgr->getUsedSession(nSIdx);
	int nType = nClientType << SOCK_STATUS_SHIFT;
	if (pSocket) {
		pSocket->SETSOCKETSTATUS(nType);
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx[%d] pSock is NULL"), nSIdx);
	}
}


int CNWCommunicator::broadcastToAllSession(char *pBuf, int nLen)
{
	if (!m_pConnectList) return 0;

	CNWSocket *pSock = NULL;
	CNWSocket::LPSOCKETDATA pSockData;
	int nCurIdx = 0, nCount = 0;

	while ((pSock = m_pConnectList->getNext(&nCurIdx)))
	{
		pSockData = g_pSessMgr->newSockData();
		if (!pSockData) {
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSendCount[%d], pSockData is NULL"), nCount);
			return nCount;
		}
		pSock->Send(pBuf, nLen, pSockData);
		nCount++;
		nCurIdx++;
	}
	gs_cLogger.DebugLog(LEVEL_ERROR, "nSendCount[%d]", nCount);
	return nCount;
}

int CNWCommunicator::timeoutProcess()
{
	int nCount = 0;
	CNWSocket *pSock;
	int nCurIdx = 0;
	TCHAR szBuf[4096], szHeader[128];
	int nLen = 0;
	if (!m_pConnectList) return 0;

	nLen = _stprintf(szBuf, " List[nSIdx:nFD ");

	while ((pSock = m_pConnectList->getNext(&nCurIdx)))	{
		if (GETTICKCOUNT() - pSock->GETTIMETICK() > m_nTimeout)	{
			pSock->SETSOCKETSTATUS(SOCK_STATUS_CLOSE_TIMEOUT);
			shutdownPutQueue(pSock);
			nCount++;
			nLen += _stprintf(szBuf + nLen, " %d:%u ", pSock->GETSESSIONIDX(), (UINT)pSock->GETSOCKET());
		}
		nCurIdx++;
	}

	if (nCount) {
		_stprintf(szHeader, "socket close - server timeout List nCount[%d] m_nTimeout[%d] ", nCount, m_nTimeout);
		gs_cLogger.Log(LEVEL_ERROR, szBuf, szHeader, "]");
	}
	return nCount;
}



CNWCommunicatorList *g_pNWCList = NULL;

CNWCommunicatorList::CNWCommunicatorList()
{
	g_pSessMgr = NULL;
	m_pCList = NULL;
	winSocketInit();
}
CNWCommunicatorList::~CNWCommunicatorList()
{
	winSocketClean();
	destroy();
}

void CNWCommunicatorList::destroy()
{
	if (g_pSessMgr) {
		delete g_pSessMgr;
		g_pSessMgr = NULL;
	}
	if (m_pCList) {
		delete m_pCList;
		m_pCList = NULL;
	}
	if (m_pQHandlerList) {
		delete m_pQHandlerList;
		m_pQHandlerList = NULL;
	}
}

bool CNWCommunicatorList::initConfig()
{
	int i, nCount, nThreadCnt, nSessionMax, nIdx;
	TCHAR szKey[36];
	char szSect[16];

	// init SessionManager
	nSessionMax = GetPrivateProfileInt(_T("SESSION"), _T("SESSION_MAX"), 2, g_pSystem);
	GetPrivateProfileString(_T("SESSION"), _T("RECV_BUF_SIZE"), "4k", szKey, sizeof(szKey), g_pSystem);
	g_nRecvBufSize = gs_pMMgr->init(szKey, nSessionMax);
	g_nSendDataCount = GetPrivateProfileInt(_T("SESSION"), "NR_SEND_SOCKETDATA", 3, g_pSystem);
	if (nSessionMax) {
		g_pSessMgr = new (std::nothrow) CSessionManager();
		if (!g_pSessMgr) {
			comErrorPrint(_T("new g_pSessMgr is NULL"));
			goto INIT_CONF_ERROR_PROC;
		}
		g_pSessMgr->createMemoryPool(nSessionMax, g_nSendDataCount);
	}

	// init IOCPHandler
	nCount = GetPrivateProfileInt(_T("IOCP"), _T("COUNT"), 1, g_pSystem);
	m_pQHandlerList = new (std::nothrow) CTList<CNWHandler>();
	if(!m_pQHandlerList) {
		comErrorPrint(_T("new m_pQHandlerList is NULL"));
		goto INIT_CONF_ERROR_PROC;
	}
	if (!m_pQHandlerList->alloc(nCount+ADD_LIST_COUNT)) {
		_stprintf(g_szMessage, "alloc m_pQHandlerList is NULL nCount[%d]", nCount);
		comErrorPrint(g_szMessage);
		goto INIT_CONF_ERROR_PROC;
	}

	for (i = 0; i < nCount; i++)
	{
		sprintf(szSect, "IOCP_%d", i);
		nThreadCnt = GetPrivateProfileInt(szSect, "THR_COUNT", 1, g_pSystem);
		if (!initIocp(nSessionMax, nThreadCnt, i)) {
			_stprintf(g_szMessage, "initIocp has failed nSessionMax[%d] nThreadCnt[%d]", nSessionMax, nThreadCnt);
			comErrorPrint(g_szMessage);
			goto INIT_CONF_ERROR_PROC;
		}
	}
	g_pErrIocpQueue = m_pQHandlerList->getObj(0);

	// init Comunicator
	nCount = GetPrivateProfileInt(_T("COMUNICATOR"), _T("COUNT"), 1, g_pSystem);
	if (nCount) {
		m_pCList = new (std::nothrow) CTList<CNWCommunicator>();
		m_pCList->alloc(nCount+ ADD_LIST_COUNT);
		for (i = 0; i < nCount; i++) {
			nIdx = i;
			if (!initCommunicator(i)) goto INIT_CONF_ERROR_PROC;
		}
	}

	return true;

INIT_CONF_ERROR_PROC:
	destroy();
	return false;
}

bool CNWCommunicatorList::initCommunicator(int *pIdx, int nIocpIdx, int nMax)
{
	CNWCommunicator * pCommunicator;
	CNWHandler *pHandler = m_pQHandlerList->getObj(nIocpIdx);

	if (!pHandler) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("get CNWHandler is NULL idx[%d]"), nIocpIdx);
		goto ERROR_initCommunicator;
	}

	pCommunicator = new (std::nothrow) CNWCommunicator();
	if (!pCommunicator) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("new pCommunicator is NULL"));
		return false;
	}
	if (!pCommunicator->init(nMax, pHandler)) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("pCommunicator->init FAIL nMax[%d]"), nMax); goto ERROR_initCommunicator;
	}
	if (!m_pCList->push_back(pCommunicator, pIdx)) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("m_pCList->push_back FAIL")); goto ERROR_initCommunicator;
	}
	return true;

ERROR_initCommunicator:
	delete pCommunicator;
	return false;
}


// object idx, IocpIdx (ini file idx), client connect list max count
bool CNWCommunicatorList::initCommunicator(int nSeq)
{
	int nIocpIdx, nMax;
	CNWHandler *pHandler;

	CNWCommunicator *pCommunicator = new (std::nothrow) CNWCommunicator();
	if (!pCommunicator) {
		comErrorPrint(_T("new pCommunicator is NULL"));
		return false;
	}

	nIocpIdx = pCommunicator->initConfig(nSeq, &nMax);
	pHandler = m_pQHandlerList->getObj(nIocpIdx);
	if (!pHandler) {
		_stprintf(g_szMessage, _T("get CNWHandler is NULL nIocpIdx[%d]"), nIocpIdx);
		comErrorPrint(g_szMessage);
		goto ERROR_initCommunicator;
	}
	if (!pCommunicator->init(nMax, pHandler)) {
		_stprintf(g_szMessage, _T("pCommunicator->init FAIL nMax[%d]"), nMax);
		comErrorPrint(g_szMessage);
		goto ERROR_initCommunicator;
	}
	if (!m_pCList->setNewObj(nSeq, pCommunicator)) {
		_stprintf(g_szMessage, _T("m_pCList->setNewObj FAIL nSeq[%d]"), nSeq);
		comErrorPrint(g_szMessage);
		goto ERROR_initCommunicator;
	}
	return true;

ERROR_initCommunicator:
	delete pCommunicator;
	return false;
}

// IOCP queue list max count, thread count, object Idx
bool CNWCommunicatorList::initIocp(int nMax, int nThreadCnt, int nSeq)
{
	CNWHandler *pHandler = new (std::nothrow) CNWHandler();
	if (!pHandler) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("new CNWHandler is NULL"));
		return false;
	}
	if (!pHandler->init(nMax, nThreadCnt)) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("pHandler->init FAIL nMax[%d] nThreadCnt[%d]"), nMax, nThreadCnt); goto ERROR_initIocp;
	}
	if (nSeq < 0) {
		if (!m_pQHandlerList->push_back(pHandler)) {
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("m_pQHandlerList->push_back FAIL")); goto ERROR_initIocp;
		}
	}
	else {
		if (!m_pQHandlerList->setNewObj(nSeq, pHandler)) {
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("m_pQHandlerList->setNewObj FAIL nIdx[%d]"), nSeq); goto ERROR_initIocp;
		}
	}

	return true;

ERROR_initIocp :
	delete pHandler;
	return false;
}
