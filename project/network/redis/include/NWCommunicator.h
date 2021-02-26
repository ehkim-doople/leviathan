/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2021 by KimEunHye												*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

#pragma once

#include "NWSocket.h"
#include "NWSessionManager.h"

class CNWCommunicator
{
public :
	CNWCommunicator();
	~CNWCommunicator();
	bool  ConnectToServer(char *pIP, int nPort, int timeoutMillisecons=0, int nSIdx = 0);
	bool Send(char *pBuf, int nLen, int nSIdx=0);
	bool Send(int nCount, char **pList, size_t *pSizeList,int nSIdx=0);
	void Close(int nSIdx = 0);
	bool IsConnected(int nSIdx=0);
	int getString(char *pCommand, char *pTarget, int nSIdx = 0);
	inline CNWSocket *	getSocket(int nSIdx = 0) { return &m_cRedis; }

private :	
	CNWSocket m_cRedis;
};




