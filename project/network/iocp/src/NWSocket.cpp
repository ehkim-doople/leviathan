/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/
#include "NWSocket.h"
#include "comLogger.h"
#include <mswsock.h>

//-----------------------------------------------------------------------------
// main 에서 한번씩만 호출
int winSocketInit()
{
	WSADATA socket;
	if(WSAStartup(MAKEWORD(2,0), &socket) != 0) //winsock v2.0을 사용하는 socket
		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("WSAStartup"));

		return 0; //실패 시 사용자 정의의 오류코드 반환

	return 1;
}

void winSocketClean()
{
	WSACleanup(); 
}
//-----------------------------------------------------------------------------

CNWSocket::CNWSocket(void)		
{
	m_pObject = 0;
	m_nAliveTick = 0;
	m_nSIdx = 0;				// session index	
	m_sockStatus = 0;
	m_lpRecvData = NULL;
	m_lpSendData = NULL;
	m_socket = INVALID_SOCKET;
	clear();
}

CNWSocket::~CNWSocket(void)
{
	if (m_socket != INVALID_SOCKET) {
		closeSocket();
	}

	if (m_lpRecvData->pData) {
		gs_pMMgr->delBuf(m_lpRecvData->pData, m_lpRecvData->nTotLen);
		m_lpRecvData->pData = NULL;
		m_lpRecvData->nTotLen = 0;
	}

	if (m_lpSendData) {
		if (m_lpSendData->pData) {
			gs_pMMgr->delBuf(m_lpSendData->pData, m_lpSendData->nTotLen);
			m_lpSendData->pData = NULL;
			m_lpSendData->nTotLen = 0;
		}
	}
}

// 소멸자 대신 사용- 프로그램 내려가기 전  단 한번 사용됨
void CNWSocket::clear()
{
	memset(m_szAddressBuf, 0, sizeof(m_szAddressBuf));
	memset(m_szIP, 0, sizeof(m_szIP));
	m_nPort = 0;
	m_nAddLen = 0;
	if (m_lpRecvData) {
		if (m_lpRecvData->pData) {
			gs_pMMgr->delBuf(m_lpRecvData->pData, m_lpRecvData->nTotLen);
			m_lpRecvData->pData = NULL;
			m_lpRecvData->nTotLen = 0;
		}
	}
	if (m_lpSendData) {
		if (m_lpSendData->pData) {
			gs_pMMgr->delBuf(m_lpSendData->pData, m_lpSendData->nTotLen);
			m_lpSendData->pData = NULL;
			m_lpSendData->nTotLen = 0;
		}
	}
}

int CNWSocket::createSocket()
{
	if (m_socket != INVALID_SOCKET) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("m_socket already create! nFD:%u nSIdx:%d msg%s"), (UINT)m_socket, m_nSIdx, err_desc_socket[CERROR_ALREADY_SOCKET]);
		closeSocket();
		return CERROR_ALREADY_SOCKET;
	}

	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(m_socket == INVALID_SOCKET) 
	{
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nSIdx:%d WSAError[%d] msg%s"), m_nSIdx, WSAGetLastError(), err_desc_socket[CERROR_CREATE_SOCKET]);
		return CERROR_CREATE_SOCKET;
	}
	return CERROR_NONE;
}

int CNWSocket::SockServerInit(const int nPort, const char* szIP)
{
	int	so_reuseaddr = 0;
	SOCKADDR_IN		saServer;

	if (createSocket()) return false;


	saServer.sin_family = PF_INET;
	if (szIP == g_sAnyIp) {
		saServer.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else {
		saServer.sin_addr.s_addr = inet_addr(szIP);
	}
	saServer.sin_port = htons((USHORT)nPort);

	m_nPort = nPort;

	if (bind(m_socket, (LPSOCKADDR)&saServer, sizeof(sockaddr)) < 0) {
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("nPort[%d] szIP[%s] nFD[%u] nSIdx:[%d] WSAError[%d] err_desc[%s]"),
				nPort, szIP, (UINT)m_socket, m_nSIdx, WSAGetLastError(), err_desc_socket[CERROR_BIND]);
		// 에러 처리
		return CERROR_BIND;
	}

	setsockopt(m_socket, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char *)&so_reuseaddr, sizeof(so_reuseaddr));

	if (listen(m_socket, 5) == -1) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nPort[%d] szIP[%s] nFD[%u] nSIdx:[%d] WSAError[%d] err_desc[%s]"), nPort, szIP, (UINT)m_socket, m_nSIdx, WSAGetLastError(), err_desc_socket[CERROR_LISTEN]);
		// 에러 처리
		return CERROR_LISTEN;
	}
	m_sockStatus |= SOCKETTYPE_SERVER;
	return CERROR_NONE;
}

