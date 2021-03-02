/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
2020.09.13 by KEH
-------------------------------------------------------------
core thread Library Group
System configration Init
*********************************************************************/
#pragma once
#ifndef	__COMCORE_H__
#define	__COMCORE_H__

//#define __RUN_XP

#include "comEnv.h"
#include "comLogger.h"
#include "comMemPool.h"
#include "comTList.h"

typedef void(*fp_Proc)(void *p, void *p2, int n);

enum E_SYS_LIST_TYPE
{
	e_TYPE_THREAD = 1,
	e_TYPE_THREADQUEUE,
	e_TYPE_THREADSCHEDULER,
	e_TYPE_THREADWRITER,
};
enum E_S_FLAG {
	IS_CREATE_THREAD = 0x00000001,
	IS_CREATE_LOGGER = 0x00000002,
	IS_ACTIVE_PUT_QUEUE = 0x00000004,
	IS_ACTIVE_TIMEPROC_INTHREAD = 0x00000008
};
typedef void(*fp_IntervalProc)(void *pObj);

struct STIntervalInfo
{
	int nThreadIdx;
	int nFlag;
	TICKTIME_MILLISEC nInterval;
	TICKTIME_MILLISEC nCur;
	TICKTIME_MILLISEC nGoal;  // stStart + stInterval
	void *pObj;
	fp_IntervalProc pFunction;
};

struct STEvent
{
	int nThreadIdx;
	int nValue;
	void *pObj;
	void *pObj2;
	fp_Proc fpProc;
};

struct STThreadInfo
{
	THREAD_ID	hTHID;
	THREAD_COND cThreadCond;
	void	*pClass;
	void	*pObj;
	int		nIdx;
	bool	bActive;
	int millisecSleep;
};

typedef void (*fp_DateChangeProc)(int nDate); 
typedef void (*fp_LoopProc)(STThreadInfo *p);


class CComThread {
public:
	CComThread();
	~CComThread();

	bool init(fp_LoopProc fpProc, void *pClass, int millisec, void *pObj = NULL);
	bool start();
	bool quit();
	static THREAD_RET_TYPE THREAD_CALLING_CONVENTION ProcThread(void *ptr);
	fp_LoopProc m_fpLoopProc;
	STThreadInfo m_stInfo;

	inline void setIdx(int nIdx) { m_stInfo.nIdx = nIdx; }
	inline bool bActive() {	return m_stInfo.bActive; }
	inline void sendSignal() { m_stInfo.cThreadCond.signal(); }
	inline void pause() { m_stInfo.millisecSleep = INFINITE; }
	inline void resume() { m_stInfo.millisecSleep = m_nMillisecSleep;  m_stInfo.cThreadCond.signal(); }
private:
	int m_nMillisecSleep;
};

struct STLCommand
{
	int nCommandCode;
	void *p;
};
void loggerLoop(STThreadInfo *p);
class CSTLogger {
public:
	CSTLogger();
	~CSTLogger();

	bool initConfig(TCHAR *pConfigFile, int seq, TCHAR *pSection);
	bool init(int nCount, int idx);
	bool start();
	bool quit();
	bool IsCommand();
	void GetQueue();
	void TimeProc();
	void procLoggerEnable(CLogger *p);
	void porcLoggerDisable(CLogger *p);
	inline bool pushCommand(STLCommand *p) {if(m_pComandList->push(p)) { m_cThread.sendSignal(); return true; } return false;}
	CComThread m_cThread;
private:
	CTList<CLogger>	*m_pLoggerList;
	CQueueS *m_pComandList;
	bool _init(int nCount);
};

void schedulerLoop(STThreadInfo *p);
class CScheduler {
public:
	CScheduler();
	~CScheduler();

	bool initConfig(TCHAR *pConfigFile, int seq, TCHAR *pSection);
	bool init(int nCount, int nMilliSecWait, int nIdx);
	bool start();
	bool quit();

