#ifndef __ResourceProc__
#define __ResourceProc__

#define MAX_SESSION_EQM	10

#include "comLogger.h"
#include "resourceProc.h"
#include <vector>
using namespace std;

enum {
	NOT_USE = 0,
	USING,
};

typedef	struct {
	SOCKET			nSD;
	char			nUse;
	time_t			tmRequest;
	bool		bReport;
} StSessionInfo;

enum EEqMThread
{
	eEqMT_Listen = 0,
	eEqMT_Receive,
	eEqMT_Monitor,
	eEqMT_Max
};

class CResourceProc 
{
public:
	CResourceProc();
	~CResourceProc();

	bool Init();
	bool Start();
	void Quit();

	void RecvProcess(int nSidx);
	int	 SetSDValues(fd_set *stFds);
	inline void	SetListenPortNo(int nPort)		{ m_nListenPort = nPort;	}
	inline int	GetListenPort()					{ return m_nListenPort;		}
	inline int LastSession()					{ return m_nSLastIdx; }
	inline void setVersion(const char *pSource) { strcpy(m_szVersion, pSource); }
	inline char * getVersion() { return m_szVersion; }
	void sendAllSession(char *szBuf, int nLen);
	int	 addSession(SOCKET sd, void *pData);
	int	 delSession(int idx);
	void checkSessionTime();
	void sendDeactivateAllPlan_PerformanceWarning(int nType);
	void getMemoryInfo(float *pUsage, unsigned long *pUsingMemory, unsigned long *pTotMemory); 

	static THREAD_RET_TYPE THREAD_CALLING_CONVENTION ListenThread(void *ptr);		// 싱글 혹은 멀티 둘 다 사용
	static THREAD_RET_TYPE THREAD_CALLING_CONVENTION ReceiveThread(void *ptr);		// 싱글 쓰레드 방식에서 사용
	static THREAD_RET_TYPE THREAD_CALLING_CONVENTION MonitorThread(void *ptr);		// 싱글 쓰레드 방식에서 사용

	THREAD_ID m_nThreadID[eEqMT_Max];
	StSessionInfo	m_stSessionArray[MAX_SESSION_EQM];
	bool	m_bAlive;
	CSTLogger *m_pEqMLogger;
	inline int getLogSeq() { if(m_pEqMLogger) return m_pEqMLogger->getSeq(); return 0;}
	inline void setInterCount(int nCount) { m_nInterfaceCount = nCount; }

	void MakeCPUPerformanceStateNL();
	void setMemoryInfo();

	int m_nStorageUsage;
	int nCpusAverageUsage_;
	float nMemUsage_;
	unsigned long ulTotalMemory_, ulUsingMemory_;
	int m_nInterfaceCount;
	unsigned long long m_nNetIOTot;
	int	m_nWriteToLog;


private :
	int		m_nListenPort;
	int		m_nSLastIdx;			
	atomic_nr m_nSessionUseCnt;				// 세션의 개수
	char m_szVersion[64];

	vector<unsigned long> vtCpusLastTick_;
	vector<int> vtCpusUsage_;


	int recvData(SOCKET FD, char *szBuf, int nLen, long ltimeOut);
	int	sendData(int nSIdx, char *szBuf, int nLen, long ltimeOut);
	// service lotine
	void SendSimpleData(int nSidx, int nMsgIdx);
	void SendResource(int nSidx, int nMsgIdx);
	void sendNetInfo(int nSidx, int nMsgIdx);
	void SendMakePacket(int nMsgIdx, const char *pMsg, int nMsgLen, int nSIdx);

	void setNetIo();

};

extern CResourceProc g_cResourceProc;

#endif
