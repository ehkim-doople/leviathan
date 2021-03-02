#include "DirectoryProc.h"
#include "comQSort.h"

CDirectoryProc *g_pDProc;
STDTime g_stProcStartTime;
STDTime g_stProcEndTime;

CDirectoryProc::CDirectoryProc()
{
	m_pFileList = NULL;
}

CDirectoryProc::~CDirectoryProc()
{
	if (m_pFileList) {
		if (m_pFileList->pList) {
			delete m_pFileList->pList;
			m_pFileList->pList = NULL;
		}
		delete m_pFileList;
		m_pFileList = NULL;
	}

}

int CDirectoryProc::getTimeAscendingFileList(char *pDir, STSortData **pResult)
{
	STSortData *pSortData = NULL, *pSortDataS;
	STFileInfoEx *pFileInfo;
	CSList *pList;
	STFileList stList;
	STFilterData stFilter;
	int i, nSize, nCount;
	stList.pList = NULL;
	stList.root = pDir;
	stList.nFlag = 0;
	if (g_stConfig.pSourceFile) {
		stFilter.pData = g_stConfig.pSourceFile;
		stFilter.pFunc = filter_stringCompare;
		if (!CFileUtil::GetFileList(&stList, &stFilter)) return 0;
	}
	else {
		if (!CFileUtil::GetFileList(&stList, NULL)) return 0;
	}
	pList = stList.pList;
	nCount = pList->size();
	nSize = sizeof(STSortData) * nCount;
	pSortData = (STSortData *)gs_pMMgr->newBuf(nSize);
	if (!pSortData) {
		return 0;
	}
	pSortDataS = pSortData;
	for (i = 0; i < nCount; i++) {
		pFileInfo = (STFileInfoEx *)pList->getObj(i);
		pSortData->nValue = pFileInfo->stFileTime.getDateTime();
		pSortData->p = pFileInfo;
		pSortData++;
	}
	SortEx(pSortDataS, nCount);
	*pResult = pSortDataS;
	memcpy(m_pFileList, &stList, sizeof(STFileList));
	return nCount;
}

bool CDirectoryProc::init()
{
	m_pFileList = new (std::nothrow) STFileList;
	if (!m_pFileList) return false;

	return true;
}


bool CDirectoryProc::verificationNSetFileProc()
{
	bool bRes = false;
	int nLineType;
	if (g_stConfig.pDTE) {
		if (g_stConfig.pLineEndKey) {
			nLineType = 2;
		}
		else {
			nLineType = 1;
		}
	}
	else nLineType = 3;

	switch (nLineType) {
	case 1: m_fpFileProc = &CFileProc::FileProc;	bRes = true; break;
	case 2: m_fpFileProc = &CFileProc::FileProc2;	bRes = true; break;
	}

	if (!bRes) {
		comErrorPrint("verificationNSetFileProc has failed");
		return false;
	}
	return true;
}

bool CDirectoryProc::proc(char *pDir)
{
	int i, nCount;

	CFileProc cFileProc;
	char szFileName[256];
	STSortData *pSortData = NULL;
	STFileInfoEx *pFileInfo;

	nCount = getTimeAscendingFileList(pDir, &pSortData);

	if (!nCount) return false;

	if (!verificationNSetFileProc()) return false;

	for (i = 0; i < nCount; i++)
	{
		pFileInfo = (STFileInfoEx *)pSortData[i].p;
		if (pFileInfo->stat == 'F') {
			gs_cLogger.PutLogQueue(LEVEL_TRACE, _T("readFILEStart [%s] [%s]"), pDir, pFileInfo->fname);
			sprintf(szFileName, "%s%s", pDir, pFileInfo->fname);
			if (cFileProc.init(szFileName, pFileInfo->nSize)) {
				(cFileProc.*m_fpFileProc)();
				gs_cLogger.PutLogQueue(LEVEL_TRACE, _T("FILEProcComplete fileSize[%d]"), cFileProc.getFileSize());
			}
		}
	}
	int nSize = sizeof(STSortData) * nCount;
	gs_pMMgr->delBuf((char*)pSortData, nSize);
	pSortData = NULL;
	Dreport();
	return true;
}


void CDirectoryProc::Dreport()
{
	CLogger *pLogger = g_stConfig.pReport;
	if (!pLogger) {
		comErrorPrint("g_stConfig.pReport is NULL");
		return ;
	}
	char szBuf[1024];
	STDTime stProcTime;

	// TODO : Analyzed log size, Number of logs analyzed


	getCurrentTime(&g_stProcEndTime);

	// Analyzer run time
	pLogger->LogPrint("### REPORT ### \n\n");
	getTimeInterval(&g_stProcStartTime, &g_stProcEndTime, &stProcTime);
	sprintf(szBuf, "  Analyzer run time : [%d]hours [%d]minutes [%d]seconds [%d]milliseconds\n",
		stProcTime.nHour, stProcTime.nMinute, stProcTime.nSec, stProcTime.nLast);
	pLogger->LogPrint(szBuf);

	// Duration of log accumulation
	getTimeInterval(&g_stConfig.stStartTime, &g_stConfig.stEndTime, &g_stConfig.stTotTime);
	sprintf(szBuf, "  Duration of log accumulation : [%d] days [%d]hours [%d]minutes [%d]seconds [%d]milliseconds\n", 
		g_stConfig.stTotTime.nDay, g_stConfig.stTotTime.nHour, g_stConfig.stTotTime.nMinute, g_stConfig.stTotTime.nSec, g_stConfig.stTotTime.nLast);
	pLogger->LogPrint(szBuf);

	g_pHandle->Report(pLogger);

	//printf("print Result ... please wait..\n");
	//SLEEP(10);
}