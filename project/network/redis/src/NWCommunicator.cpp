#include "NWCommunicator.h"


CNWCommunicator::CNWCommunicator()
{

}

CNWCommunicator::~CNWCommunicator()
{
}






bool CNWCommunicator::IsConnected(int nSIdx)
{
	return m_cRedis.Connected();
}

bool  CNWCommunicator::ConnectToServer(char *pIP, int nPort, int timeoutMillisecons, int nSevIdx)
{
	struct timeval timeout;
	if (0 < timeoutMillisecons) {
		timeout.tv_sec = timeoutMillisecons / 1000;
		timeout.tv_usec = timeoutMillisecons % 1000;
		timeout.tv_usec = timeout.tv_usec * 1000;
	}
	else {
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
	}
	return m_cRedis.ConnectToTimeout(pIP, nPort, timeout);
}

bool CNWCommunicator::Send(char *pBuf, int nLen, int nSIdx)
{ 
	return m_cRedis.Send(pBuf, nLen);
}

bool CNWCommunicator::Send(int nCount, char **pList, size_t *pSizeList, int nSIdx)
{
	return m_cRedis.Send(nCount, pList, pSizeList);
}

void CNWCommunicator::Close(int nSIdx)
{
	m_cRedis.closeSocket();
}

int CNWCommunicator::getString(char *pCommand, char *pTarget, int nSIdx)
{
	return m_cRedis.getString(pCommand, pTarget);
}


