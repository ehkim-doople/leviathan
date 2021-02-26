#include "../DpLib/CmProcess.h"


#ifndef WIN32
 #include	<sys/time.h>
 #include	<pthread.h>
 #include	<sys/socket.h>
#endif

#include <sys/types.h>
#include "comEnv.h"
#include "comProfileutil.h"
#include "comDateTime.h"

#include "resourceProc.h"


enum EData_Type
{
	param_Alive = 100,
	param_CPU_avg_Usage,
	param_HDD_Usage,
	param_MemoryInfo,
	param_ResourceInfo,
	param_NET_Info,
};

enum ENet_Info_Size
{
	eNet_InterfaceCount_Size = 4,
	eNet_InterfaceName_Size = 24,
	eNet_Bytes_Size = 8
};


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

#define MAX_PACKET_LEN 4096
#define MAX_SESSION_TIMEOUT 20


#pragma pack ( push, 1 )
#define MAX_SIMPLE_DATA 20
 struct STSimpleData
 {
	 int nValue;
	 char szTemp[MAX_SIMPLE_DATA];
 };

 struct STResource
 {
	 int nCpu;
	 int nStorage;
	 int nMemory;
	 char szTemp[MAX_SIMPLE_DATA];
 };

 struct stPacketHeader
 {
	 unsigned int nDataSize;
 };
 struct STMessageHeader
 {
	 unsigned int nMsgIndex;
 };

#pragma pack (pop)

#define PACKET_HEADER_SIZE          sizeof(stPacketHeader)
#define PACKET_MSGHEADER_SIZE       sizeof(STMessageHeader)


CResourceProc g_cResourceProc; //20161123 keh add


int g_nLogSeq;
void dayChangeProc(int nDate)
{
	if(!g_cResourceProc.getLogSeq()) return;

	if(g_nLogSeq != g_cResourceProc.getLogSeq())
	{
		TCHAR szConfig[256];
		_tcscpy(szConfig, CEnv::WorkingDir());
		int nDirLen = _tcslen(CEnv::WorkingDir());
		_tcscpy(&szConfig[nDirLen], _T("dpres.ini"));
		PutPrivateProfileInt(_T("LOGINFO"), _T("SEQ"), g_cResourceProc.getLogSeq(), szConfig);
		SavePrivateProfile();
		confEntryDestroy();
	}
}

CResourceProc::CResourceProc()
{
	m_nListenPort = 0;
	m_nSLastIdx	  = 0;
	memset(m_stSessionArray, 0, sizeof(m_stSessionArray));
	m_bAlive = true;
	m_nListenPort = 19000;
	memset(m_nThreadID, 0, sizeof(m_nThreadID));
	m_nSessionUseCnt.init();
	m_pEqMLogger = NULL;

	m_nStorageUsage = 0;
	nCpusAverageUsage_=0;
	nMemUsage_=0;
	ulTotalMemory_ = 0;
	ulUsingMemory_=0;
	m_nInterfaceCount = 0;
	m_nNetIOTot = 0;
	m_nWriteToLog = 0;
}


CResourceProc::~CResourceProc()
{
	Quit();
}

void CResourceProc::Quit()
{
	int i;

	m_bAlive = false;

	// 메모리 제거
	for(i = 0; i < m_nSLastIdx; i++)
	{
		if(m_stSessionArray[i].nUse) {
			SOCKETCLOSE(m_stSessionArray[i].nSD);
			m_stSessionArray[i].nUse = NOT_USE;
		}
	}

	dayChangeProc(0);

	if(m_pEqMLogger) {
		delete m_pEqMLogger;
		m_pEqMLogger = NULL;
	}
}

