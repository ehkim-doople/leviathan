#include "iocpSocketPool.h"
#include "Communicator.h"



/*
// http:
header 의 시작 : HTTP method(GET, POST, HEAD, PUT, DELETE, TRACE, OPTION, CONNECT), header 의 끝 : 빈줄
Content-Length : <length>
https://developer.mozilla.org/ko/docs/Web/HTTP/Headers/Content-Length
chunked : 16진수 길이 ('\r\n') + chunk(body)
https://developer.mozilla.org/ko/docs/Web/HTTP/Headers/Transfer-Encoding
HTTP/1.1 200 OK
Content-Type: text/plain
Transfer-Encoding: chunked
*/
//-----------------------------------------------------------------------------------
//	DataSize, MsgIndex, body
//-----------------------------------------------------------------------------------
struct stPacketHeader
{
	unsigned int nDataSize;
};

struct STMessageHeader
{
	unsigned int nMsgIndex;
};

struct STComPacket
{
	E_HEADER_LENGTH_TYPE eLength_type;
	unsigned int nHeaderSize;
	unsigned int nBodySize;
	char *pHeader; // STMessageHeader *, char *, NULL
	char *pBody;
};

#define PACKET_HEADER_SIZE          sizeof(stPacketHeader)
#define PACKET_MSGHEADER_SIZE       sizeof(STMessageHeader)

int MakePacketHeader(char *pDest, const char *pMsg, int nMsgLen, int nBufSize);
void DecodePacketHeader(char *pDest, stPacketHeader *stHeader, const char *pPacket);


// recv header
// recv body
// recv process

void MakePacket(STComPacket *p)
{
	switch (p->eLength_type)
	{
	case ePacket_none			: break;
	case ePacket_numberLength	: break;
	case ePacket_stringLength	: break;
	case ePacket_httpLength		: break;
	case ePacket_httpChunked	: break;
	}
}

void makePacket_1(STComPacket *p)
{

}


int MakePacketHeader(char *pDest, const char *pMsg, int nMsgLen, int nBufSize)
{
	int nHeaderSize = PACKET_HEADER_SIZE;
	int nTotSize = nHeaderSize + nMsgLen;
	if (pDest == NULL || NULL == pMsg)
		return 0;

	if (nTotSize > nBufSize) {
		// todo error Log
		return 0;
	}

	stPacketHeader stPacketHeader;
	stPacketHeader.nDataSize = nMsgLen;

	// 헤더 추가 (암호화 포함)
	memcpy(pDest, (char *)&stPacketHeader, nHeaderSize);

	// Message 추가
	memcpy(&pDest[nHeaderSize], pMsg, nMsgLen);

	return nTotSize;
}


void DecodePacketHeader(char *pDest, stPacketHeader *stHeader, const char *pPacket)
{
	// 암호화 decoding  후 msg 만 추출해서 pDest 에 셋팅
	memcpy(pDest, pPacket + PACKET_HEADER_SIZE, stHeader->nDataSize);
	pDest[stHeader->nDataSize] = 0;
}


/*
패킷구조
|	4byte	|	4byte	|	byteData	|
|  length	|	code	|	Data		|
*/
void recvProcess_short(CNWSocket * pSocket, int nRecvBufLen)
{
	int nHeaderSize = PACKET_HEADER_SIZE;
	int  nTotLen = 0, nProcCompleteLen = 0, nPacketLen = 0, nRemainLen;
	CNWSocket::LPSOCKETDATA pSockData = pSocket->GETRECVDATA();
	StRemain * pRemain = pSocket->getRemain();
	stPacketHeader *pHeader;

	char *pNextPath;
	if (0 < pRemain->nAddLen) {
		memcpy(pRemain->pRecvBuf + pRemain->nAddLen, pSockData->pData, pSockData->nCurLen);
		nTotLen = pRemain->nAddLen + pSockData->nCurLen;
		pNextPath = pRemain->pRecvBuf;
	}
	else {
		nTotLen = pSockData->nCurLen;
		pNextPath = pSockData->pData;
	}
	
	// nTotLen =  남아있는 데이터 길이 + 지금 받은 데이터 길이
	// nProcCompleteLen = 처리 완료 길이

	pHeader = (stPacketHeader*)pNextPath; 
	nPacketLen = nHeaderSize + pHeader->nDataSize;

	// 
	while (nTotLen >= nProcCompleteLen + nPacketLen) { // 처리완료되고, 추가로 처리해야할 새 패킷 길이가 받은 데이터 길이보다 작을때...
		nProcCompleteLen += nPacketLen; pNextPath += nHeaderSize;
		STMessageHeader *pMsgHeader = (STMessageHeader*)pNextPath;
		if (pMsgHeader->nMsgIndex == 1) { // param_Alive
			pNextPath += pHeader->nDataSize; continue;
		}
		//----------------------------------------------
		//msgproc_recvData(pMsgHeader->nMsgIndex, pNextPath + PACKET_MSGHEADER_SIZE, pHeader->nDataSize - PACKET_MSGHEADER_SIZE);
		//(this->*m_pMsgProcFuncList[pMsgHeader->nMsgIndex])(pMsgHeader->nMsgIndex, pNextPath+nMHSize, pHeader->nDataSize-nMHSize);
		//----------------------------------------------
		pNextPath += pHeader->nDataSize;
		pHeader = (stPacketHeader*)pNextPath;
		nPacketLen = nHeaderSize + pHeader->nDataSize;
	}

	// 데이터가 처리되고 남은 데이터가 있을때 -- 여기서 pRemain->pRecvBuf 는 처리후 남은 데이터를 보관하여 다음 패킷과 완성형을 만들기 위한 용도로 사용됨.
	if (nProcCompleteLen < nTotLen) {
		pRemain->nAddLen = nRemainLen = nTotLen - nProcCompleteLen;
		if (pRemain->pRecvBuf) {
			if (nProcCompleteLen) {
				if (pRemain->nBufSize < nPacketLen + nRecvBufLen) {
					char *pNewBuf = g_cMMgr.newBuf(nPacketLen + nRecvBufLen);
					g_cMMgr.delBuf(pRemain->pRecvBuf, pRemain->nBufSize);
					pRemain->nBufSize = nPacketLen + nRecvBufLen;
					pRemain->pRecvBuf = pNewBuf;
				}
			}
			else return;
		}
		else {
			pRemain->pRecvBuf = g_cMMgr.newBuf(nPacketLen + nRecvBufLen);
			pRemain->nBufSize = nPacketLen + nRecvBufLen;
		}
		memcpy(pRemain->pRecvBuf, pNextPath, nRemainLen);
	}
	else if (pRemain->pRecvBuf) {
		g_cMMgr.delBuf(pRemain->pRecvBuf, pRemain->nBufSize);
		memset(pRemain, 0, sizeof(StRemain));
	}
}