	//static THREAD_RET_TYPE THREAD_CALLING_CONVENTION SchedulerThread(void *ptr);
	// time queue proc for scheduling
	inline STIntervalInfo *timeNext(int *idx) { return m_pTimeList->getNext(idx); }
	inline bool deleteTime(STIntervalInfo *pData) { return m_pTimeList->del(pData); }
	inline int addTime(STIntervalInfo *pData) { return m_pTimeList->add(pData); }
	inline int getMilliSec() {return m_nMilliSecWait;}
	inline int getMilliSecHalf() { return m_nMillSecHalf; }
	inline void setGoal() {	m_nMillSecGoal += m_nMilliSecWait;	}
	inline TICKTIME_MILLISEC getGoal() { 	return m_nMillSecGoal;	}
	CComThread m_cThread;
private:
	CMemList<STIntervalInfo> *m_pTimeList;
	int m_nMilliSecWait;
	int m_nMillSecHalf;
	TICKTIME_MILLISEC m_nMillSecGoal;
};



void eventThreadLoop(STThreadInfo *p);
class CComQueueThread {
public:
	CComQueueThread();
	~CComQueueThread();

	bool initConfig(TCHAR *pConfigFile, int seq, TCHAR *pSection);
	bool init(int nCount, int nIdx);
	bool start();
	bool quit();
	bool putQueue(STEvent *p) { if (m_cThread.bActive()) { m_sLogQueue.push(p); m_cThread.sendSignal(); return true; } return false; }
	STEvent *getEvent() { return (STEvent *)m_sLogQueue.pop(); }
	CComThread m_cThread;

private:
	CQueueS	m_sLogQueue;
};


class CCoreList
{
public:
	CCoreList();
	~CCoreList();

	// system.ini
	bool initSystemFile();
	bool initDefaultPool(int nCnt_IdxBuf2, int nCnt_IdxBuf32, int nCnt_IdxBuf_1m);

	bool initLogger(CLogger *pLogger, TCHAR *pConfig, TCHAR *pSection);
	bool initLogger(CLogger *pLogger);
	bool setLoggerEnable(CLogger *pLogger);
	bool setLoggerDisable(CLogger *pLogger);
	bool start();
	void destroy();
	
	inline STIntervalInfo * newTime() { return m_pIntervalPool->newMem(); }
	inline STEvent * newEvent() { return m_pEventPool->newMem(); }
	inline bool delTimer(STIntervalInfo *p) { return m_pIntervalPool->delMem(p); }
	inline bool delEvent(STEvent *p) { return m_pEventPool->delMem(p); }

	bool putEvent(STEvent *p);
	bool putTimer(STIntervalInfo *p);
	inline bool popTimer(STIntervalInfo *p) { p->nFlag = 0; }


	inline bool isLogWriter() { if (m_pWriterList) return true; return false; }
	inline bool isScheduler() { if (m_pSchedulerList) return true; return false;	}
	inline bool isQueueThread() { if (m_pEventThreadList) return true; return false;	}

	// return value : list idx, error : -1
	int addComThread(fp_LoopProc fpProc, void *pClass, int millisec, void *pObj);
	bool delComThread(int nSeq);
	int addLogWriter(int nCount);					// queue init count
	int addScheduler(int nCount, int millisecWait); // queue init count
	int addQueueThread(int nCount);					// queue init count

	// initialize by userDefined.ini
	bool initComThread(TCHAR *pConfigFile, TCHAR *pSection);
	bool initLogWriter(TCHAR *pConfigFile, TCHAR *pSection, bool bSystem=false);
	bool initScheduler(TCHAR *pConfigFile, TCHAR *pSection);
	bool initQueueThread(TCHAR *pConfigFile, TCHAR *pSection);

	bool initComThread(int nThreadCount);
	bool initLogWriter(int nThreadCount);
	bool initScheduler(int nThreadCount, int nSchedulerPoolCount=0);
	bool initQueueThread(int nThreadCount, int nQueuePoolCount=0);
	
	void setListAllocType(E_ALLOC_TYPE type, E_SYS_LIST_TYPE eListType);

private:
	CTList<CSTLogger>		*m_pWriterList;
	CTList<CScheduler>		*m_pSchedulerList;
	CTList<CComThread>		*m_pThreadList;
	CTList<CComQueueThread> *m_pEventThreadList;

	CMemPool<STIntervalInfo> *m_pIntervalPool;
	CMemPool<STEvent>		 *m_pEventPool;
};

extern CCoreList *gs_pCore;


#endif	//	__COMCORE_H__



