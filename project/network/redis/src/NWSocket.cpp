/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/
#include <hiredis.h>
#include "NWSocket.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

CNWSocket::CNWSocket(void)		
{
    m_sockStatus = 0;
	m_redisContext = NULL;
	//m_socket = INVALID_SOCKET;
	//m_nAliveTick =0 ;
	memset(m_szIP, 0, sizeof(m_szIP));
	m_nPort = 0;


}

CNWSocket::~CNWSocket(void)
{
	closeSocket();
}

// 소멸자 대신 사용- 프로그램 내려가기 전  단 한번 사용됨
void CNWSocket::clear()
{
}

int CNWSocket::createSocket()
{
	return CERROR_NONE;
}
//
//int CNWSocket::SockServerInit(const int nPort, const char* szIP)
//{
//	return 0;
//}

//#######################################################################################################################
    //개별 패킷 send

bool CNWSocket::Send(char *pCommand, int nLen) // nLen : msg Len
{
	bool bRes = true;
	redisReply *reply = (redisReply *)redisCommand(m_redisContext, pCommand);
	if (reply) {
		if (reply->type == REDIS_REPLY_ERROR) {
			gs_cLogger.DebugLog(LEVEL_ERROR, "Error : %s %s", reply->str);
			bRes = false;
		}
		freeReplyObject(reply);
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, "reply is NULL [error:%s]", m_redisContext->errstr);
		bRes = false;
	}
	if (!bRes) closeSocket();
	return bRes;
}


bool CNWSocket::Send(int nCount, char **pList, size_t *pLenList)
{
	bool bRes = true;

	if (!m_redisContext) {
		gs_cLogger.DebugLog(LEVEL_ERROR, "m_redisContext is NULL [IsConnected():%d]", ISSOCKETSTATUS(SOCK_STATUS_CONNECTED));
		return false;
	}

	redisReply *reply = (redisReply *)redisCommandArgv(m_redisContext, nCount, const_cast<const char**>(pList), pLenList);
	if (reply) {
		if (reply->type == REDIS_REPLY_ERROR) {
			gs_cLogger.DebugLog(LEVEL_ERROR, "Error : %s %s", reply->str);
			bRes = false;
		}
		freeReplyObject(reply);
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, "reply is NULL [error:%s]", m_redisContext->errstr);
		bRes = false;
	}
	if (!bRes) closeSocket();
	return bRes;
}

 
int CNWSocket::closeSocket(bool bLog) // 소켓 재생성이 필요함
{
	if (m_redisContext) {
		if (bLog) gs_cLogger.PutLogQueue(LEVEL_INFO, "closeSocket[Ip:%s, port:%d, fd:%d]", m_szIP, m_nPort, m_redisContext->fd);
		redisFree(m_redisContext);
		m_redisContext = NULL;
		memset(m_szIP, 0, sizeof(m_szIP));
		m_nPort = 0;
		DELSOCKETSTATUS(SOCK_STATUS_CONNECTED);
	}
	else {
		gs_cLogger.DebugLog(LEVEL_WARN, "Already closeSocket!");
	}
    return 0;
}

int CNWSocket::getString(char *pCommand, char *pResult)
{
	int nResLen = 0;
	redisReply *reply = (redisReply *)redisCommand(m_redisContext, pCommand);
	if (reply) {
		if (reply->type == REDIS_REPLY_STRING) {
			strcpy(pResult, reply->str);
			nResLen = reply->len;
		}
		else {
			gs_cLogger.DebugLog(LEVEL_ERROR, "pCommand[%s] reply->type is not String", pCommand);
		}
		freeReplyObject(reply);
	}
	else
	{
		gs_cLogger.DebugLog(LEVEL_ERROR, "reply is NULL");
		closeSocket();
	}
	return nResLen;
}

bool CNWSocket::ConnectTo(const char* szIP, int nPort)
{
	// 이미 연결된 상태
	m_sockStatus |= SOCK_STATUS_TOSERVER;

	if (m_sockStatus & SOCK_STATUS_CONNECTED)
	{
		// 목적지 주소가 같으면, return
		if (strcmp(szIP, m_szIP) == 0 && nPort == m_nPort) {
			gs_cLogger.DebugLog(LEVEL_WARN, "Already Connected!");
			return true;
		}
	}

	if (m_redisContext) closeSocket();

	m_redisContext = redisConnect(szIP, nPort);
	if (!m_redisContext) {
		_stprintf(g_szMessage, "m_redisContext->errstr: %s", m_redisContext->errstr);
		SETSOCKETSTATUS(SOCK_STATUS_CREATE_ERROR);
		return false;
	}
	else if (m_redisContext->err) {
		_stprintf(g_szMessage, "m_redisContext->errstr: %s", m_redisContext->errstr);
		closeSocket(false);
		if (!ISSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR)) {
			comPutError(g_szMessage, g_szSystemError);
			SETSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR);
		}
		return false;
	}

	DELSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR);
	strcpy(m_szIP, szIP);
	m_nPort = nPort;
	m_sockStatus |= SOCK_STATUS_CONNECTED;
	gs_cLogger.PutQueue(LEVEL_INFO, "Success!! ConnectTo[Ip:%s, port:%d, fd:%d]", szIP, nPort, m_redisContext->fd);
	return true;
}

bool CNWSocket::ConnectToTimeout(const char* szIP, int nPort, struct timeval timeout)
{
	// 이미 연결된 상태
	m_sockStatus |= SOCK_STATUS_TOSERVER;

	if (m_sockStatus & SOCK_STATUS_CONNECTED)
	{
		// 목적지 주소가 같으면, return
		if (strcmp(szIP, m_szIP) == 0 && nPort == m_nPort) {
			gs_cLogger.DebugLog(LEVEL_WARN, "Already Connected!");
			return true;
		}
	}

	if (m_redisContext) closeSocket();

	m_redisContext = redisConnect(szIP, nPort);
	if (!m_redisContext) {
		_stprintf(g_szMessage, "m_redisContext is NULL");
		if (!ISSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR)) {
			comPutError(g_szMessage, g_szSystemError);
			SETSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR);
		}
		return false;
	}
	else if (m_redisContext->err) {
		_stprintf(g_szMessage, "m_redisContext->errstr: %s", m_redisContext->errstr);
		closeSocket(false);
		if (!ISSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR)) {
			comPutError(g_szMessage, g_szSystemError);
			SETSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR);
		}
		return false;
	}


	DELSOCKETSTATUS(SOCK_STATUS_CONNECTED_ERROR);
	m_sockStatus |= SOCK_STATUS_CONNECTED;
	strcpy(m_szIP, szIP);
	m_nPort = nPort;
	gs_cLogger.PutQueue(LEVEL_INFO, "Success!! ConnectTo[Ip:%s, port:%d, fd:%d]", szIP, nPort, m_redisContext->fd);
	return true;
}