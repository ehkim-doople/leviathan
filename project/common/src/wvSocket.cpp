#ifndef WIN32
 #include	<unistd.h>
 #include	<stdlib.h>
 #include	<fcntl.h>

 #include 	<sys/types.h>
 #include	<sys/socket.h>
 #include	<netinet/in.h>
 #include	<arpa/inet.h>
 #include	<netdb.h>
#else
 #include	<winsock2.h>
#endif

#include	<time.h>
#include	<errno.h>
#include	<string.h>

#include	"wvSocket.h"

#define	SOCK_FLAG	AF_INET

WVSocket::WVSocket(void)
{
	m_bConnected = false;
	m_nSocket = -1;
	m_szRecvData = NULL;
	m_nRecvDLen = 0;
	m_nRecvBLen = 0;
	memset(m_szIP, 0, sizeof(m_szIP));
	m_nPort = -1;

	m_nSocket = ::socket(SOCK_FLAG, SOCK_STREAM, 0);

}

WVSocket::WVSocket(const char *szIP, int nPort)
{
	m_bConnected = false;
	m_nSocket = -1;
	m_szRecvData = NULL;
	m_nRecvDLen = 0;
	m_nRecvBLen = 0;


	memset(m_szIP, 0, sizeof(m_szIP));
	m_nPort = -1;


	ConnectTo(szIP, nPort);
}

WVSocket::~WVSocket(void)
{
	if (m_nSocket > 0) 
		onClose();

	if (m_szRecvData)
		delete [] m_szRecvData;	//20100127

	m_nRecvBLen = m_nRecvDLen = 0;

}


bool	WVSocket::IsRecvData()
{
	struct timeval	tv;
	fd_set	stFds;
	int	nCount;
	int nfds;

	if (m_nSocket < 0)
		return false;

	tv.tv_sec	= 0;
	tv.tv_usec	= 10000;

	FD_ZERO(&stFds);

	FD_SET(m_nSocket, &stFds);
	nfds = (int)m_nSocket;
	nCount =  select (nfds + 1, &stFds, NULL, NULL, &tv);
	if (nCount  == -1) {
		strcpy(m_szErrMsg, "Socket select error");
		onClose();
		return false;
	}

	if (FD_ISSET(m_nSocket, &stFds))
		return true;

	return false;
}

void	WVSocket::onClose()
{
	SOCKETCLOSE(m_nSocket);
	m_nSocket = -1;
	m_bConnected = false;
}

void	WVSocket::SetAcceptedSocket(int nSocket)
{
	if (m_nSocket >= 0)
		SOCKETCLOSE(m_nSocket);
	m_nSocket = nSocket;
	m_bConnected = true;
}
	
int WVSocket::GetRecvData(char *pBuf, int nBLen)
{
	if (m_szRecvData && pBuf) {
		if (nBLen < m_nRecvDLen) {
			c_memcpy(pBuf, m_szRecvData, nBLen);
			return -1;
		}
		c_memcpy(pBuf, m_szRecvData, m_nRecvDLen);
		return 1;
	}
	return 0;
}
	
int	WVSocket::SetOption(int level, int optname, const void *optval, socklen_t optlen)
{
	if (m_nSocket < 0)
		return -1;

#ifdef WIN32
	return setsockopt(m_nSocket, level, optname, (char*)optval, optlen);
#else
	return setsockopt(m_nSocket, level, optname, optval, optlen);
#endif
}

int	WVSocket::GetOption(int level, int optname, void *optval, socklen_t *optlen)
{
	if (m_nSocket < 0)
		return -1;

#ifdef WIN32
	return getsockopt(m_nSocket, level, optname, (char*)optval, optlen);
#elif defined(__ia64)
	return getsockopt(m_nSocket, level, optname, optval, (int *)optlen);
#else
	return getsockopt(m_nSocket, level, optname, optval, optlen);
#endif
}

