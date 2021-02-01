#pragma once

enum E_HEADER_LENGTH_TYPE
{
	ePacket_none = 0,
	ePacket_numberLength = 1,
	ePacket_stringLength,
	ePacket_httpLength, // header 의 시작 : HTTP method(GET, POST, HEAD, PUT, DELETE, TRACE, OPTION, CONNECT), header 의 끝 : 빈줄
	ePacket_httpChunked,
};

typedef void(*fp_parsingPacket) (void *pData, void *pDummy, int nVal, int nVal2);
typedef void(*fp_parsingHeader) (void *pData);

// initialize
class CComSocket {
public :
	CComSocket();
	~CComSocket();
	void init(int nIocpIdx);
	void init(fp_parsingPacket function, fp_parsingHeader function1, E_HEADER_LENGTH_TYPE type);
	void initServerSession(int nCount);
	void initClientSession(int nCount);
	bool initServer(char *pIp, int nPort);

	// multi socket control
	bool	IsRecvData(int nSIdx = 0);
	int		Socket(int nSIdx = 0);
	int		RecvDataLen(int nSIdx = 0);
	int		SetOption(int level, int optname, const void *optval, int optlen, int nSIdx = 0);
	int		GetOption(int level, int optname, void *optval, int *optlen, int nSIdx = 0);
	int		GetData(char *szBuf, int nRLen, long ltimeOut = 0, int nSIdx = 0);
	int		PutData(char *szBuf, int nSLen, long ltimeout = 0, int nSIdx = 0);
	void	SetAcceptedSocket(int nSocket, int nSIdx = 0);

	bool	IsConnected(int nSIdx = 0);
	int		GetRecvData(char *pBuf, int nBLen, int nSIdx = 0);
	int		ConnectTo(const char* szIP, int nPort, int nSIdx = 0);
	int		ConnectWithTimeout(int nSIdx = 0, int nTimeout = 0);
	int 	ConnectWithTimeout(char *szIP, int nPort, int nTimeout = 0, int nSIdx = 0);
	int		SendNRecv(int* nLen, char* szBuffer, int nBLen, int nSIdx = 0);
	int		RecvStringData(char* szBuffer, int nBLen, int nSIdx = 0);

	int		SendPacket(char* szBuffer, int nSIdx = 0);
	static  const char *GetHostIP(int nSIdx = 0);
	void	Disconnect(int nSIdx = 0);

private:
	void *m_pObj;
	fp_parsingPacket m_fpParsingProcess;
	fp_parsingHeader m_fpParsingHeader;
};