bool CResourceProc::Init()
{
	char szDir[256], szName[128];
	char szConfig[256];
	bool res=true;
	CEnv::initWorkingPath(NULL);
	strcpy(szConfig, CEnv::WorkingDir());
	int nDirLen = strlen(szConfig);
	strcpy(&szConfig[nDirLen], "dpres.ini");

	m_nListenPort = GetPrivateProfileInt("SERVERINFO", "PORT", 20010, szConfig);
	int LogEnable = GetPrivateProfileInt("LOGINFO", "ISENABLE", 0, szConfig);
	m_pEqMLogger = new (std::nothrow) CSTLogger(LogEnable);
	if(!m_pEqMLogger) {
		printf("CResourceProc::Init new m_pEqMLogger error\n");
		res = false;
		goto END_PROC;
	}
	if(LogEnable) {
		int nLogLevel = GetPrivateProfileInt(_T("LOGINFO"), _T("LOGLEVEL"), 0, szConfig);
		GetPrivateProfileString(_T("LOGINFO"), _T("LOGDIR"), "", szDir, sizeof(szDir), szConfig);
		GetPrivateProfileString(_T("LOGINFO"), _T("LOGNAME"), _T("monitor"), szName, sizeof(szName), szConfig);
		m_nWriteToLog = GetPrivateProfileInt(_T("LOGINFO"), _T("WRITE_RESOURCE"), 0, szConfig);
		g_nLogSeq = GetPrivateProfileInt(_T("LOGINFO"), _T("SEQ"), 1, szConfig);
		m_pEqMLogger->setSeq(g_nLogSeq);
		m_pEqMLogger->setPFDayChange(dayChangeProc);

		if(!strlen(szDir)) {
			_tcscpy(szDir, CEnv::WorkingDir());
			nDirLen = strlen(szDir);
			strcpy(&szDir[nDirLen], "Log/moniLog");
		}
		if(!strlen(szName)) {
			_tcscpy(szName, _T("monitor"));
		}

		if(!m_pEqMLogger->initMemory(20)) {
			printf("CResourceProc::Init m_pEqMLogger initMemory error\n");
			res = false;
			goto END_PROC;
		}

		m_pEqMLogger->clearFlag(IS_ACTIVE_DAYCHANGE_LOG);
		m_pEqMLogger->initCSLogger(szDir, szName, nLogLevel);

		if(!m_pEqMLogger->Start()) {
			printf("CResourceProc::Init m_pEqMLogger->Start() error\n");
			res = false;
			goto END_PROC;
		}
	}

	printf("m_nListenPort[%d]\n",m_nListenPort);

END_PROC :
#ifndef WIN32
	//confEntryDestroy();
#endif


	return res;
}

void CResourceProc::checkSessionTime()
{
	int	i;
	time_t curTm = time(NULL);
	for (i = 0; i < m_nSLastIdx; i++) {
		if (m_stSessionArray[i].nUse != USING)
			continue;

		if((curTm - m_stSessionArray[i].tmRequest) > MAX_SESSION_TIMEOUT) {
			delSession(i);
		}
	}
}

int	 CResourceProc::SetSDValues(fd_set *stFds)
{
	int	i, nMax = -1;

	FD_ZERO(stFds);	

	for (i = 0; i < m_nSLastIdx; i++) {
		if (m_stSessionArray[i].nUse != USING)
			continue;

		if (nMax <= m_stSessionArray[i].nSD)
			nMax = m_stSessionArray[i].nSD;

		FD_SET(m_stSessionArray[i].nSD, stFds);
	}

	return nMax;
}


bool CResourceProc::Start()
{
	THREAD_ID	pthid;

	if (!THREAD_CREATE(&pthid, CResourceProc::ListenThread, this)) {
		return false;
	}
	m_nThreadID[eEqMT_Listen] = pthid;

	if (!THREAD_CREATE(&pthid, CResourceProc::ReceiveThread, this)) {
		return false;
	}
	m_nThreadID[eEqMT_Receive] = pthid;

	if (!THREAD_CREATE(&pthid, CResourceProc::MonitorThread, this)) {
		return false;
	}
	m_nThreadID[eEqMT_Monitor] = pthid;

	m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("CResourceProc::Start() SUCCESS"));
	gs_pLogger = m_pEqMLogger;
	gs_pLogger->PutLogQueue(LOG_TRACE, _T("gs_pLogger is Write"));	return true;
}



