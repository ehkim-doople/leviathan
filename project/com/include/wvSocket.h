/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

#ifndef	__WVSOCKET_H__
#define	__WVSOCKET_H__

#include	<sys/types.h>
#ifndef WIN32
 #include	<sys/socket.h>
#endif
#include	"comTypes.h"


#ifndef WIN32
 #ifdef	DEC
  typedef	int	socklen_t;
  #define NODELAY	MSG_NONBLOCK
 #else
  #define NODELAY	MSG_DONTWAIT
 #endif

 #define SOCKETCLOSE(sockfd)		close(sockfd)

#else
 #define NODELAY	0
 typedef int socklen_t;
 #define SOCKETCLOSE(sockfd)		closesocket(sockfd)
#endif

#define	DEF_TIMEOUT		3	



class WVSocket
{
public:
	WVSocket(void);
	WVSocket(const char *szIP, int nPort);//, WVLogger *pLogger=NULL);
	~WVSocket(void);

	bool	IsRecvData();
	SOCKET	Socket()	{ return m_nSocket; }
	int		RevcDataLen()	{ return m_nRecvDLen; }
	int		SetOption(int level, int optname, const void *optval, socklen_t optlen);
	int		GetOption(int level, int optname, void *optval, socklen_t *optlen);
	int		GetData(char *szBuf, int nRLen, long ltimeOut = 0);
	int		PutData(char *szBuf, int nSLen, long ltimeout = 0);
	void	SetAcceptedSocket(int nSocket);

	bool	Connected()	{ return m_bConnected; }
	int		GetRecvData(char *pBuf, int nBLen);
	int		ConnectTo(const char* szIP, int nPort);
	int		SendNRecv(int* nLen, char* szBuffer, int nBLen);
	int		RecvStringData(char* szBuffer, int nBLen);
	
	int		SendPacket(char* szBuffer);
	static  const char *GetHostIP();
	int 	ConnectWithTimeout (char *szIP, int nPort, int nTimeout = 0);
	
	
protected:
	void		onClose();
	char		*socketErrorMsg();

	bool		m_bConnected;
	SOCKET		m_nSocket;
	char		*m_szRecvData;
	int			m_nRecvBLen;
	int			m_nRecvDLen;
	char		m_szIP[30];
	int			m_nPort;
#ifdef WIN32
	char		m_szErrMsg[516];
#endif

private :
};

#endif	//__WVSOCKET_H__