/*
WSA_IO_PENDING : 비동기 이므로 차후 수행할 예정이라는 의미.
WSAWOULDBLOCK  : 한번에 처리되지 않고, 지연발생했다는 의미

*/

int CNWSocket::bindAccept(SOCKET serverSock)
{
	WSABUF sWsaBuf;
	sWsaBuf.buf =	m_szAddressBuf; 
	sWsaBuf.len =	0;
	m_lpRecvData->stCommon.opCode = OP_ACCEPT;

	if(AcceptEx(serverSock, m_socket, sWsaBuf.buf, 0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&m_lpRecvData->nCurLen, reinterpret_cast<LPOVERLAPPED>(m_lpRecvData)))
	{
		// nRet == SOCKET_ERROR -1
		if(WSAGetLastError() != WSA_IO_PENDING && WSAGetLastError() != WSAEWOULDBLOCK) 
		{
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("nFD[%lu] nSIdx:[%d] FD[%u] WSAError[%d] err_desc[%s]"), (UINT)m_socket, m_nSIdx, serverSock, WSAGetLastError(), err_desc_socket[CERROR_BINDACCEPT]);
			return CERROR_BINDACCEPT;
		}
	}

	return CERROR_NONE;
}

int CNWSocket::doAccept(CRTSQueue *pRTSQueue)
{
	SOCKADDR		*lpLocalSockAddr = NULL, *lpRemoteSockAddr = NULL;
	int				nLocalSockaddrLen=0, nRemoteSockaddrLen=0;


	m_lpRecvData->stCommon.opCode = 0;
	m_sockStatus |= SOCK_STATUS_CONNECTED;

	// get remote address
	GetAcceptExSockaddrs(m_szAddressBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 
		&lpLocalSockAddr, &nLocalSockaddrLen, &lpRemoteSockAddr, &nRemoteSockaddrLen); 

	if(0 != nRemoteSockaddrLen) {
		SetConnectionIP( inet_ntoa( ((SOCKADDR_IN *)lpRemoteSockAddr)->sin_addr) );
	} else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("nFD[%u] nSIdx:[%d] WSAError[%d] err_desc[%s]"), (UINT)m_socket, m_nSIdx, WSAGetLastError(), err_desc_socket[CERROR_ACCEPT]);
		return CERROR_ACCEPT;
	}

	// bind completion key & connection context
	/*
		1번째 인자 : 붙일 IO 핸들을 의미, 파일디스크립터, 소켓 구조체
		2번째 인자 : IOCP 의 이름
		3번째 인자 : 사용할 객체 포인터, 차후 GetQueuedCompletonStatus 에서 수행중인 작업을 구분하기 위함
		4번째 인자 : 완료된 결과가 들어왔을 경우 쓰레드를 깨워 활성화 시킬 쓰레드의 최대 개수 
		             -> 너무 많이 활성화 되는 것을 막기 위함
					 -> 0 : (CPU 숫자 * 2) + 2 가 자동으로 지정됨
	*/
	if(pRTSQueue->registerSocket((HANDLE)m_socket, this) == CERROR_REGIOCP) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("registerSocket has Failed! nFD[%u] nSIdx:[%d] err_desc[%s]"), m_socket, m_nSIdx);
		return CERROR_REGIOCP;
	}    
    return CERROR_NONE;
}

/*
	recv 대기한다.
*/
int	CNWSocket::bindRecv()  // accept 이후 한번만 호출
{
	if (!m_lpRecvData->pData) {
		m_lpRecvData->pData = gs_pMMgr->newBuf(g_nRecvBufSize);
		m_lpRecvData->nTotLen = g_nRecvBufSize;
	}
	m_nAddLen = 0;
	memset(m_lpRecvData->pData, 0, m_lpRecvData->nTotLen);  // bufSize
	memset(&m_lpRecvData->stCommon, 0, sizeof(STOVERLAPPED));
	return RecvPacket(m_lpRecvData->nTotLen);
}

int CNWSocket::RecvPacket(int nRecvSize)
{
	WSABUF sWsaBuf;
	DWORD nFlags = 0;

	sWsaBuf.buf =	m_lpRecvData->pData + m_nAddLen;
	sWsaBuf.len = nRecvSize;
	m_lpRecvData->nCurLen = 0;

	m_lpRecvData->stCommon.opCode = OP_RECV;

	// WSARecv 가 완료하면 반환값은 0 이다.
	if(WSARecv(m_socket,  &sWsaBuf, 1, &m_lpRecvData->nCurLen, 
		&nFlags, reinterpret_cast<LPWSAOVERLAPPED>(m_lpRecvData), NULL) == SOCKET_ERROR) 
	{
		int nRet = WSAGetLastError();

		if(nRet != WSA_IO_PENDING && nRet != WSAEWOULDBLOCK)  { // 에러발생
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("nFD[%u] nSIdx:[%d] WSAGetLastError[%d] err_desc[%s]"), (UINT)m_socket, m_nSIdx, nRet, err_desc_socket[CERROR_SENDRECV]);
			SETSOCKETSTATUS(SOCK_STATUS_CLOSE_ERROR);
			socketShutdown();
			return CERROR_SENDRECV;
		}
	}
	m_lpRecvData->nCurLen += m_nAddLen;
	return CERROR_NONE;
}