int	 CResourceProc::addSession(SOCKET sd, void *pData)
{
	int	idx = -1, i;

	for(i = 0; i < MAX_SESSION_EQM; i++)
	{
		if(m_stSessionArray[i].nUse == NOT_USE)
		{
			idx = i;
			break;
		}
	}

	if(-1 < idx) {
		m_stSessionArray[idx].nSD		= sd;
		m_stSessionArray[idx].tmRequest	= time(NULL);
		m_stSessionArray[idx].bReport = false;
		m_nSessionUseCnt.atomic_increment();
		m_stSessionArray[idx].nUse		= USING;
		if(idx >= m_nSLastIdx) m_nSLastIdx = idx+1;
		m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("addSession :: nSIdx[%d] nSD[%d] lastCount[%d] useCnt[%d]"),
			idx, m_stSessionArray[idx].nSD, m_nSLastIdx, m_nSessionUseCnt.getCount());

		checkSessionTime();
	} 
	else {
		m_pEqMLogger->PutLogQueue(LOG_ERROR, _T("addSession :: maxSession[%d] OVER! nSD[%d]"),MAX_SESSION_EQM, sd);
	}

	return idx;
}

int	 CResourceProc::delSession(int idx)
{
	if(m_stSessionArray[idx].nUse == NOT_USE) return 0;

	m_stSessionArray[idx].nUse = NOT_USE;

	if( -1 < m_stSessionArray[idx].nSD ) {
		m_stSessionArray[idx].bReport = false;
		SOCKETCLOSE(m_stSessionArray[idx].nSD);
		m_stSessionArray[idx].nSD = -1;
		m_nSessionUseCnt.atomic_decrement();
		m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("delSession :: nSIdx[%d] nSD[%d] useCnt[%d]"),
			idx, m_stSessionArray[idx].nSD, m_nSessionUseCnt.getCount());
		return 1;
	}

	return 0;
	
}

THREAD_RET_TYPE  THREAD_CALLING_CONVENTION
CResourceProc::ListenThread(void *ptr)
{
	CResourceProc *pBasic = (CResourceProc *)ptr;
	struct sockaddr_in stClient, stServer;

	SOCKET		nServerSocketFd = 0, nClientSocketFd = 0;
	socklen_t	nClientLen		= 0;
	int			so_reuseaddr	= 1;
	int			nCIdx = 0;

	pBasic->m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("=========== Start CResourceProc ListenThread =========="));


	/* create socket */
	nServerSocketFd = socket(PF_INET, SOCK_STREAM, 0);	
	
	if(nServerSocketFd < 0) 
	{
		pBasic->m_pEqMLogger->PutLogQueue(LOG_ERROR, _T("Cannot open socket"));
		return 0;
	}
	
	/* bind server port */
	stServer.sin_family = AF_INET;
	stServer.sin_addr.s_addr = htonl(INADDR_ANY);
	stServer.sin_port = htons(pBasic->GetListenPort());

	setsockopt(nServerSocketFd, SOL_SOCKET, SO_REUSEADDR | SO_LINGER, SOCKETOPTDATA&so_reuseaddr, sizeof(so_reuseaddr));

	if (bind (nServerSocketFd, (struct sockaddr *) &stServer, sizeof(stServer)) < 0) 
	{
		pBasic->m_pEqMLogger->PutLogQueue(LOG_ERROR, _T("Cannot bind port[%s]"), strerror(errno));
		return 0;
	}

	if (listen(nServerSocketFd, 5) == -1) 
	{
		pBasic->m_pEqMLogger->PutLogQueue(LOG_ERROR, _T("Listen Error [%s]"), strerror(errno));
		return 0;
	}

	pBasic->m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("waiting for Connect on port TCP %d"), pBasic->GetListenPort());


	while(pBasic->m_bAlive) 
	{
		nClientLen = sizeof(stClient);
		
		nClientSocketFd = accept(nServerSocketFd, (struct sockaddr*)&stClient, &nClientLen);
		
		if (nClientSocketFd < 0) {
			pBasic->m_pEqMLogger->PutLogQueue(LOG_ERROR, _T("Cannot accept connection "));
			continue;
		}

		setsockopt(nClientSocketFd, SOL_SOCKET, SO_REUSEADDR | SO_LINGER, SOCKETOPTDATA&so_reuseaddr, sizeof(so_reuseaddr));

		// one client  당  세션 추가 
		
		nCIdx = pBasic->addSession(nClientSocketFd, NULL);
		if (nCIdx < 0) {
			pBasic->m_pEqMLogger->PutLogQueue(LOG_ERROR, _T("Too Many Client"));
			SOCKETCLOSE(nClientSocketFd);
			continue;
		}

		pBasic->m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("=========== Client Connected [%d:%d] ==========="), nCIdx, nClientSocketFd);
	}

	return 0;
}

