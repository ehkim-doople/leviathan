/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2021 by KimEunHye												*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

/********************************************************************
	2012.05.03 by KEH
	-------------------------------------------------------------
	@ 클라이언트 소켓 관련 처리를 정의한다.


*********************************************************************/
#pragma once
//#include <hiredis.h>
#include "comLogger.h"

struct redisContext;

#define CERROR_NONE 0
#define CERROR_ALREADY_SOCKET -1
#define CERROR_CREATE_SOCKET -2


#define MAX_IP_LEN 24

/* socket 의 다양한 정보 */
enum {
	SOCK_STATUS_SERVER			= 0x01000000,	// Listen socket
	SOCK_STATUS_TOSERVER		= 0x02000000,	// socket of connect to other server 
	SOCK_STATUS_ACCEPTED		= 0x04000000,	// accepted client socket
	SOCK_STATUS_SERVERTYPE		= 0x000000ff,	// 0x000000ff 예약 (타입 상세 정보) - server type , ex:A_Server, B_Server, C_Server type number
	SOCK_STATUS_CONNECTED		= 0x00100000,
	SOCK_STATUS_CREATE_ERROR	= 0x00200000,
	SOCK_STATUS_CONNECTED_ERROR = 0x00400000,
	SOCK_STATUS_DETAIL			= 0x0000ff00,	// 0x0000ff00 예약 (타입 상세 정보) - client type (bind type )
	SOCK_STATUS_SEND			= 0x00010000,	// send 중
	SOCK_STATUS_RECV			= 0x00020000,	// recv 중
	SOCK_STATUS_CLOSE_TIMEOUT	= 0x00040000,	
	SOCK_STATUS_CLOSE_NORMAL	= 0x00080000,
	SOCK_STATUS_CLOSE_ERROR		= 0x00100000	
};

class CNWSocket
{
public:


	CNWSocket(void);       // 사용 안함
	~CNWSocket(void);      // 사용 안함

    void clear();           // 소멸자 대신 사용

	int createSocket();


	/*********************************************************************
	*	클라이언트 Connect To Server
	**********************************************************************/

	inline bool	Connected()	{ if(m_sockStatus & SOCK_STATUS_CONNECTED) return true; return false; }
	bool ConnectTo(const char* szIP, int nPort);
	bool ConnectToTimeout(const char* szIP, int nPort, struct timeval timeout);

	bool Send(char *pBuf, int nLen);
	bool Send(int nCount, char **pList, size_t *pLenList);

	int getString(char *pCommand, char *pResult);

	bool socketShutdown();	
	int closeSocket(bool bLog=true);	// 프로그램 종료시 호출 (closesocket -> create socket -> acceptEx)

	/*********************************************************************/
	inline void SETSOCKETSTATUS(int nStatus) { m_sockStatus |= nStatus; }
	inline void DELSOCKETSTATUS(int nStatus) { m_sockStatus &= ~nStatus; }
	inline bool ISSOCKETSTATUS(int nStatus) { return m_sockStatus & nStatus ? true : false; }
	inline int  GETSOCKETSTATUS() { return m_sockStatus; }


private:
	redisContext *	m_redisContext;
	char	m_szIP[MAX_IP_LEN];			
	int		m_sockStatus;
	int		m_nPort;
};