void CNWSocket::changeRecvBuf(int nNewSize)
{
	char *pNewBuf = gs_pMMgr->newBuf(nNewSize);
	memset(pNewBuf, 0, nNewSize);
	memcpy(pNewBuf, m_lpRecvData->pData, m_lpRecvData->nCurLen);
	m_nAddLen = m_lpRecvData->nCurLen;
	gs_pMMgr->delBuf(m_lpRecvData->pData, m_lpRecvData->nTotLen);
	m_lpRecvData->pData = pNewBuf; m_lpRecvData->nTotLen = nNewSize;
}

//#######################################################################################################################
    //개별 패킷 send

int CNWSocket::Send(char *pMsg, int nLen, LPSOCKETDATA lpSendData) // nLen : msg Len
{
	WSABUF sWsaBuf;
	DWORD nFlags = 0;

	sWsaBuf.buf = pMsg;
	sWsaBuf.len = nLen;
	lpSendData->nCurLen = 0;
	lpSendData->stCommon.opCode = OP_SEND;
	lpSendData->pData = pMsg;
	lpSendData->nTotLen = nLen;

	// WSASend 가 완료하면 반환값은 0 이다.    
	int nRes = WSASend(m_socket, &sWsaBuf, 1, &lpSendData->nCurLen,
		nFlags, reinterpret_cast<LPWSAOVERLAPPED>(lpSendData), NULL);
	if (nRes == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING || errorCode == WSAEWOULDBLOCK) {
			gs_cLogger.DebugLog(LEVEL_WARN, "nFD[%u] nSIdx:[%d] WSA_IO_PENDING nTotLen[%d]", (UINT)m_socket, m_nSIdx, sWsaBuf.len);
		}
		else { // 에러 발생
			m_lpSendData = lpSendData;
			g_pErrIocpQueue->putRTSQueue(OP_CLOSE, this);
			const char *pDesc = err_desc_socket[CERROR_SENDRECV];
			gs_cLogger.DebugLog(LEVEL_ERROR, _T("nFD[%u] nSIdx:[%d] curLen[%d] totLen[%d] errorCode[%d] err_desc[%s]"), (UINT)m_socket, m_nSIdx, lpSendData->nCurLen, sWsaBuf.len, errorCode, pDesc);
			return CERROR_SENDRECV;
		}
	}
	//gs_cLogger.PutLogQueue(LEVEL_TRACE, "nSIdx[%d] _SendPacket nTotLen[%d] nCurLen[%d]", m_nSIdx, sWsaBuf.len, lpSendData->nCurLen);
	return CERROR_NONE;
}

bool CNWSocket::socketShutdown()	// gracefull socketclose
{
    if(m_sockStatus & SOCK_STATUS_CONNECTED)
    {
	    shutdown(m_socket, SD_BOTH);
        m_sockStatus &= ~SOCK_STATUS_CONNECTED;     // 추가적인 send 막음
		g_pErrIocpQueue->putRTSQueue(OP_CLOSE, this);
        return true;
    }
    return false;

}


 
int CNWSocket::closeSocket(bool bLog) // 소켓 재생성이 필요함
{
	//struct linger li = {1,0};	// 즉시 종료
//	li.l_onoff	= 1;	// 1:대기여부 결정 0:대기 안함
//	li.l_linger = 0;	// 기다리는 시간을 지정, 0이면 송신버퍼에 남아있는 데이터를 파기
	// SO_LINGER : close() 를 호출했을때, 커널에서 응용 프로그램으로 복귀하는 시점을
	//			  송신 버퍼의 자료가 모두 전송된 것이 확인될 때까지 지연할 수 있다. 

	int nOptValue = 1;
	UINT nFD = (UINT)m_socket;
	if(m_socket !=  INVALID_SOCKET) {
//		setsockopt(m_socket, SOL_SOCKET, SO_LINGER | SO_REUSEADDR, (char *)&li, sizeof(li));
		setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&nOptValue, sizeof(nOptValue));
		closesocket(m_socket);  // IOCP 등록 자동 해제
		m_socket = INVALID_SOCKET;
		DELSOCKETSTATUS(SOCK_STATUS_CONNECTED);
		clear();
		if(bLog) gs_cLogger.PutLogQueue(LEVEL_INFO, "nSIdx[%d] nFD[%u] socket Close complete", m_nSIdx, nFD);
		return 1;
	}	

    return 0;
}