THREAD_RET_TYPE  THREAD_CALLING_CONVENTION
CResourceProc::ReceiveThread(void *ptr)
{
	CResourceProc *pBasic = (CResourceProc *)ptr;
	
	int	nLast;
	int nRet = 0;
	struct timeval tv;
	int nFdMax = 0;
	int nConnCnt = 0;
	int i = 0;
	fd_set          stFds;

	pBasic->m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("=========== Start CResourceProc Receive Thread ==========="));	

	while(pBasic->m_bAlive) {
		if(pBasic->LastSession() <= 0) {
			SLEEP(1);
			continue;
		}

		nFdMax = pBasic->SetSDValues(&stFds);
		if (nFdMax <= 0) {
			SLEEP(1);
			continue;
		}

		nLast = pBasic->LastSession();

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		nConnCnt = select(nFdMax + 1, &stFds, NULL, NULL, &tv);
		//nConnCnt = select(nFdMax + 1, &stFds, NULL, NULL, NULL);

		// -1 : 실패
		// 0  : tv 만큼 기다려도 반응없음
		// 1이상 : 파일기술자 모음의 전체 개수

		if(nConnCnt <= 0)  
			continue;

		for (i = 0; i < nLast && nConnCnt; i++) 
		{
			if (pBasic->m_stSessionArray[i].nSD < 0)
				continue;

			nRet = FD_ISSET(pBasic->m_stSessionArray[i].nSD, &stFds);
			if (!nRet)
				continue;

			nConnCnt --;

			pBasic->RecvProcess(i);			
        }
	}
	return 0;
}

int	CResourceProc::recvData(SOCKET FD, char *szBuf, int nLen, long ltimeOut)
{
	int	nRet, nPos = 0;
	time_t	stTm, curTm;

	if (FD < 0) {
		//printf("recvData recv ERROR\n");
		m_pEqMLogger->PutLogQueue(LOG_DEBUG,_T("recvData FD is -1"));
		return -1;
	}

	if (ltimeOut)
		stTm = curTm = time(NULL);

	while (1) {
		nRet = recv(FD, szBuf+nPos, nLen-nPos, NODELAY);
		if (nRet <= 0) {
#ifdef WIN32
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
			if (errno != EWOULDBLOCK) {
#endif
				m_pEqMLogger->PutLogQueue(LOG_DEBUG,_T("recvData ERROR FD[%d] errno[%d] str[%s]"), FD,  errno, strerror(errno));
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

			if (curTm - stTm > ltimeOut) {
				m_pEqMLogger->PutLogQueue(LOG_DEBUG,_T("recvData time nRet[%d] out[%ld] curTm[%ld] stTm[%ld]"), nRet,ltimeOut, curTm, stTm);
				return nPos;
				//return 0;
			}
		}
	}
	m_pEqMLogger->DoWritePacket(-1, RCV_SOCKET, FD, szBuf, nPos);
	return nPos;
}



THREAD_RET_TYPE  THREAD_CALLING_CONVENTION
CResourceProc::MonitorThread(void *ptr)
{
	CResourceProc *pBasic = (CResourceProc *)ptr;
	int nCount = 0;
	pBasic->m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("=========== Start CResourceProc monitor Thread ==========="));

	pBasic->setInterCount(INIT_INTERFACE());
	while (pBasic->m_bAlive) {
		SET_NET1();
		SLEEP(1);
		SET_NET2();
		nCount++;
		pBasic->MakeCPUPerformanceStateNL();
		pBasic->setMemoryInfo();

		if (pBasic->m_nWriteToLog && nCount == 3) {
			nCount = 0;
			pBasic->setNetIo();
			pBasic->m_pEqMLogger->PutLogQueue(LOG_INFO, "Usage : CPU[%d] Mem[%f] Storage[%d] NetIO[%d]bytes",
				pBasic->nCpusAverageUsage_, pBasic->nMemUsage_, pBasic->m_nStorageUsage, pBasic->m_nNetIOTot);
		}
	}
	return 0;
}