int WVSocket::ConnectTo(const char* szIP, int nPort)
{
	struct sockaddr_in sa;
	
	strcpy(m_szIP, szIP);
	m_nPort = nPort;
	if (m_nSocket == -1) {
		m_nSocket = ::socket(SOCK_FLAG, SOCK_STREAM, 0);
		if (m_nSocket == -1) {
			strcpy(m_szErrMsg, "WVSocket::ConnectTo Socket create Error");
			return -1;
		}
	}

	memset(&sa, 0x00, sizeof(sa));
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons((short)nPort);
	sa.sin_addr.s_addr = inet_addr(szIP);
	
	int nRet = connect(m_nSocket, (struct sockaddr*)&sa, sizeof(sockaddr_in));
	if(nRet == -1) {
		strcpy(m_szErrMsg, "WVSocket::ConnectTo Socket connect Error");		
		onClose();
		return nRet;
	}
	m_bConnected = true;
	return 1;
}

int WVSocket::SendNRecv(int* nLen, char* szBuffer, int nBLen)
{
	if(m_nSocket < 0)	return -1;

	int nSize = send(m_nSocket, szBuffer, *nLen, NODELAY);
	if(nSize != *nLen) {
		strcpy(m_szErrMsg, "WVSocket::ConnectTo Socket send Error");		
		onClose();
		return -1;
	}
	SLEEP(1);
	*nLen = RecvStringData(szBuffer, nBLen);
	return *nLen < 0 ? -1 : *nLen;
}



int	WVSocket::PutData(char *szBuf, int nLen, long ltimeOut)
{
	time_t	tmIn, tmCur;
	int	sLen, sPos;

	if (m_nSocket < 0)
		return -1;
	
	if (ltimeOut == 0) {
		sLen = send(m_nSocket, szBuf, nLen, 0);
		if (sLen < 0) 
		{
			strcpy(m_szErrMsg, "WVSocket::PutData Socket send Error");	
			onClose();
			return -1;
		}
		return sLen;
	}

	sPos = sLen = 0;
	tmIn = tmCur = time(NULL);
	while (sPos < nLen) {
		sLen = send(m_nSocket, szBuf + sPos, nLen - sPos, NODELAY);
		if (sLen < 0) {
#ifdef WIN32
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
			if (errno == EWOULDBLOCK) {
#endif
				SLEEP(1);
				continue;
			}
			else {
				strcpy(m_szErrMsg, "WVSocket::PutData Socket send Error - 2");	
			}
			onClose();
			return -1;

		}
		else
			sPos += sLen;

		tmCur = time(NULL); // 20090420 rivo

		if (tmCur - tmIn > ltimeOut) {
			strcpy(m_szErrMsg, "WVSocket::PutData send timeout Error");
			return sPos;
		}
	}

	return sPos;
}

int	WVSocket::GetData(char *szBuf, int nLen, long ltimeOut)
{
	int	nRet, nPos = 0;
	time_t	stTm, curTm;

	if (m_nSocket < 0)
		return -1;

	if (ltimeOut)
		stTm = curTm = time(NULL);

	while (1) {
		nRet = recv(m_nSocket, szBuf+nPos, nLen-nPos, NODELAY);
		if (nRet <= 0) {
#ifdef WIN32
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
			if (errno != EWOULDBLOCK) {
#endif
				strcpy(m_szErrMsg, "WVSocket::GetData recv Error");
				onClose();
				return -1;
			}
			SLEEP(1);
		}
		else
			nPos += nRet;
		if (nPos >= nLen)
			break;

		if (ltimeOut) {
			curTm = time(NULL);

			if (curTm - stTm > ltimeOut)
				return nPos;
		}
	}
	return nPos;
}