bool CNWSocket::ConnectTo(const char* szIP, int nPort, int miliseconds, CRTSQueue *pRTSQueue)
{
	struct sockaddr_in sa;


	// 이미 연결된 상태
	if(m_sockStatus & SOCK_STATUS_CONNECTED)
	{
		// 목적지 주소가 같으면, return
		if(strcmp(szIP, m_szIP) == 0 && nPort == m_nPort) {
			return true;

		} else { // 목적지 주소가 다르면, 소켓 close
			closeSocket(); 
		}
	}

	if (m_socket == INVALID_SOCKET) 
	{
		if(createSocket() == CERROR_CREATE_SOCKET)
		{
			return false;
		}
	}

	memset(&sa, 0x00, sizeof(sa));
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons((short)nPort);
	sa.sin_addr.s_addr = inet_addr(szIP);

	int nRet;
	u_long iMode = 1;  // 0:blocking, 1:nonBlocking


	nRet = ioctlsocket(m_socket, FIONBIO, &iMode);
	if (nRet != NO_ERROR) {
		_stprintf(g_szMessage, _T("ioctlsocket Fail nRet[%d] nSIdx[%d] FD[%u] [%s:%d]"), nRet, m_nSIdx, (UINT)m_socket, szIP, nPort);
		goto CONNECTTO_ERROR;
	}

	nRet = connect(m_socket, (struct sockaddr*)&sa, sizeof(sockaddr_in));
	if(nRet == -1) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			_stprintf(g_szMessage, _T("connect has failed! nSIdx[%d] FD[%u] [%s:%d]"), m_nSIdx, (UINT)m_socket, szIP, nPort);
			goto CONNECTTO_ERROR;
		}
	}

	fd_set wrset;
	fd_set exset;
	FD_ZERO(&wrset);
	FD_ZERO(&exset);

	FD_SET(m_socket, &wrset);
	FD_SET(m_socket, &exset);

	//wait till connect finishes
	struct timeval tval;

	if (0 < miliseconds) {
		tval.tv_sec = miliseconds/1000;
		tval.tv_usec = miliseconds % 1000;
		tval.tv_usec = tval.tv_usec * 1000;
	}
	else {
		tval.tv_sec = 0;
		tval.tv_usec = 500000;
	}
	nRet = select(0, 0, &wrset, &exset, &tval);


	if (FD_ISSET(m_socket, &wrset))
	{
		m_sockStatus |= SOCK_STATUS_CONNECTED;
		strcpy_s(m_szIP, szIP);
		m_nPort = nPort;

		if (pRTSQueue->registerSocket((HANDLE)m_socket, this) == CERROR_REGIOCP) {
			goto CONNECTTO_ERROR;
		}

		if (bindRecv() == CERROR_NONE) {
			gs_cLogger.PutLogQueue(LEVEL_INFO, "nSIdx[%d] FD[%u] IP[%s] nPort[%d] ConnectTo Complete", m_nSIdx, (UINT)m_socket, szIP, nPort);
			DELSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR);
			return true;
		}
	}
	else {
		_stprintf(g_szMessage, _T("select Fail nRet[%d] nSIdx[%d] FD[%u] [%s:%d]"), nRet, m_nSIdx, (UINT)m_socket, szIP, nPort);
	}

CONNECTTO_ERROR : 
    closeSocket(false);
	if (!ISSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR)) {
		comPutError2(g_szMessage, g_szSystemError);
		SETSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR);
	}
	return false;
}

bool CNWSocket::setOption(int level, int optname, const void *optval, int optlen)
{
	int iResult = setsockopt(m_socket, level, optname, (char *)optval, optlen);
	if (iResult == SOCKET_ERROR) {
		_stprintf(g_szMessage, _T("iResult[%d] nSIdx[%d] FD[%u] error[%d]"), iResult, m_nSIdx, (UINT)m_socket, WSAGetLastError());
		comErrorPrint(g_szMessage);
		return false;
	}
	return true;
}

bool CNWSocket::getOption(int level, int optname, void *optval, int *optlen)
{
	int iResult = getsockopt(m_socket, level, optname, (char *)optval, optlen);
	if (iResult == SOCKET_ERROR) {
		_stprintf(g_szMessage, _T("iResult[%d] nSIdx[%d] FD[%u] error[%d]"), iResult, m_nSIdx, (UINT)m_socket, WSAGetLastError());
		comErrorPrint(g_szMessage);
		return false;
	}
	return true;
}