int	CResourceProc::sendData(int nSIdx, char *szBuf, int nLen, long ltimeOut)
{
	time_t	tmIn, tmCur;
	int	sLen, nPos;
	SOCKET FD = m_stSessionArray[nSIdx].nSD;

	if (FD < 0)
		return -1;
	
	if (ltimeOut == 0) {
		sLen = send(FD, szBuf, nLen, 0);
		if (sLen < 0) {
			m_pEqMLogger->PutLogQueue(LOG_DEBUG,_T("sendData - 1 ERROR FD[%d] errno[%d] str[%s]"), FD,  errno, strerror(errno));
			return -1;
		}
		return sLen;
	}

	nPos = sLen = 0;
	tmIn = tmCur = time(NULL);
	while (nPos < nLen) {
		sLen = send(FD, szBuf + nPos, nLen - nPos, NODELAY);
		if (sLen < 0) {
#ifdef WIN32
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
			if (errno == EWOULDBLOCK) {
#endif
				SLEEP(1);
				continue;
			}
			m_pEqMLogger->PutLogQueue(LOG_DEBUG,_T("sendData - 2 ERROR FD[%d] errno[%d] str[%s]"), FD,  errno, strerror(errno));
			return -1;

		}
		else
			nPos += sLen;

		tmCur = time(NULL); 

		if (tmCur - tmIn > ltimeOut) {
			m_pEqMLogger->PutLogQueue(LOG_DEBUG,_T("sendData time out[%ld] curTm[%ld] tmIn[%ld]"), ltimeOut, tmCur, tmIn);
			return -1;
		}
	}
	m_pEqMLogger->PutLogQueue(LOG_TRACE,_T("sendData nSIdx[%d] nLen[%d]"), nSIdx, nLen);
	m_pEqMLogger->DoWritePacket(-1, SND_SOCKET, FD, szBuf, nPos);
	return nPos;
}

void CResourceProc::RecvProcess(int nSidx)
{
   //- recv contents-length -----------------------------------------
   int nResult;
   int nHeaderSize = PACKET_HEADER_SIZE;

	// recv packet length
	char szBuf[MAX_PACKET_LEN << 1];  // 8192
	stPacketHeader *pHeader     = (stPacketHeader*)szBuf;
	STMessageHeader *pMsgHeader = (STMessageHeader*)&szBuf[nHeaderSize];

	nResult = recvData(m_stSessionArray[nSidx].nSD, szBuf, nHeaderSize, DEF_TIMEOUT);
	if (nResult == -1)
	{
		m_pEqMLogger->PutLogQueue(LOG_DEBUG,_T("CResourceProc::RecvProcess header recv error SOCKET[%d]"), m_stSessionArray[nSidx].nSD);    
		delSession(nSidx);
		return;
	}
   if(!nResult) return;
   //- recv contents ------------------------------------------------
   nResult += recvData(m_stSessionArray[nSidx].nSD, szBuf+nResult, pHeader->nDataSize, DEF_TIMEOUT);
   if (nResult == -1)
   {
		m_pEqMLogger->PutLogQueue(LOG_DEBUG,_T("CResourceProc::RecvProcess body recv error SOCKET[%d]"), m_stSessionArray[nSidx].nSD);    
		delSession(nSidx);
		return;
   }

   if(!nResult) return;

   szBuf[nResult] = 0;
   m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("RecvProcess RECV FROM CLIENT : SD[%d] nMsgIndex[%d]"), m_stSessionArray[nSidx].nSD, pMsgHeader->nMsgIndex);
	switch(pMsgHeader->nMsgIndex)
	{	
	case param_Alive : break;
	case param_CPU_avg_Usage : 
	case param_HDD_Usage : 
	case param_MemoryInfo :
		SendSimpleData(nSidx, pMsgHeader->nMsgIndex);
		break;
	case param_ResourceInfo :
		SendResource(nSidx, pMsgHeader->nMsgIndex);
		break;
	case param_NET_Info :
		sendNetInfo(nSidx, pMsgHeader->nMsgIndex);
		break;
	}

	m_stSessionArray[nSidx].tmRequest	= time(NULL);
}


