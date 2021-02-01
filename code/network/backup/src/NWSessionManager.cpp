/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/

#include "NWSessionManager.h"
#include "NWGlobal.h"


CSessionManager::CSessionManager()
{
    m_pSessMemMgr   = NULL;
	m_pDataMemMgr	= NULL;
	m_pSockMemMgr	= NULL;
    m_pRecvMemMgr   = NULL;

	m_bCreateMemSession = false;	
	m_pSessionArray = NULL;
    m_pConnectList = NULL;
}


CSessionManager::~CSessionManager(void)
{	
    if(m_pSessMemMgr) {

    	delete m_pSessMemMgr;
        m_pSessMemMgr = NULL;
    }

    if(m_pDataMemMgr) {
        delete m_pDataMemMgr;
        m_pDataMemMgr = NULL;
    }
    if(m_pSockMemMgr) {
	    delete m_pSockMemMgr;
        m_pSockMemMgr = NULL;
    }

    if(m_pRecvMemMgr) {
        delete m_pRecvMemMgr;
        m_pRecvMemMgr = NULL;
    }
    if(m_pSessionArray) {
        free(m_pSessionArray);
        m_pSessionArray = NULL;
    }

    if(m_pConnectList)  {
        delete m_pConnectList;
        m_pConnectList = NULL;
    }

	m_bCreateMemSession = false;
}

void CSessionManager::createMemoryPool(int nMaxCnt, char isMemType, int socketFlag)
{
	// 각각 MAX_SESSION * CLASS SIZE 크기의 메모리가 할당된다.
	// 각각은 MAX_SESSION 개의 포인터가 std::stack 에서 관리된다.
	int i,j;
	int nMax;

	if(m_bCreateMemSession == true) return;   
	
	
	m_pSockMemMgr = new CMemPool<CNWSocket>();
	if(!m_pSockMemMgr->alloc(nMaxCnt)) {
		printf("CSessionManager::createMemoryPool m_pSockMemMgr ERROR! [%s]\n", m_pSockMemMgr->getMessage());
	}

	m_pSessMemMgr = new CBMemPool<CSession>();
	if(!m_pSessMemMgr->alloc(nMaxCnt)) {
		printf("CSessionManager::createMemoryPool m_pSessMemMgr ERROR! [%s]\n", m_pSessMemMgr->getMessage());
	}

	nMax = nMaxCnt*(g_nSendDataCount + NR_RECV_SOCKETDATA);
	m_pDataMemMgr = new CBMemPool<CNWSocket::SOCKETDATA>();
	if (!m_pDataMemMgr->alloc(nMax)) {
		printf("CSessionManager::createMemoryPool m_pDataMemMgr ERROR! [%s]\n", m_pDataMemMgr->getMessage());
	}

	m_pRecvMemMgr = new CSBufPool();
	if(!m_pRecvMemMgr->alloc(g_nRecvBufSize, nMaxCnt)) {
		printf("CSessionManager::createMemoryPool m_pRecvMemMgr ERROR! [%s]\n", m_pRecvMemMgr->getMessage());
	}

    if(isMemType & IS_MEMTYPE_CONNLIST)
    {
        m_pConnectList = new CMemList<CNWSocket>();
		if(!m_pConnectList->alloc(nMaxCnt)) {
			printf("CSessionManager::createMemoryPool m_pConnectList ERROR! [%s]\n", m_pConnectList->getMessage());
		}
    }

   	m_pSessionArray = (CSession **)malloc(sizeof(CSession *) * nMaxCnt);
	if(!m_pSessionArray) {
		printf("CSessionManager::createMemoryPool m_pSessionArray malloc ERROR!\n");
	}

    //=== 메모리 할당 완료 =====

	CNWSocket	*pSock;
	CSession	*pSess;


	/*	
		세션메모리를 연결하여 생성한다.
		세션클래스에는 관련된 객체 포인터를 저장하고,
		세션 클래스를 제외한 각 객체는 세션 인덱스를 갖는다.
	*/
	int nDataIdx=0;
	for(i = 0; i < nMaxCnt; i++)
	{
		pSock = m_pSockMemMgr->getMem(i);		// 미리 할당된 메모리 반환

        //recv data setting
        pSock->SETLPRECVDATA(m_pDataMemMgr->getMem(nDataIdx++));
        pSock->SETLPRECVDATA_BUF((char *)m_pRecvMemMgr->getMem(i));
       
        // send data setting
        for(j=0; j < g_nSendDataCount; j++)
        {
		    pSock->SETLPSENDDATA(m_pDataMemMgr->getMem(nDataIdx++), j);
        }

        // session data setting
        pSess = m_pSessMemMgr->getMem(i);
		pSess->SETSOCKET(pSock);

		pSock->SETSESSIONIDX(i);
		m_pSessionArray[i] = pSess;

		pSock->SETSOCKETSTATUS(socketFlag);

		pSock->createObject();		// 초기화 함수를 호출 -- 생성자에서 호출 할수도 있음

	}
	m_bCreateMemSession = true;
}

