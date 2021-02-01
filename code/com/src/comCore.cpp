/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                   */
/*   Copyright 2012 by keh									  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

#include "comCore.h"

STTodayTime gs_today_time;
CCoreList *gs_pCore;

CComThread::CComThread()
{
	m_stInfo.bActive = false;
	m_stInfo.hTHID = NULL;
	m_stInfo.nIdx = -1;
	m_stInfo.pClass = NULL;
	m_stInfo.millisecSleep = INFINITE;
	m_nMillisecSleep = INFINITE;
	m_stInfo.pObj = NULL;
	m_fpLoopProc = NULL;

}

CComThread::~CComThread()
{
	quit();
}


bool CComThread::init(fp_LoopProc fpProc, void *pClass, int millisec, void *pObj)
{
	m_stInfo.pClass = pClass;
	m_stInfo.pObj = pObj;
	m_stInfo.millisecSleep = millisec;
	m_fpLoopProc = fpProc;
	m_nMillisecSleep = millisec;
	return true;
}

bool CComThread::start()
{
	if (!m_fpLoopProc) {
		comErrorPrint("m_fpLoopProc is NULL");
		return false;
	}
	if (0 > m_stInfo.nIdx) {
		comErrorPrint("0 > m_stInfo.nIdx");
		return false;
	}

	if (m_stInfo.bActive) return true;
	// create Single Thread
	if (!THREAD_CREATE(&m_stInfo.hTHID, ProcThread, this)) {
		comErrorPrint("THREAD_CREATE has Failed");
		return false;
	}
	m_stInfo.bActive = true;
	return true;
}

bool CComThread::quit()
{
	if (!m_stInfo.hTHID) return false;
	int nStatus = 0; // for linux
	m_stInfo.bActive = false;
	THREAD_JOIN(m_stInfo.hTHID, (void **)&nStatus);
	return true;
}

THREAD_RET_TYPE	THREAD_CALLING_CONVENTION
CComThread::ProcThread(void *ptr)
{
	//pthread_detach(pthread_self());	
	CComThread *pClass = (CComThread *)ptr;
	STThreadInfo *pBasic = &(pClass->m_stInfo);

	while (pBasic->bActive)
	{
		pBasic->cThreadCond.wait(pBasic->millisecSleep, NULL);	// -1 : INFINITE
		pClass->m_fpLoopProc(pBasic);
	}
	pClass->quit();
	return 0;
}

//-------------------------------------------------------------

CSTLogger::CSTLogger()
{
	m_pComandList = NULL;
	m_pLoggerList = NULL;
}

CSTLogger::~CSTLogger()
{
	quit();
	if (m_pComandList) {
		delete m_pComandList;
		m_pComandList = NULL;
	}
	if (m_pLoggerList) {
		delete m_pLoggerList;
		m_pLoggerList = NULL;
	}
}



bool CSTLogger::initConfig(TCHAR *pConfigFile, int i, TCHAR *pSection)
{
	TCHAR szSect[24];
	int nMax;

	_stprintf(szSect, _T("%s_%d"), pSection, i);
	nMax = GetPrivateProfileInt(szSect, _T("LIST_COUNT"), 0, pConfigFile);
	if (!nMax) return false;
	if (!_init(nMax)) return false;
	return true;
}

bool CSTLogger::init(int nCount, int idx)
{
	if (!nCount) return false;
	if (!_init(nCount)) return false;
	m_cThread.setIdx(idx);
	return true;
}

bool CSTLogger::_init(int nCount) {
	m_pLoggerList = new CTList<CLogger>();
	if (!m_pLoggerList->alloc(nCount, eAlloc_Type_none)) return false;
	m_pComandList = new CQueueS();
	if (!m_pComandList->alloc(10, eAlloc_Type_none)) return false;
	return true;
}

bool CSTLogger::start()
{
	if (m_cThread.bActive()) return true;
	if (!m_cThread.init(loggerLoop, this, INFINITE)) {
		comErrorPrint("m_cThread.init");
		return false;
	}
	if (!m_cThread.start()) {
		comErrorPrint("m_cThread.start");
		return false;
	}
	printf("CSTLogger::start()\n");
	return true;
}

bool CSTLogger::IsCommand()
{
	bool bRes = false;
	STLCommand *pCommand = (STLCommand *)m_pComandList->pop();
	int nSTLSize = sizeof(STLCommand);
	while (pCommand) {
		bRes = true;
		if (pCommand->nCommandCode == 1) { // enable
			procLoggerEnable((CLogger *)pCommand->p);
		}
		else if (pCommand->nCommandCode == 2) { // 
			porcLoggerDisable((CLogger *)pCommand->p);
		}
		gs_pMMgr->delBuf((char *)pCommand, nSTLSize);
		pCommand = (STLCommand *)m_pComandList->pop();
	}

	return false;
}

void CSTLogger::procLoggerEnable(CLogger *pLogger)
{
	if (pLogger->isLogFlag(IS_LOG_ENABLE)) {
		printf("Logger is already Enabled!");
		return ;
	}
	else if (pLogger->isLogFlag(IS_LOG_OPEN)) {}
	else if (pLogger->isLogFlag(IS_LOG_SET_COMPLETE)) {
		pLogger->OpenLogger();
	}
	else {
		comErrorPrint("Logger has not Initialized!");
		return ;
	}

	// input
	if (m_pLoggerList->IsExist(pLogger)) {
		printf("Logger already exist!\n");
		return;
	}
	else {
		pLogger->setCondition(&m_cThread.m_stInfo.cThreadCond);
		if (!m_pLoggerList->push_back(pLogger)) {
			printf("m_pLoggerList->push_back failed!\n");
			return;
		}
	}
	pLogger->setLogFlag(IS_LOG_ENABLE);
	printf("[%s] procLoggerEnable\n", pLogger->getLogName());

}

void CSTLogger::porcLoggerDisable(CLogger *pLogger)
{
	int nCount = 0;
	TCHAR *pData = pLogger->pop();
	while (pData)
	{
		int nLen = (int)_tcslen(pData) + 1;
		pLogger->LogPrint(pData);
		pLogger->addLogSize(nLen);
		gs_pMMgr->delBuf(pData, nLen);
		nCount++;
		pData = pLogger->pop();

	}
	pLogger->disable();
	m_pLoggerList->del(pLogger);
	pLogger->clearLogFlag(IS_LOG_ENABLE);
	printf("writeCount[%d] [%s] porcLoggerDisable\n", nCount, pLogger->getLogName());
}

bool CSTLogger::quit()
{
	m_cThread.quit();
	if (m_pLoggerList) {
		delete m_pLoggerList;
		m_pLoggerList = NULL;
	}
	return true;
}

// 사용 안함
void CSTLogger::GetQueue()
{
	int idx=0;
	TCHAR *pData;
	CLogger *pLogger = m_pLoggerList->getNext(&idx);
	while (pLogger) {

		pData = pLogger->pop();
		while (pData)
		{
			int nLen = (int)_tcslen(pData) + 1;
			pLogger->LogPrint(pData);
			pLogger->addLogSize(nLen);
			gs_pMMgr->delBuf(pData, nLen);
			pData = pLogger->pop();

		}
		idx++;
		pLogger = m_pLoggerList->getNext(&idx);
	}
}


void loggerLoop(STThreadInfo *p)
{
	CSTLogger *pBasic = (CSTLogger *)p->pClass;
	if (pBasic->IsCommand()) return;
	pBasic->GetQueue();
	pBasic->TimeProc();
}

void CSTLogger::TimeProc()
{
	CDateTime	ltm;
	int nFlag = 0;
	gs_today_time.second = ltm.Sec();
	if (ltm.Minute() != gs_today_time.minute) {
		gs_today_time.minute = ltm.Minute();
		nFlag |= LOG_FLAG_MINCHANGE;
	}
	if (ltm.Hour() != gs_today_time.hour) {
		gs_today_time.hour = ltm.Hour();
		nFlag |= LOG_FLAG_HOURCHANGE;
	}
	if (ltm.Mday() != gs_today_time.mday) { // 1. Day Change 체크
		nFlag |= LOG_FLAG_DAYCHANGE;
		gs_today_time.date  = _ttoi(ltm.DateString(0));
		gs_today_time.year	= ltm.Year();
		gs_today_time.month	= ltm.Month();
		gs_today_time.mday	= ltm.Mday();
	}

	if (nFlag) {
		int idx = 0;
		CLogger *pLogger = m_pLoggerList->getNext(&idx);
		while (pLogger) {
			pLogger->checkChangeProc(nFlag);
			idx++;
			pLogger = m_pLoggerList->getNext(&idx);
		}
	}
}

CScheduler::CScheduler()
{
	m_pTimeList = NULL;
	m_nMilliSecWait = -1;
	m_nMillSecGoal = 0;
}
CScheduler::~CScheduler()
{
	quit();
}

bool CScheduler::initConfig(TCHAR *pConfigFile, int i, TCHAR *pSection)
{
	TCHAR szSect[24];
	int nCount;

	_stprintf(szSect, _T("%s_%d"), pSection, i);
	nCount = GetPrivateProfileInt(szSect, _T("LIST_COUNT"), 0, pConfigFile);
	if (!nCount) return false;
	m_nMilliSecWait = GetPrivateProfileInt(szSect, _T("TIME_WAIT"), -1, pConfigFile);
	m_nMillSecHalf = m_nMilliSecWait / 2;
	m_nMillSecGoal = CURRENT_TIME + m_nMilliSecWait;

	m_pTimeList = new CMemList<STIntervalInfo>();
	if (!m_pTimeList->alloc(nCount, eAlloc_Type_none)) { delete m_pTimeList; printf("has Failed!! m_pTimeList->alloc(%d)\n", nCount); return false; }

	return true;

}

bool CScheduler::init(int nCount, int nMilliSecWait, int nIdx)
{
	m_nMilliSecWait = nMilliSecWait;
	m_nMillSecHalf = m_nMilliSecWait / 2;
	m_nMillSecGoal = CURRENT_TIME + m_nMilliSecWait;
	m_pTimeList = new CMemList<STIntervalInfo>();
	if (!m_pTimeList->alloc(nCount, eAlloc_Type_none)) { delete m_pTimeList; printf("has Failed!! m_pTimeList->alloc(%d)\n", nCount); return false; }
	m_cThread.setIdx(nIdx);
	return true;
}


bool CScheduler::start()
{
	if (m_cThread.bActive()) return true;
	if (!m_cThread.init(schedulerLoop, this, m_nMillSecHalf)) {
		comErrorPrint("m_cThread.init");
		return false;
	}
	if (!m_cThread.start()) {
		comErrorPrint("m_cThread.start");
		return false;
	}
	return true;
}

bool CScheduler::quit()
{
	m_cThread.quit();
	if (m_pTimeList) {
		delete m_pTimeList;
		m_pTimeList = NULL;
	}
	return true;
}

void schedulerLoop(STThreadInfo *p)
{
	CScheduler *pBasic = (CScheduler *)p->pClass;
	STIntervalInfo *pData;
	TICKTIME_MILLISEC cur = CURRENT_TIME;
	int diff = pBasic->getGoal() - cur;
	

	if (1 < diff) {
		p->millisecSleep = diff;
		return;
	}


	int idx = 0;
	pData = (STIntervalInfo *)pBasic->timeNext(&idx);
	while (pData)
	{
		if (!pData->nFlag) {
			pBasic->deleteTime(pData);
		}
		else 
		{
			pData->pFunction(pData);
		}

		idx++;
		pData = (STIntervalInfo *)pBasic->timeNext(&idx);
	}
	pBasic->setGoal();
	p->millisecSleep = pBasic->getMilliSecHalf();
}

CComQueueThread::CComQueueThread()
{

}
CComQueueThread::~CComQueueThread()
{
	quit();
}

bool CComQueueThread::initConfig(TCHAR *pConfigFile, int seq, TCHAR *pSection)
{
	TCHAR szSect[24];
	int nCount;

	_stprintf(szSect, _T("%s_%d"), pSection, seq);
	nCount = GetPrivateProfileInt(szSect, _T("LIST_COUNT"), 20, pConfigFile) + LIST_ADD_COUNT;

	if (!m_sLogQueue.alloc(nCount, eAlloc_Type_none)) { printf("has Failed!! m_sLogQueue.alloc(%d)\n", nCount); return false; }
	return true;
}

bool CComQueueThread::init(int nCount, int nIdx)
{
	if (!m_sLogQueue.alloc(nCount, eAlloc_Type_none)) { printf("has Failed!! m_sLogQueue.alloc(%d)\n", nCount); return false; }
	m_cThread.setIdx(nIdx);
	return true;
}


bool CComQueueThread::start()
{
	if (m_cThread.bActive()) return true;
	if (!m_cThread.init(eventThreadLoop, this, INFINITE)) {
		comErrorPrint("m_cThread.init");
		return false;
	}
	if (!m_cThread.start()) {
		comErrorPrint("m_cThread.start");
		return false;
	}
	return true;
}

bool CComQueueThread::quit()
{
	m_cThread.quit();
	return true;
}


void eventThreadLoop(STThreadInfo *p)
{
	CComQueueThread *pBasic = (CComQueueThread *)p->pClass;
	STEvent *pEvent = pBasic->getEvent();
	while (pEvent)
	{
		pEvent->fpProc(pEvent->pObj, pEvent->pObj2, pEvent->nValue);
		gs_pCore->delEvent(pEvent);
		pEvent = pBasic->getEvent();
	}
}

CCoreList::CCoreList()
{
	m_pWriterList = NULL;
	m_pSchedulerList = NULL;
	m_pThreadList = NULL;
	m_pEventThreadList = NULL;
	m_pIntervalPool = NULL;
	m_pEventPool = NULL;

	gs_pMMgr = new (std::nothrow) CMemManager();
	if (!gs_pMMgr) {
		comErrorPrint("gs_pMMgr alloc has Failed!!!");
		exit(0);
	}

	if (!initSystemFile()) {
		comErrorPrint("initSystemFile() has Failed!!!");
		exit(0);
	}
	if (!start())
	{
		comErrorPrint("start() has Failed!!!");
		exit(0);
	}	
}


CCoreList::~CCoreList()
{
	destroy();
}

void CCoreList::destroy()
{
	if (m_pWriterList) {
		delete m_pWriterList; 
		m_pWriterList = NULL;
	}
	if (m_pSchedulerList) {
		delete m_pSchedulerList;
		m_pSchedulerList = NULL;
	}
	if (m_pThreadList) {
		delete m_pThreadList;
		m_pThreadList = NULL;
	}
	if (m_pEventThreadList) {
		delete m_pEventThreadList;
		m_pEventThreadList = NULL;
	}
	if (m_pIntervalPool) {
		delete m_pIntervalPool;
		m_pIntervalPool = NULL;
	}
	if (m_pEventPool) {
		delete m_pEventPool;
		m_pEventPool = NULL;
	}
}

bool CCoreList::initSystemFile()
{
	int i, nVal, nShift;
	if (!CEnv::initSystem()) return false;

	STBufConf szConf;
	TCHAR szKey[36];
	memset(&szConf, 0, sizeof(STBufConf));

	nVal = GetPrivateProfileInt(_T("MPOOL"), _T("IDX_BUF_32"), 0, g_pSystem);
	if (!nVal) goto INIT_THREAD;
	
	szConf.nMaxList[IDX_BUF_2] = GetPrivateProfileInt(_T("MPOOL"), _T("IDX_BUF_2"), 0, g_pSystem);
	
	for (i = IDX_BUF_4; i < IDX_BUF_256; i++)
	{
		_stprintf(szKey, _T("IDX_BUF_%d"), 1 << i);
		szConf.nMaxList[i] = GetPrivateProfileInt(_T("MPOOL"), szKey, 0, g_pSystem) + 256;
	}
	for (i = IDX_BUF_256; i < IDX_BUF_1k; i++)
	{
		_stprintf(szKey, _T("IDX_BUF_%d"), 1 << i);
		szConf.nMaxList[i] = GetPrivateProfileInt(_T("MPOOL"), szKey, 0, g_pSystem) + 64;
	}

	nVal = 1; nShift = 0;
	for (i = IDX_BUF_1k; i < IDX_BUF_64k; i++)
	{
		_stprintf(szKey, _T("IDX_BUF_%dk"), nVal); nShift++; //0(1K), 1(2K), 2(4K), 3(8K), 4(16K), 5(32K)
		szConf.nMaxList[i] = GetPrivateProfileInt(_T("MPOOL"), szKey, 0, g_pSystem) + 4;
		nVal = 1 << nShift;
	}
	//6(64K)
	for (i = IDX_BUF_64k; i < IDX_BUF_512k; i++)
	{
		_stprintf(szKey, _T("IDX_BUF_%dk"), nVal); nShift++; //6(64K), 7(128K), 8(256K)
		szConf.nMaxList[i] = GetPrivateProfileInt(_T("MPOOL"), szKey, 0, g_pSystem) + 2;
		nVal = 1 << nShift;
	}
	//9(512K)
	szConf.nMaxList[IDX_BUF_512k] = GetPrivateProfileInt(_T("MPOOL"), _T("IDX_BUF_512k"), 0, g_pSystem) + 1;
	szConf.nMaxList[IDX_BUF_1m] = GetPrivateProfileInt(_T("MPOOL"), _T("IDX_BUF_1m"), 0, g_pSystem);
	szConf.nMaxList[IDX_BUF_2m] = GetPrivateProfileInt(_T("MPOOL"), _T("IDX_BUF_2m"), 0, g_pSystem);

	gs_pMMgr->init(&szConf);
	if (!gs_pMMgr->alloc()) {
		comErrorPrint("gs_pMMgr->alloc() has Failed!!!");
		return false;
	}

INIT_THREAD :
	if (!initComThread(g_pSystem, _T("COMTHREAD")))			return false;
	if (!initLogWriter(g_pSystem, _T("LOG_WRITER"),true))	return false;
	if (!initScheduler(g_pSystem, _T("SCHEDULER")))			return false;
	if (!initQueueThread(g_pSystem, _T("EVENTPROC")))		return false;

	// LogTimeExpresion
	if (!gs_DTEManager.initConfig(g_pSystem)) {
		comErrorPrint("gs_DTEManager.initConfig has Failed!!!");
		return false;
	}

	// basic logger init
	if (!initLogger(&gs_cLogger, g_pSystem, _T("LOG"))) {
		comErrorPrint("initLogger has Failed!!!");
		return false;
	}

	return true;

}

bool CCoreList::initDefaultPool(int nCnt_IdxBuf2, int nCnt_IdxBuf32, int nCnt_IdxBuf_1m)
{
	int i, nVal;

	STBufConf szConf;
	memset(&szConf, 0, sizeof(STBufConf));

	szConf.nMaxList[IDX_BUF_2] = nCnt_IdxBuf2;
	
	nVal = nCnt_IdxBuf32 << 2;
	for (i = IDX_BUF_4; i < IDX_BUF_256; i++)
	{
		szConf.nMaxList[i] = nVal + 1024;
	}

	for (i = IDX_BUF_256; i < IDX_BUF_1k; i++)
	{
		szConf.nMaxList[i] = nCnt_IdxBuf32 + 64;
	}

	nVal = nCnt_IdxBuf32 >> 2;
	for (i = IDX_BUF_1k; i < IDX_BUF_64k; i++)
	{
		szConf.nMaxList[i] = nVal + 4;
	}
	//6(64K)
	for (i = IDX_BUF_64k; i < IDX_BUF_512k; i++)
	{
		szConf.nMaxList[i] = nVal + 2;
	}
	//9(512K)
	nVal = nCnt_IdxBuf32 >> 8;
	szConf.nMaxList[IDX_BUF_512k] = nVal;
	szConf.nMaxList[IDX_BUF_1m] = nCnt_IdxBuf_1m;
	szConf.nMaxList[IDX_BUF_2m] = nCnt_IdxBuf_1m;

	gs_pMMgr->init(&szConf);
	if (!gs_pMMgr->alloc()) {
		comErrorPrint("gs_pMMgr->alloc() has Failed!!!");
		return false;
	}

	return true;
}

void CCoreList::setListAllocType(E_ALLOC_TYPE type, E_SYS_LIST_TYPE eListType)
{
	switch (eListType)
	{
	case e_TYPE_THREAD: m_pThreadList->setObjAllocType(type);
		break;
	case e_TYPE_THREADQUEUE: m_pEventThreadList->setObjAllocType(type);
		break;
	case e_TYPE_THREADSCHEDULER: m_pSchedulerList->setObjAllocType(type);
		break;
	case e_TYPE_THREADWRITER: m_pWriterList->setObjAllocType(type);
		break;
	}
}

bool CCoreList::initComThread(TCHAR *pConfigFile, TCHAR *pSection)
{
	int nCount, i, idx;
	CComThread *pComThread;
	nCount = GetPrivateProfileInt(pSection, _T("LIST_COUNT"), 0, pConfigFile);
	if (nCount)
	{
		if (!m_pThreadList) {
			m_pThreadList = new CTList<CComThread>();
			if (!m_pThreadList->alloc(nCount + LIST_ADD_COUNT, eAlloc_Type_none)) {
				comErrorPrint("m_pThreadList = new CTList<CComThread>() is NULL");
				return false;
			}
		}

		for (i = 0; i < nCount; i++) {
			pComThread = new (std::nothrow) CComThread();
			if (!pComThread) {
				comErrorPrint("new (std::nothrow) CComThread() is NULL");
				return false;
			}
			if (!m_pThreadList->push_back(pComThread, &idx)) {
				comErrorPrint("m_pThreadList->push_back");
				return false;
			}
			pComThread->setIdx(idx);
		}
	}
	return true;
}


bool CCoreList::initLogWriter(TCHAR *pConfigFile, TCHAR *pSection, bool bSystem)
{
	int nCount, i, idx;
	CSTLogger *pWriter;
	char szErrorMsg[128];
	bool bRes;

	nCount = GetPrivateProfileInt(pSection, _T("LIST_COUNT"), 1, pConfigFile);
	if (nCount) {
		if (!m_pWriterList) {
			m_pWriterList = new CTList<CSTLogger>();
			if (!m_pWriterList->alloc(nCount + LIST_ADD_COUNT, eAlloc_Type_none)) {
				sprintf(szErrorMsg, "m_pWriterList->alloc nCnt[%d]", nCount + LIST_ADD_COUNT);
				comErrorPrint(szErrorMsg);
				return false;
			}
		}

		for (i = 0; i < nCount; i++) {
			pWriter = new (std::nothrow) CSTLogger();
			if (!pWriter) {
				sprintf(szErrorMsg, "[seq:%d] new (std::nothrow) CSTLogger() is NULL", i);
				comErrorPrint(szErrorMsg);
				return false;
			}
			if (!pWriter->initConfig(pConfigFile, i, pSection)) {
				if (bSystem) {
					bRes = pWriter->init(LIST_ADD_COUNT, 0);
				}
				if(!bRes) {
					sprintf(szErrorMsg, "[seq:%d] pWriter->init", i);
					comErrorPrint(szErrorMsg);
					return false;
				}
			}
			if (!m_pWriterList->push_back(pWriter, &idx)) {
				sprintf(szErrorMsg, "[seq:%d] pWriter->push_back", idx);
				comErrorPrint(szErrorMsg);
				return false;
			}
			pWriter->m_cThread.setIdx(idx);
		}
	}
	return true;
}


bool CCoreList::initScheduler(TCHAR *pConfigFile, TCHAR *pSection)
{
	int nCount, i, nMax, idx;
	CScheduler *pScheduler;
	char szErrorMsg[128];
	// scheduler
	nCount = GetPrivateProfileInt(pSection, _T("LIST_COUNT"), 0, pConfigFile);
	if (nCount) {
		if (!m_pSchedulerList) {
			m_pSchedulerList = new (std::nothrow) CTList<CScheduler>();
			if (!m_pSchedulerList) {
				sprintf(szErrorMsg, "new (std::nothrow) CTList<CScheduler>() is NULL");
				comErrorPrint(szErrorMsg);
				return false;
			}
			if (!m_pSchedulerList->alloc(nCount + LIST_ADD_COUNT, eAlloc_Type_none)) {
				sprintf(szErrorMsg, "m_pSchedulerList->alloc nCnt[%d]", nCount + LIST_ADD_COUNT);
				comErrorPrint(szErrorMsg);
				return false;
			}
		}

		nMax = GetPrivateProfileInt(pSection, _T("TIME_POOL_MAX"), 0, pConfigFile) + CORE_PLUS_POOL_COUNT;
		m_pIntervalPool = new (std::nothrow) CMemPool<STIntervalInfo>();
		if (!m_pIntervalPool->alloc(nMax, eAlloc_Type_alloc)) {
			printf("has Failed!! m_pIntervalPool.alloc(%d)\n", nMax);
			return false;
		}

		for (i = 0; i < nCount; i++) {
			pScheduler = new (std::nothrow) CScheduler();
			if (!pScheduler) {
				sprintf(szErrorMsg, "[seq:%d] new (std::nothrow) CScheduler() is NULL", i);
				comErrorPrint(szErrorMsg);
				return false;
			}
			if (!pScheduler->initConfig(pConfigFile, i, pSection)) {
				sprintf(szErrorMsg, "[seq:%d] pScheduler->init", i);
				comErrorPrint(szErrorMsg);
				return false;
			}
			if (!m_pSchedulerList->push_back(pScheduler, &idx)) {
				sprintf(szErrorMsg, "[seq:%d] pScheduler->push_back", idx);
				comErrorPrint(szErrorMsg);
				return false;
			}
			pScheduler->m_cThread.setIdx(idx);
		}
	}
	return true;
}


bool CCoreList::initQueueThread(TCHAR *pConfigFile, TCHAR *pSection)
{
	int nCount, i, nMax, idx, nTmp;
	CComQueueThread *pEventThread;
	nCount = GetPrivateProfileInt(pSection, _T("LIST_COUNT"), 0, pConfigFile);
	if (nCount)
	{
		m_pEventThreadList = new CTList<CComQueueThread>();
		if (!m_pEventThreadList->alloc(nCount + LIST_ADD_COUNT, eAlloc_Type_none)) {
			printf("has Failed!! m_pEventThreadList = new CTList<CComQueueThread>()\n");
			return false;
		}
		nTmp = nCount * 20;
		nMax = GetPrivateProfileInt(pSection, _T("EVENT_POOL_MAX"), nTmp, pConfigFile) + CORE_PLUS_POOL_COUNT;
		m_pEventPool = new CMemPool<STEvent>();
		if (!m_pEventPool->alloc(nMax, eAlloc_Type_alloc)) {
			printf("has Failed!! m_pEventPool.alloc(%d)\n", nMax);
			return false;
		}
		for (i = 0; i < nCount; i++) {
			pEventThread = new (std::nothrow) CComQueueThread();
			if (!pEventThread) {
				printf("has Failed!! [seq:%d] pEventThread = new (std::nothrow) CComQueueThread()\n", i);
				return false;
			}
			if (!pEventThread->initConfig(pConfigFile, i, pSection)) {
				printf("has Failed!! [seq:%d] pEventThread->init()\n", i);
				return false;
			}

			if (!m_pEventThreadList->push_back(pEventThread, &idx)) {
				printf("has Failed!! [seq:%d] m_pEventThreadList->setNewObj(pScheduler)\n", i);
				return false;
			}
			pEventThread->m_cThread.setIdx(idx);
		}
	}
	return true;
}



bool CCoreList::initLogger(CLogger *pLogger, TCHAR *pConfig, TCHAR *pSection)
{
	E_LOG_INIT_RES eRes;
	if (!pLogger) {
		_stprintf(g_szMessage, _T("pLogger is NULL! [%s:%s]"), pConfig, pSection);
		comErrorPrint(g_szMessage);
		return false;
	}
	eRes = pLogger->initConfig(pConfig, pSection);

	switch (eRes) {
		case LOG_INIT_CHANGED_SUCCESS: 
		case LOG_INIT_NO_FILE: return true;
		case LOG_INIT_ERROR:
		{
			_stprintf(g_szMessage, _T("pLogger->initConfig has Failed!!![%s:%s]"), pConfig, pSection);
			comErrorPrint(g_szMessage);
			return false;
		}
		case LOG_INIT_SUCCESS: 
		{
			CSTLogger *pWriter = m_pWriterList->getObj(pLogger->getSTLNo());
			if (!pWriter) {
				_stprintf(g_szMessage, _T("pWriter is NULL!!![%s:%s] [nSTLNo:%d]"), pConfig, pSection, pLogger->getSTLNo());
				comErrorPrint(g_szMessage);
				return false;
			}
			pWriter->procLoggerEnable(pLogger);
		}
		break;
	}
	return true;
}

bool CCoreList::initLogger(CLogger *pLogger)
{
	if (!pLogger) {
		comErrorPrint(g_szMessage);
		return false;
	}

	CSTLogger *pWriter = m_pWriterList->getObj(pLogger->getSTLNo());
	if (!pWriter) {
		_stprintf(g_szMessage, _T("pWriter is NULL!!![nSTLNo:%d]\n"), pLogger->getSTLNo());
		comErrorPrint(g_szMessage);
		return false;
	}

	pWriter->procLoggerEnable(pLogger);

	return true;
}


bool CCoreList::setLoggerEnable(CLogger *pLogger)
{
	CSTLogger *pWriter = m_pWriterList->getObj(pLogger->getSTLNo());
	if (!pWriter) {
		_stprintf(g_szMessage, _T("pWriter is NULL! pLogger[nSTLNo:%d]"), pLogger->getSTLNo());
		comErrorPrint(g_szMessage);
		return false;
	}
	pLogger->setLogFlag(IS_LOG_ENABLE);
	STLCommand *pBuf = (STLCommand *)gs_pMMgr->newBuf(sizeof(STLCommand));
	pBuf->nCommandCode = eLCode_ENABLE;
	pBuf->p = pLogger;
	pWriter->pushCommand(pBuf);
	return true;
}

bool CCoreList::setLoggerDisable(CLogger *pLogger)
{
	CSTLogger *pWriter = m_pWriterList->getObj(pLogger->getSTLNo());
	if (!pWriter) {
		_stprintf(g_szMessage, _T("pWriter is NULL! pLogger[nSTLNo:%d]"), pLogger->getSTLNo());
		comErrorPrint(g_szMessage);
		return false;
	}
	pLogger->clearLogFlag(IS_LOG_ENABLE);
	STLCommand *pBuf = (STLCommand *)gs_pMMgr->newBuf(sizeof(STLCommand));
	pBuf->nCommandCode = eLCode_DISABLE;
	pBuf->p = pLogger;
	pWriter->pushCommand(pBuf);
	return true;
}



bool CCoreList::start()
{
	int i, nCount;
	CSTLogger *pWriter;
	CScheduler *pScheduler;
	CComQueueThread *pEventThread;
	CComThread *pComThread;	


	CDateTime curTm;
	gs_today_time.date		= _ttoi(curTm.DateString(0));
	gs_today_time.year		= curTm.Year();
	gs_today_time.month		= curTm.Month();
	gs_today_time.mday		= curTm.Mday();
	gs_today_time.hour		= curTm.Hour();
	gs_today_time.minute	= curTm.Minute();

	// STLogger
	if (m_pWriterList)
	{
		nCount = m_pWriterList->size();
		for (i = 0; i < nCount; i++) {
			pWriter = m_pWriterList->getObj(i);
			if (pWriter) {
				if (!pWriter->start()) {
					goto ERROR_STARTPROC;
				}
			}
		}
	}

	// CScheduler
	if (m_pSchedulerList)
	{
		nCount = m_pSchedulerList->size();
		for (i = 0; i < nCount; i++) {
			pScheduler = m_pSchedulerList->getObj(i);
			if (pScheduler) {
				if (!pScheduler->start()) {
					goto ERROR_STARTPROC;
				}
			}
		}
	}

	// Event
	if (m_pEventThreadList)
	{
		nCount = m_pEventThreadList->size();
		for (i = 0; i < nCount; i++) {
			pEventThread = m_pEventThreadList->getObj(i);
			if (pEventThread) {
				if (!pEventThread->start()) {
					goto ERROR_STARTPROC;
				}
			}
		}
	}

	// comThread
	if (m_pThreadList)
	{
		nCount = m_pThreadList->size();
		for (i = 0; i < nCount; i++) {
			pComThread = m_pThreadList->getObj(i);
			if (pComThread) {
				if (!pComThread->start()) {
					goto ERROR_STARTPROC;
				}
			}
		}
	}
	return true;

ERROR_STARTPROC:
	destroy();
	return false;
}


bool CCoreList::putEvent(STEvent *p)
{
	CComQueueThread *pQueueThread = m_pEventThreadList->getObj(p->nThreadIdx);
	if (!pQueueThread) {
		return false;
	}
	if (!pQueueThread->putQueue(p)) {
		return false;
	}
	return true;
}

bool CCoreList::putTimer(STIntervalInfo *p)
{
	CScheduler *pSchedulerThread = m_pSchedulerList->getObj(p->nThreadIdx);
	if (!pSchedulerThread) {
		return false;
	}
	if (!pSchedulerThread->addTime(p)) {
		return false;
	}
	return true;
}


int CCoreList::addComThread(fp_LoopProc fpProc, void *pClass, int millisec, void *pObj)
{
	CComThread *pThread;
	int idx;
	if (!m_pThreadList) {
		if (!initComThread(LIST_ADD_COUNT)) return -1;
	}

	pThread = new (std::nothrow) CComThread();
	if (!pThread) {
		_stprintf(g_szMessage, _T("[lastIdx:%d] new CComThread"), m_pThreadList->lastIdx());
		comErrorPrint(g_szMessage);
		goto ERROR_ADDCOMTHREAD;
	}

	if (!pThread->init(fpProc, pClass, millisec, pObj)) {
		_stprintf(g_szMessage, _T("[lastIdx:%d] pComThread->init()"), m_pThreadList->lastIdx());
		comErrorPrint(g_szMessage);
		goto ERROR_ADDCOMTHREAD;
	}

	if (!m_pThreadList->push_back(pThread, &idx)) {
		_stprintf(g_szMessage, _T("[idx:%d] m_pThreadList->push_back()"), idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDCOMTHREAD;
	}
	pThread->setIdx(idx);
	if (!pThread->start()) {
		m_pThreadList->del(idx);
		_stprintf(g_szMessage, _T("[idx:%d] pComThread->start()"), idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDCOMTHREAD;
	}
	return idx;

ERROR_ADDCOMTHREAD:
	delete pThread;
	return -1;
}

bool CCoreList::delComThread(int nSeq)
{
	CComThread *pThread = m_pThreadList->getObj(nSeq);
	if (pThread) {
		m_pThreadList->del(nSeq);
		delete pThread;
		return true;
	}
	return false;
}

int CCoreList::addLogWriter(int nCount)
{
	int idx;

	if (!m_pWriterList) {
		if (!initLogWriter(LIST_ADD_COUNT)) return -1;
	}

	CSTLogger *pWriter = new (std::nothrow) CSTLogger();
	if (!pWriter) {
		comErrorPrint("pWriter = new (std::nothrow) CSTLogger()");
		return -1;
	}
	if (!m_pWriterList->push_back(pWriter, &idx)) {
		_stprintf(g_szMessage, _T("[idx:%d] pWriter->push_back(pWriter)"), idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDWRITER;
	}
	if (!pWriter->init(nCount, idx)) {
		m_pWriterList->del(idx);
		_stprintf(g_szMessage, _T("pWriter->init(nCount:%d, idx:%d)"), nCount, idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDWRITER;
	}
	if (!pWriter->start()) {
		m_pWriterList->del(idx);
		_stprintf(g_szMessage, _T("[idx:%d] pWriter->start()!"), idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDWRITER;
	}
	return idx;

ERROR_ADDWRITER:
	delete pWriter;
	return -1;
}

int CCoreList::addScheduler(int nCount, int millisecWait)
{
	int idx;

	if (!m_pSchedulerList) {
		if (!initScheduler(LIST_ADD_COUNT)) {
			return -1;
		}
	}

	CScheduler *pScheduler = new (std::nothrow) CScheduler();
	if (!pScheduler) {
		comErrorPrint("pScheduler = new (std::nothrow) CScheduler()");
		return -1;
	}
	if (!m_pSchedulerList->push_back(pScheduler, &idx)) {
		_stprintf(g_szMessage, _T("[idx:%d] pScheduler->push_back(pWriter)"), idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDScheduler;
	}
	if (!pScheduler->init(nCount, millisecWait, idx)) {
		m_pSchedulerList->del(idx);
		_stprintf(g_szMessage, _T("pScheduler->init(nCount:%d, millisecWait:%d, idx:%d)"), nCount, millisecWait, idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDScheduler;
	}
	if (!pScheduler->start()) {
		m_pSchedulerList->del(idx);
		_stprintf(g_szMessage, _T("[idx:%d] pScheduler->start()"), idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDScheduler;
	}
	return idx;

ERROR_ADDScheduler:
	delete pScheduler;
	return -1;
}

int CCoreList::addQueueThread(int nCount)
{
	int idx;

	if (!m_pEventThreadList) {
		if (!initQueueThread(LIST_ADD_COUNT)) return -1;
	}

	CComQueueThread *pEventThread = new (std::nothrow) CComQueueThread();
	if (!pEventThread) {
		comErrorPrint("FAIL! pEventThread = new (std::nothrow) CComQueueThread()");
		return -1;
	}
	if (!m_pEventThreadList->push_back(pEventThread, &idx)) {
		_stprintf(g_szMessage, _T("[idx:%d] m_pEventThreadList->push_back(pEventThread)"), idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDEvent;
	}
	if (!pEventThread->init(nCount, idx)) {
		m_pEventThreadList->del(idx);
		_stprintf(g_szMessage, _T("pEventThread->init(nCount:%d, idx:%d)"), nCount,idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDEvent;
	}
	if (!pEventThread->start()) {
		m_pEventThreadList->del(idx);
		_stprintf(g_szMessage, _T("[idx:%d] pEventThread->start()! "), idx);
		comErrorPrint(g_szMessage);
		goto ERROR_ADDEvent;
	}
	return idx;

ERROR_ADDEvent:
	delete pEventThread;
	return -1;
}

bool CCoreList::initComThread(int nThreadCount)
{
	char szErrorMsg[128];

	if (!m_pThreadList) {
		m_pThreadList = new (std::nothrow) CTList<CComThread>();
		if (!m_pThreadList->alloc(nThreadCount + LIST_ADD_COUNT, eAlloc_Type_none)) {
			sprintf(szErrorMsg, "m_pThreadList->alloc(%d)", nThreadCount + LIST_ADD_COUNT);
			comErrorPrint(szErrorMsg);
			return false;
		}
	}
	return true;
}
bool CCoreList::initLogWriter(int nThreadCount)
{
	char szErrorMsg[128];
	if (!m_pWriterList) {
		m_pWriterList = new (std::nothrow) CTList<CSTLogger>();
		if (!m_pWriterList->alloc(nThreadCount + LIST_ADD_COUNT, eAlloc_Type_none)) {
			sprintf(szErrorMsg, "m_pWriterList->alloc(%d)", nThreadCount + LIST_ADD_COUNT);
			comErrorPrint(szErrorMsg);
			return false;
		}
	}
	return true;
}
bool CCoreList::initScheduler(int nThreadCount, int nSchedulerPoolCount)
{
	char szErrorMsg[128];
	if (!m_pSchedulerList) {
		m_pSchedulerList = new CTList<CScheduler>();
		if (!m_pSchedulerList->alloc(nThreadCount + LIST_ADD_COUNT, eAlloc_Type_none)) {
			sprintf(szErrorMsg, "m_pSchedulerList->alloc(%d)", nThreadCount + LIST_ADD_COUNT);
			comErrorPrint(szErrorMsg);
			return false;
		}
	}
	if (nSchedulerPoolCount) {
		if (!m_pIntervalPool) {
			m_pIntervalPool = new CMemPool<STIntervalInfo>();
			if (!m_pIntervalPool->alloc(nSchedulerPoolCount, eAlloc_Type_alloc)) {
				sprintf(szErrorMsg, "has Failed!! m_pIntervalPool->alloc(%d)\n", nSchedulerPoolCount);
				comErrorPrint(szErrorMsg);
				return false;
			}
		}
	}
	return true;
}

bool CCoreList::initQueueThread(int nThreadCount, int nQueuePoolCount)
{
	char szErrorMsg[128];
	if (!m_pEventThreadList) {
		m_pEventThreadList = new CTList<CComQueueThread>();
		if (!m_pEventThreadList->alloc(nThreadCount+LIST_ADD_COUNT, eAlloc_Type_none)) {
			sprintf(szErrorMsg, "m_pEventThreadList->alloc(%d)", nThreadCount + LIST_ADD_COUNT);
			comErrorPrint(szErrorMsg);
			return false;
		}
	}
	if (nQueuePoolCount)
	{
		if (!m_pEventPool) {
			m_pEventPool = new CMemPool<STEvent>();
			if (!m_pEventPool->alloc(nQueuePoolCount, eAlloc_Type_alloc)) {
				sprintf(szErrorMsg, "has Failed!! m_pEventPool->alloc(%d)\n", nQueuePoolCount);
				comErrorPrint(szErrorMsg);
				return false;
			}
		}

	}
	return true;
}