void CResourceProc::sendAllSession(char *pBuf, int nLen)
{
	int	idx = -1, nResult;

	for(idx = 0; idx < m_nSLastIdx; idx ++)
	{
		if(m_stSessionArray[idx].nUse == USING) {
			nResult = sendData(idx, pBuf, nLen, DEF_TIMEOUT);
		   m_pEqMLogger->PutLogQueue(LOG_TRACE, _T("nSIdx[%d] nSD[%d] szData[%s]"),idx, m_stSessionArray[idx].nSD, &pBuf[8]);  // test 용
		   if (nResult == -1)
		   {
				m_pEqMLogger->PutLogQueue(LOG_ERROR,_T("CResourceProc::sendAllSession send error SOCKET[%d]"), m_stSessionArray[idx].nSD);    
				delSession(idx);
		   }
		}
	}
}

void CResourceProc::SendMakePacket(int nMsgIdx, const char *pMsg, int nMsgLen, int nSIdx) 
{
	int nHeaderSize = PACKET_HEADER_SIZE;
	int nMHSize = sizeof(STMessageHeader);
	int nTotSize = nHeaderSize+nMHSize+nMsgLen;
	char *pDest = NULL;
	char szBuf[8192];

	if(8192 < nTotSize) {
		pDest = new (std::nothrow) char[nTotSize];
		if(!pDest) {
			m_pEqMLogger->PutLogQueue(LOG_ERROR,_T("CResourceProc::SendMakePacket send new pData is NULL size[%d]"), nTotSize); 
			return;
		}
	}
	else pDest = szBuf;	

 	stPacketHeader stPacketHeader;
	stPacketHeader.nDataSize = nMsgLen+nMHSize;

    memcpy(pDest, (char *)&stPacketHeader, nHeaderSize);
    memcpy(&pDest[nHeaderSize], (char *)&nMsgIdx, nMHSize);
	if(pMsg) {
		memcpy(&pDest[nHeaderSize+nMHSize], pMsg, nMsgLen);
	}


	nTotSize = sendData(nSIdx, pDest, nTotSize, DEF_TIMEOUT);
	
	if (nTotSize == -1)
	{
		m_pEqMLogger->PutLogQueue(LOG_ERROR,_T("CResourceProc::SendMakePacket send error SOCKET[%d]"), m_stSessionArray[nSIdx].nSD);    
		delSession(nSIdx);
	}

	if(8192 < nTotSize) { delete[] pDest; }
}

void CResourceProc::SendSimpleData(int nSidx, int nMsgIdx)
{
	STSimpleData stData;
	int nDataSize = sizeof(STSimpleData);
	memset(&stData, 0, nDataSize);
	switch(nMsgIdx) 
	{
	case param_CPU_avg_Usage :  // cpu avgUsage
		stData.nValue = nCpusAverageUsage_;
		//printf("eProtocol_reqCPUAvgUsage : %d\n", stData.nValue);
		break;
	case param_HDD_Usage : // hdd usage
		stData.nValue = m_nStorageUsage;
		//printf("eProtocol_reqHDDUsage : %d\n", stData.nValue);
		break;
	case param_MemoryInfo : 
		{
			memcpy(&(stData.nValue), &nMemUsage_, 4);
			memcpy(stData.szTemp, (char *)&ulTotalMemory_, sizeof(unsigned long));
			memcpy(stData.szTemp+8, (char *)&ulUsingMemory_, sizeof(unsigned long));
		}
		break;
	}

	SendMakePacket(nMsgIdx, (char *)&stData, nDataSize, nSidx); 
}

void CResourceProc::SendResource(int nSidx, int nMsgIdx)
{
	STResource stData;
	int nDataSize = sizeof(STResource);
	memset(&stData, 0, nDataSize);
	stData.nCpu = nCpusAverageUsage_;
	stData.nStorage = m_nStorageUsage;
	memcpy(&(stData.nMemory), &nMemUsage_, 4);
	memcpy(stData.szTemp, (char *)&ulTotalMemory_, sizeof(unsigned long));
	memcpy(stData.szTemp + 8, (char *)&ulUsingMemory_, sizeof(unsigned long));

	SendMakePacket(nMsgIdx, (char *)&stData, nDataSize, nSidx); 
}