int CSessionManager::getRemainCnt(int nType)
{
	switch(nType)
	{
		case MEMTYPE_SESSION	: return m_pSessMemMgr->GETREMAINCOUNT();	
		case MEMTYPE_NWSOCKET	: return m_pSockMemMgr->GETREMAINCOUNT();	 
	}
	return 0;
}



int CSessionManager::getMaxCnt(int nType)
{
	switch(nType)
	{
		case MEMTYPE_SESSION	: return m_pSessMemMgr->GETMAXCOUNT();	
		case MEMTYPE_NWSOCKET	: return m_pSockMemMgr->GETMAXCOUNT();	 
	}
	return 0;
}


/*	BINDACCEPT 를 위한 소켓관련 객체 포인터를 스택에서 pop 한다.*/
CNWSocket * CSessionManager::getNewNWSocket(int nFlag, int idx)
{
    switch(nFlag)
    {
    case POP_WAY_QUEUE  : return m_pSockMemMgr->newMem();
    case POP_WAY_INDEX  : return m_pSockMemMgr->getMem(idx);
    }

    return NULL;

}


/*	소켓 연결 완료 후 처리 */
bool CSessionManager::addSession(CNWSocket *pSocket)
{
    if(!m_pConnectList) {
        printf("addSession ERROR!\n");
        return false;
    }

    //PJNINT nCnt;

    if(m_pConnectList->add(pSocket))
    {
	    gs_cLogger.PutLogQueue(LEVEL_TRACE, "addSession nSIdx[%d] ListCount[%d]", pSocket->GETSESSIONIDX(), m_pConnectList->size());
        return true;
    }
    else {
        gs_cLogger.PutLogQueue(LEVEL_ERROR, "addSession [%s]", m_pConnectList->getMessage());
        return false;
    }
}


/*	소켓 종료후 처리 */
void CSessionManager::delSession(CNWSocket *pSocket)
{
    m_pSessionArray[pSocket->GETSESSIONIDX()]->setObject(NULL);

    if(!m_pConnectList) return;

	// 소켓 연결 리스트에서 멤버 삭제
    if(m_pConnectList->del(pSocket))
    {
        gs_cLogger.PutLogQueue(LEVEL_TRACE, "delSession nSIdx[%d] ListCount[%d]", pSocket->GETSESSIONIDX(), m_pConnectList->size());
        if((pSocket->GETSOCKETSTATUS() & SOCK_STATUS_SERVER)) {
            return;
        }
	    m_pSockMemMgr->delMem(pSocket);
    }
}

bool CSessionManager::isConnected(int nSIdx)
{
	CNWSocket *pSock = m_pSessionArray[nSIdx]->GetSocket();
    return m_pConnectList->isObj(pSock);
}


void CSessionManager::prepareAccept(SOCKET serverSocket, int nFlag)
{
	int i, nTot;
	CNWSocket *pSocket;

	// 반환된 소켓을 확인하여 다시 BindAccept 상태로 만든다.
    // 반환된 소켓을 바로 OS 에서 Accept 하지 않게 하기 위해 간격 50을 둔다.


	nTot = getRemainCnt(MEMTYPE_NWSOCKET);	
    if(nFlag) {
        if( 50 > nTot ) return;
    }

	for(i= 0; i < nTot; i++)
	{
		pSocket = getNewNWSocket(POP_WAY_QUEUE);
        if(pSocket == NULL) return; 
		pSocket->createSocket();
        m_pSessionArray[pSocket->GETSESSIONIDX()]->setObject(NULL);
		if(pSocket->bindAccept(serverSocket) == CERROR_BINDACCEPT)
		{
			gs_cLogger.PutLogQueue(LEVEL_ERROR, "ERR_SOCKET_AcceptEx nSIdx[%d]", pSocket->GETSESSIONIDX());
			pSocket->closeSocket();
			m_pSockMemMgr->delMem(pSocket);
		}
		else {
			gs_cLogger.PutLogQueue(LEVEL_DEBUG, "Client Socket Create & AcceptEx nSIdx[%d]", pSocket->GETSESSIONIDX());
		}
	}
	gs_cLogger.PutLogQueue(LEVEL_FATAL, "BindAccept nTot[%d]", nTot);
}



