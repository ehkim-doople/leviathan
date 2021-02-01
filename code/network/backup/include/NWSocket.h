/********************************************************************
	2012.05.03 by KEH
	-------------------------------------------------------------
	@ 클라이언트 소켓 관련 처리를 정의한다.

	@ 절차
	1. createSocket
	2. bindAccept
	3. bindIOCP - accept 된 소켓을 IOCP 에 연결
	4. send / recv
	5. 소켓 종료
*********************************************************************/
#pragma once

#include "RTSQueue.h"



/* socket 의 다양한 정보 */
enum {
	SOCK_STATUS_SERVER		= 0x10000000,	
	SOCK_STATUS_SERVERTYPE	= 0x000000ff,	// 0x000000ff 예약 (타입 상세 정보) - server type
	SOCK_STATUS_CONNECTED	= 0x01000000,
	SOCK_STATUS_DETAIL		= 0x0000ff00,	// 0x0000ff00 예약 (타입 상세 정보) - client type
	SOCK_STATUS_SEND		= 0x00010000,	// send 중
	SOCK_STATUS_RECV		= 0x00020000,	// recv 중
	SOCK_STATUS_WRITEBUF	= 0x00040000,	// write buf 중
	SOCK_STATUS_SENDBUF	    = 0x00080000	// write buf Send 중
};

#define SOCK_STATUS_SHIFT	8
#define MAX_IO_COUNT	    10000
#define NR_BUF              3
#define NR_SEND_SOCKETDATA  3
#define NR_RECV_SOCKETDATA  1



// 커널에서 받아오는 recv Buf
//typedef struct {
//    char szBuf[RECV_BUFFER_SIZE];
//} StRecvBuf;

typedef struct {
	char *pRecvBuf;
	int nAddLen;
	int nBufSize;
} StRemain;

int winSocketInit();
void winSocketClean();

class CNWSocket
{
public:
	typedef struct SOCKETDATA {
		STOVERLAPPED	stCommon;
		CNWSocket	*   lpClient;
		DWORD			nTotLen;		// 총 목적 길이
		DWORD			nCurLen;		// 현재 받은/보낸 길이
		char *			pData;
	} SOCKETDATA, *LPSOCKETDATA;


	CNWSocket(void);       // 사용 안함
	~CNWSocket(void);      // 사용 안함

    void createObject();    // 생성자 대신 사용
    void clear();           // 소멸자 대신 사용

	void initCNWSocket();
	int createSocket();

	/*********************************************************************
	*	클라이언트 accept 대기 -> accept 완료
	**********************************************************************/
	int bindAccept(SOCKET serverSock);
	int doAccept(CRTSQueue *pRTSQueue);
	
	inline void SetConnectionIP(const char *szIP) { memcpy(m_szIP, szIP, MAX_IP_LEN); }
	inline char * GetConnectionIP() { return m_szIP; }

	/*********************************************************************
	*	클라이언트 Connect To Server
	**********************************************************************/

	inline bool	Connected()	{ if(m_sockStatus & SOCK_STATUS_CONNECTED) return true; return false; }
	int	ConnectTo(const char* szIP, int nPort, CRTSQueue *pRTSQueue);
	
	/*********************************************************************
	*	accept 완료후 IO 처리 진행
	**********************************************************************/
	// 
	int bindRecv();
	int RecvPacket();
	int Send(char *pBuf, int nLen, CRTSQueue *pRTSQueue);

    int SendBuf(CRTSQueue *pRTSQueue);
    int WriteToBuffer(const char *pMsg, int nLen, CRTSQueue *pRTSQueue);

	/*********************************************************************
	*	소켓 종료를 위한 절차
	*---------------------------------------------------------------------
		1. 레퍼런스 카운트 체크 - 카운트 존재하면, 강제 종료
		2. PostQueuedCompletionStatus 를 호출하여 ProcessIOCP 큐로 작업을 넘김
		3. ProcessThread 에서 이벤트 감지시 처리
		 - closeSocket() 호출 : 대기시간 필요
		 - 세션 메모리 초기화
		 - 다시 BindAccept 모드 들어감
	**********************************************************************/

	bool socketShutdown();	
	
//	inline BOOL isPossibleDisconnect()	{if(0 < m_lpRecvData->nCount || 0 < m_lpSendData->nCount) return FALSE;	return TRUE;}
//	inline BOOL isPossibleDisconnect()	{if(0 < m_lpRecvData->nCount ) return FALSE;	return TRUE;}
	

	int closeSocket();	// 프로그램 종료시 호출 (closesocket -> create socket -> acceptEx)

	/*********************************************************************/

 
	inline int GETSESSIONIDX()				{ return m_nSIdx; }
	inline void SETSESSIONIDX(int nSIdx)	{ m_nSIdx = nSIdx; }
	inline SOCKET GETSOCKET()				{ return m_socket; }
	
	//inline char *getRecvBuf() { return m_pRecvBuf; }
	//inline int getAddLen() { return m_nAddLen; }
	//inline void setAddLen(int nLen) { m_nAddLen = nLen; }
	inline StRemain * getRemain() { return &m_stRemain; }

	/*
		LPSOCKETDATA type의 메모리 할당은 sessionManager 에서 일괄 할당/셋팅 관리를 책임진다.
	*/
	inline void SETLPRECVDATA(LPSOCKETDATA lpRecvData)          { m_lpRecvData          = lpRecvData; }
    inline void SETLPRECVDATA_BUF(char *pData)                  { m_lpRecvData->pData   = pData; }
	inline LPSOCKETDATA GETRECVDATA()							{ return m_lpRecvData; }
	inline void SETLPSENDDATA(LPSOCKETDATA lpSendData,int j)    { m_lpSendData[j]       = lpSendData; }
	//inline void SETLPSENDDATA_BUF(char *pData,int j)            { m_lpSendData[j]->pData = pData; }

    inline void SETSOCKETSTATUS(int nStatus)	{ m_sockStatus |= nStatus; }	
	inline void DELSOCKETSTATUS(int nStatus)	{ m_sockStatus &= ~nStatus; }	
	inline int  GETSOCKETSTATUS()				{ return m_sockStatus; }	
	inline unsigned int GETTIMETICK()			{ return m_nAliveTick; }
	inline void SETTIMETICK(unsigned int nTime) { m_nAliveTick = nTime; }

	int _SendPacket(LPSOCKETDATA lpSendData, CRTSQueue *pRTSQueue);

private:
	SOCKET	m_socket;
	unsigned int    m_nAliveTick;
	char	m_szAddressBuf[MAX_ADDRESS_LEN]; 
	char	m_szIP[MAX_IP_LEN];			
	int		m_sockStatus;
	int		m_nSIdx;					
	int		m_nPort;					

	LPSOCKETDATA m_lpRecvData;
	LPSOCKETDATA m_lpSendData[NR_SEND_SOCKETDATA];
    //atomic_nr m_nSendIdx;
	int m_nSendIdx;
	//COM_CRITICAL_SECTION	*m_CSWriteBufSocket;
	SPIN_LOCK m_spinLock;


	StRemain m_stRemain;	
	LPSOCKETDATA getSendLPData();

};