void CResourceProc::setNetIo()
{
	int i;
	char *pName;
	unsigned long long nRXBytes, nTXBytes, nTot = 0;
	for (i = 0; i < m_nInterfaceCount; i++) {
		GET_NET_INFO(i, &pName, &nRXBytes, &nTXBytes);
		nTot += nRXBytes + nTXBytes;
	}
	m_nNetIOTot = nTot;
}

void CResourceProc::sendNetInfo(int nSidx, int nMsgIdx)
{
	char szBuf[512];
	char *pBuf = szBuf;
	int nDataSize=0, i;
	char *pName;
	unsigned long long nRXBytes, nTXBytes, nTot=0;
	

	memset(pBuf, 0, 512);

	pBuf += eNet_Bytes_Size;
	memcpy(pBuf, (char *)&m_nInterfaceCount, eNet_InterfaceCount_Size);
	pBuf += eNet_InterfaceCount_Size; 
	nDataSize += (eNet_Bytes_Size + eNet_InterfaceCount_Size);
	for (i = 0; i < m_nInterfaceCount; i++) {
		GET_NET_INFO(i, &pName, &nRXBytes, &nTXBytes);
		memcpy(pBuf, pName, strlen(pName)+1); pBuf += eNet_InterfaceName_Size;
		memcpy(pBuf, (char *)&nRXBytes, eNet_Bytes_Size); pBuf += eNet_Bytes_Size;
		memcpy(pBuf, (char *)&nTXBytes, eNet_Bytes_Size); pBuf += eNet_Bytes_Size;
		nDataSize += (eNet_InterfaceName_Size + eNet_Bytes_Size + eNet_Bytes_Size);
		nTot += nRXBytes + nTXBytes;
	}
	m_nNetIOTot = nTot;
	memcpy(szBuf, (char *)&m_nNetIOTot, eNet_Bytes_Size);
	SendMakePacket(nMsgIdx, (char *)&szBuf, nDataSize, nSidx);
}



void CResourceProc::MakeCPUPerformanceStateNL(void)
{
	vector<unsigned long> vtCpusCurrentTick;
	int nCpuUsage, nCpuTotalUsage, nCpuCount;
	vector<int>::iterator vtIt;

	CCmProcess::GetSystemCpusTick(&vtCpusCurrentTick);
	if (vtCpusLastTick_.size() <= 0)   // if run first time
	{
		vtCpusLastTick_ = vtCpusCurrentTick;
		nCpusAverageUsage_ = 0;
		return;
	}

	CCmProcess::GetSystemCpusUsage(&vtCpusLastTick_, &vtCpusCurrentTick, &vtCpusUsage_);
	vtCpusLastTick_ = vtCpusCurrentTick;
	//- remove first node (average cpu usage calculated by system) ---
	vtIt = vtCpusUsage_.begin();
	if (vtIt != vtCpusUsage_.end())
	{
		vtCpusUsage_.erase(vtIt);
	}

	nCpuCount = vtCpusUsage_.size();
	if (nCpuCount <= 0)    // error ?
	{
		nCpusAverageUsage_ = 0;
		return;
	}

	nCpuTotalUsage = 0;
	for (vtIt = vtCpusUsage_.begin(); vtIt != vtCpusUsage_.end(); vtIt++)
	{
		nCpuUsage = *vtIt;
		nCpuTotalUsage += nCpuUsage;
	}
	nCpusAverageUsage_ = nCpuTotalUsage / nCpuCount;

}

void CResourceProc::setMemoryInfo()
{
	double tot, used;
	GET_MEM_INFO(&ulTotalMemory_, &ulUsingMemory_);
	tot = (double)ulTotalMemory_;
	used = (double)ulUsingMemory_;
	nMemUsage_ = (float) (used / tot * 100);
//	printf("setMemoryInfo() :: nMemUsage_ [%f]\n", nMemUsage_);
	m_nStorageUsage = GET_HDD_USAGE(CEnv::WorkingDir());
}