int WVSocket::RecvStringData(char* szBuffer, int nBLen)
{
	int nSize = 0;
	int nPos = 0;
	int nTmp = 0;
	char	tmpBuf[5];

	nTmp = GetData(tmpBuf, 3, DEF_TIMEOUT);
	nPos += 3;

	if (nTmp == -1) {
		onClose();
		strcpy(m_szErrMsg, "WVSocket::RecvStringData length Reading Down!");
		return -1;
	}
	else if (nTmp == -2) {
		onClose();
		strcpy(m_szErrMsg, "WVSocket::RecvStringData length Reading Timeout!");
		return -1;
	}
	SLEEP(1);
	//패킷 총 길이 계산하기
	nSize = atoi(tmpBuf);
	if (m_nRecvBLen < nPos + nSize + 1) {
		if (m_szRecvData)
			delete [] m_szRecvData;	//20100127
		m_nRecvBLen = nPos+nSize+1;
		m_szRecvData = new char[m_nRecvBLen];
		if (m_szRecvData == NULL)
			return -1;
	}
	memset(m_szRecvData, 0, m_nRecvBLen);
	c_memcpy(m_szRecvData, tmpBuf, nPos);
	
	if((nTmp = GetData(m_szRecvData+nPos, nSize - nPos, DEF_TIMEOUT)) == -1) {
		onClose();
		return -1;
	}
	else if (nTmp == -2) {
		onClose();
		return -1;
	}

	nPos = nSize;
	if (nBLen < nPos) {
		return -2;
	}

	strcpy(szBuffer, m_szRecvData);
	m_nRecvDLen = nPos;
	return nPos;
}

const char *
WVSocket::GetHostIP()
{
	static	char _hostIP[20];
    struct  hostent *hEntry;
	char    szHName[128];

	if (gethostname(szHName, sizeof(szHName)) == 0) {
		hEntry = gethostbyname(szHName);
		if (hEntry && hEntry->h_addr_list && hEntry->h_addr_list[0]) {
			sprintf(_hostIP, "%d.%d.%d.%d", 
				(unsigned char)hEntry->h_addr_list[0][0],
				(unsigned char)hEntry->h_addr_list[0][1],
				(unsigned char)hEntry->h_addr_list[0][2],
				(unsigned char)hEntry->h_addr_list[0][3]);
			return _hostIP;
		}
	}
	return NULL;
}


int WVSocket::ConnectWithTimeout (char *szIP, int nPort, int nTimeout)
{
#ifndef WIN32
	struct sockaddr_in sa;
	struct	timeval tmval;
	int nRet, flags, error;
	fd_set	wset, rset;
	
	strcpy(m_szIP, szIP);
	m_nPort = nPort;
	if (m_nSocket == -1) {
		m_nSocket = ::socket(SOCK_FLAG, SOCK_STREAM, 0);
		if (m_nSocket == -1) {
			return -1;
		}
	}

	memset(&sa, 0x00, sizeof(sa));
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons((short)nPort);
	sa.sin_addr.s_addr = inet_addr(szIP);
	
	if (!nTimeout) {
		if (m_cLogger)
			m_cLogger->LogPrint(LOG_DEBUG, "Normal connect\n");
		nRet = connect(m_nSocket, (struct sockaddr*)&sa, sizeof(sockaddr_in));
		if(nRet == -1) {
			onClose();
			return nRet;
		}
		else {
			m_bConnected = true;
			return 1;
		}
	}


	memset(&tmval, 0, sizeof(tmval));
	tmval.tv_sec = nTimeout;

	flags = fcntl(m_nSocket, F_GETFL, 0);
	fcntl(m_nSocket, F_SETFL, flags | O_NONBLOCK);

	error = 0;

	nRet = connect(m_nSocket, (struct sockaddr*)&sa, sizeof(sockaddr_in));
	if (nRet < 0 && errno != EINPROGRESS) {
		onClose();
		return nRet;
	}

	if (nRet == 0)
		goto done;

	FD_ZERO(&rset);
	FD_SET(m_nSocket, &rset);
	wset = rset;

	if (select(m_nSocket + 1, &rset, &wset, NULL, &tmval) == 0) {
		printf("TImeout Error\n");
		errno = ETIMEDOUT;
		return -1;
	}

	if (FD_ISSET(m_nSocket, &rset) || FD_ISSET(m_nSocket, &wset)) {
		int len = sizeof(error);
#if	defined(__ia64)
		if (getsockopt(m_nSocket, SOL_SOCKET, SO_ERROR, (void*)&error, &len) < 0) {
#else
		if (getsockopt(m_nSocket, SOL_SOCKET, SO_ERROR, (void*)&error, (socklen_t *)&len) < 0) {
#endif
			onClose();
			return -1;
		}
	}
	
done:
	fcntl(m_nSocket, F_SETFL, flags);
	if (error != 0) {
		errno = error;
		onClose();
		return -1;
	}
	
	m_bConnected = true;
#endif
	return 1;
}