//-----------------------------------------------------------------------------------
//	DataSize, MsgIndex, body
//-----------------------------------------------------------------------------------




CComSocket::CComSocket()
{
	m_nIocpIdx = -1;
	m_fpParsingProcess = NULL;
}
CComSocket::~CComSocket()
{

}
void CComSocket::init(int nIocpIdx)
{
	CCommunicator * p = g_cIocpCore.getObj(nIocpIdx);
	m_pObj = p;
	p->initConfig(nIocpIdx);
}


void CComSocket::init(fp_parsingPacket function, fp_parsingHeader function1, E_HEADER_LENGTH_TYPE type)
{
	m_fpParsingHeader = function1;
	m_fpParsingProcess = function;
	CCommunicator * p = (CCommunicator *)m_pObj;
	fp_recvProcess recvProc;
	switch (type)
	{
	case ePacket_none: break;
	case ePacket_numberLength: recvProc = recvProcess_short; break;
	case ePacket_stringLength: break;
	case ePacket_httpLength: break;
	case ePacket_httpChunked: break;
	}

	p->init(recvProc);
}

void CComSocket::initServerSession(int nCount)
{

}
void CComSocket::initClientSession(int nCount)
{

}
bool CComSocket::initServer(char *pIp, int nPort)
{

}

// multi socket control
bool	CComSocket::IsRecvData(int nSIdx = 0)
{

}
int		CComSocket::Socket(int nSIdx = 0)
{

}
int		CComSocket::RecvDataLen(int nSIdx = 0)
{

}
int		CComSocket::SetOption(int level, int optname, const void *optval, int optlen, int nSIdx = 0)
{

}
int		CComSocket::GetOption(int level, int optname, void *optval, int *optlen, int nSIdx = 0)
{

}
int		CComSocket::GetData(char *szBuf, int nRLen, long ltimeOut = 0, int nSIdx = 0)
{

}
int		CComSocket::PutData(char *szBuf, int nSLen, long ltimeout = 0, int nSIdx = 0)
{

}
void	CComSocket::SetAcceptedSocket(int nSocket, int nSIdx = 0)
{

}

bool	CComSocket::IsConnected(int nSIdx = 0)
{

}
int		CComSocket::GetRecvData(char *pBuf, int nBLen, int nSIdx = 0)
{

}
int		CComSocket::ConnectTo(const char* szIP, int nPort, int nSIdx = 0)
{

}
int		CComSocket::ConnectWithTimeout(int nSIdx = 0, int nTimeout = 0)
{

}
int 	CComSocket::ConnectWithTimeout(char *szIP, int nPort, int nTimeout = 0, int nSIdx = 0)
{

}
int		CComSocket::SendNRecv(int* nLen, char* szBuffer, int nBLen, int nSIdx = 0)
{

}
int		CComSocket::RecvStringData(char* szBuffer, int nBLen, int nSIdx = 0)
{

}

int		CComSocket::SendPacket(char* szBuffer, int nSIdx = 0)
{

}
const char *CComSocket::GetHostIP(int nSIdx = 0)
{

}
void	CComSocket::Disconnect(int nSIdx = 0)
{

}


	//int m_nIocpIdx;
	//fp_recvProcess fpRecvProcess;