CNWSocket::SOCKETDATA * CSessionManager::newSockMem()
{
	CNWSocket::SOCKETDATA * pMem = m_pDataMemMgr->newMem();

	if(pMem == NULL ) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, "empty - TYPE:SOCKETDATA nTot[%d] nCur[%d]", m_pDataMemMgr->GETMAXCOUNT(), m_pDataMemMgr->GETREMAINCOUNT());
        return NULL;
	}
	memset(pMem, 0, sizeof(pMem));
	return pMem;
}
/*	연결된 모든 client 들에게 동일한 메시지를 broadcast 한다.*/

int CSessionManager::broadcastToAllSession(char *pBuf, int nLen, CRTSQueue *pRTSQueue)
{
    if(!m_pConnectList) return 0;
    
	CNWSocket *pSock = NULL;

    int nCurIdx = 0,  nCount =0;
    
    while((pSock = m_pConnectList->getNext(&nCurIdx)))
    {

        pSock->Send(pBuf, nLen, pRTSQueue);

        nCount++;
		nCurIdx++;
    }	
	gs_cLogger.PutLogQueue(LEVEL_FATAL, "= broadcastToAllSession nCount[%d]", nCount);
	return nCount;
}

void CSessionManager::sendToAllSession(CRTSQueue *pRTCQueue)
{
    if(!m_pConnectList) return;

	CNWSocket *pSock = NULL;
    int nCurIdx = 0;    
    while((pSock = m_pConnectList->getNext(&nCurIdx)))
    {
        pSock->SendBuf(pRTCQueue);
		nCurIdx++;
    }	
}

int CSessionManager::checkSocketTimetick(unsigned int nCheckTime,  CNWSocket **pDelList)
{
	//int nTime = 0;
	int nCount=0;

    if(!m_pConnectList) return 0;

	CNWSocket *pSock;
    int nCurIdx = 0, nListCnt;

	nListCnt = m_pConnectList->capacity();
	gs_cLogger.PutLogQueue(LEVEL_WARN, "CheckSocketTimetick ConneCnt[%d] ", nListCnt);
    
    while((pSock = m_pConnectList->getNext(&nCurIdx)))
    {
 		if(g_nTick - pSock->GETTIMETICK() > nCheckTime)
		{
			pDelList[nCount] = pSock;
			nCount++;
		}
		nCurIdx++;
        //gs_cLogger.PutLogQueue(LEVEL_WARN, "CheckSocketTimetick idx[%d] listCnt[%d] ", idx, m_pConnectList->getListCount());

    }

    gs_cLogger.PutLogQueue(LEVEL_WARN, "CheckSocketTimetick delete List Count[%d] ", nCount);
	return nCount;
}


/*
	연결된 모든 client 들의 소켓을 종료한다.
	프로그램 종료와 같은 긴급한 상황 아니면 호출되지 않는다.
*/
void CSessionManager::AllClose()
{
	CNWSocket *pSock;

    int nCurIdx = 0, nCnt = 0, nMax;

    if(m_pConnectList)
    {
        int nCount = 0;
        while((pSock = m_pConnectList->getNext(&nCurIdx)))
        {
            if(pSock->closeSocket()) nCnt++;        
			nCurIdx++;
        }
    }
    else if(m_pSessMemMgr)
    {
        nMax = m_pSessMemMgr->GETMAXCOUNT();
        for(nCurIdx = 0; nCurIdx < nMax; nCurIdx++)
        {
            if(m_pSessionArray[nCurIdx]->GetSocket()->closeSocket()) nCurIdx++;
        }
    }

    gs_cLogger.PutLogQueue(LEVEL_TRACE, "AllClose() nCnt[%u]\n",nCnt);
}


