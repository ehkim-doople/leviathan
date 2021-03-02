#include "handler.h"
#include "comCore.h"

STConfig g_stConfig;
CHandler *g_pHandle;

CHandler::CHandler()
{
	memset(&g_stConfig, 0, sizeof(STConfig));
	memset(m_pAnalyzerList, 0, sizeof(m_pAnalyzerList));
}

CHandler::~CHandler()
{
	int i;

	for (i = 0; i < eAType_Max; i++)
	{
		if (m_pAnalyzerList[i]) {
			delete m_pAnalyzerList[i];
			m_pAnalyzerList[i] = NULL;
		}
	}
}

void setDefaultConfig(STLogInfo *pInfo) {
	pInfo->nDateTimeFormat = LOG_DATE_NONE;
	pInfo->szKeepDuration[0] = 0;
	pInfo->nSTLNo = eLDOPT_STL_NO;
	pInfo->nQueueListCount = eLDOPT_LISTMAX;
	pInfo->nSizeLimit = 50000000;
	pInfo->nLogLevel = LEVEL_INFO;
	pInfo->nLogType = LOG_TYPE_FILE;
	pInfo->bIsPrint = false;
	pInfo->bIsLevelPrint = false;
}


bool CHandler::init()
{
	char szSector[64], szName[256], szBuf[256];
	bool res = true;
	int nRes;
	STLogInfo stInfo;

	if (!gs_pCore) {
		gs_pCore = new (std::nothrow) CCoreList();
		if (!gs_pCore) {
			comErrorPrint("gs_pCore alloc has Failed!!!");
			return false;
		}
		gs_pCore->initDefaultPool(12, 256, 0);
	}

	strcpy(szSector, "RANGE");
	g_stConfig.nSourceFlag = GetPrivateProfileInt(szSector, _T("SOURCE_FLAG"), eSourceFlag_FILE, g_pProcessConfig);
	if (g_stConfig.nSourceFlag == eSourceFlag_FILE) {
		nRes = GetPrivateProfileString(szSector, _T("SOURCE_FILENAME"), _T(""), szName, sizeof(szName), g_pProcessConfig);
		if (!nRes) {
			comErrorPrint("config SOURCE_FILENAME is NULL");
			return false;
		}
		g_stConfig.pSourceFile = gs_pMMgr->newBuf(nRes + 1);
		strcpy(g_stConfig.pSourceFile, szName);
	}
	else if(g_stConfig.nSourceFlag == eSourceFlag_DIRECTORY) {
		nRes = GetPrivateProfileString(szSector, _T("SOURCE_DIRECTORY"), _T(""), szName, sizeof(szName), g_pProcessConfig);
		if (!nRes) {
			comErrorPrint("config SOURCE_DIRECTORY is NULL");
			return false;
		}
		g_stConfig.pSourceDirectory = gs_pMMgr->newBuf(nRes + 1);
		strcpy(g_stConfig.pSourceDirectory, szName);
	}
	else {
		comErrorPrint("config source flag is NONE");
		return false;
	}

	char szLogName[64], szExtesion[12];
	nRes = GetPrivateProfileInt(szSector, _T("IS_REPORT_FILE"), 1, g_pProcessConfig);
	if (nRes) {
		g_stConfig.pReport = new CLogger();
		if (g_stConfig.pReport) {
			setDefaultConfig(&stInfo);
			GetPrivateProfileString(szSector, _T("REPORT_PATHNAME"), _T("~\\Report\\result.txt"), szName, sizeof(szName), g_pProcessConfig);
			detachFullPath(szName, szBuf, szLogName, szExtesion);
			stInfo.pLogDir = szBuf;
			stInfo.pLogName = szLogName;
			stInfo.pFileExtension = szExtesion;
			if (LOG_INIT_SUCCESS == g_stConfig.pReport->initLog(&stInfo)) {
				if (!gs_pCore->initLogger(g_stConfig.pReport)) return false;
			}
		}
	}

	nRes = GetPrivateProfileInt(szSector, _T("IS_ERROR_FILE"), 1, g_pProcessConfig);
	if (nRes) {
		g_stConfig.pExtractLog = new CLogger();
		if (g_stConfig.pExtractLog) {
			setDefaultConfig(&stInfo);
			GetPrivateProfileString(szSector, _T("ERROR_PATHNAME"), _T("~\\Report\\Extract.log"), szName, sizeof(szName), g_pProcessConfig);
			detachFullPath(szName, szBuf, szLogName, szExtesion);
			stInfo.pLogDir = szBuf;
			stInfo.pLogName = szLogName;
			stInfo.pFileExtension = szExtesion;
			if (LOG_INIT_SUCCESS == g_stConfig.pExtractLog->initLog(&stInfo)) {
				if (!gs_pCore->initLogger(g_stConfig.pExtractLog)) return false;
			}
		}
	}

	nRes = GetPrivateProfileInt(_T("LINE_DEFINE"), _T("IS_START_TIMESTEMP"), 1, g_pProcessConfig);
	if (nRes) {
		nRes = GetPrivateProfileString(szSector, _T("DATETIME_EXPRESSION"), _T("[yyyy/mm/dd hh:mm:ss:3]"), szBuf, sizeof(szBuf), g_pProcessConfig);
		if (nRes) {
			g_stConfig.pDTE = (STDateTimeExp*)calloc(1, sizeof(STDateTimeExp));
			if (!parsingDateTimeExp(szBuf, g_stConfig.pDTE)) {
				sprintf(g_szMessage, "parsingDateTimeExp has failed! DATETIME_EXPRESSION[%s]", szBuf);
				comErrorPrint(g_szMessage);
				return false;
			}
		}
	}
	
	// Line
	nRes = GetPrivateProfileString(_T("LINE_DEFINE"), _T("START_LINE_KEY"), _T(""), szName, sizeof(szName), g_pProcessConfig);
	if (nRes) {
		g_stConfig.pLineStartKey = gs_pMMgr->newString(szName);
	}
	nRes = GetPrivateProfileString(_T("LINE_DEFINE"), _T("END_LINE_KEY"), _T(""), szName, sizeof(szName), g_pProcessConfig);
	if (nRes) {
		g_stConfig.pLineEndKey = gs_pMMgr->newString(szName);
	}
	g_stConfig.nSkippingLen = GetPrivateProfileInt(_T("LINE_DEFINE"), _T("IS_EXTRACT_FILE"), 0, g_pProcessConfig);
	res = initAnalistConfig();

//END_PROC:
#ifndef WIN32
	//confEntryDestroy();
#endif
	return res;
}


bool CHandler::initAnalistConfig()
{
	CAnalyzer cAnalyzer;
	char szSector[32];
	int i;
	bool bRes;
	for (i = 0; i < eAType_Max; i++)
	{
		sprintf(szSector, "ANALYSIS_%d", i);
		cAnalyzer.initConfig(g_pProcessConfig, szSector);
		if (cAnalyzer.isAvailable()) {
			switch (i) {
			case eAType_KeyValue		: m_pAnalyzerList[i] = new CAnalyzer_KeyValue();		break;
			case eAType_ExtractNToFile	: m_pAnalyzerList[i] = new CAnalyzer_ExtractNToFile();  break;
			case eAType_DateTime		: m_pAnalyzerList[i] = new CAnalyzer_DateTime();		break;
			case eAType_BetweenLines	: m_pAnalyzerList[i] = new CAnalyzer_BetweenLines();	break;
			case eAType_Average			: m_pAnalyzerList[i] = new CAnalyzer_Average();			break;
			}
			bRes = m_pAnalyzerList[i]->initConfig(g_pProcessConfig, szSector);
			if (!bRes) {
				sprintf(g_szMessage, "Analyzer->initConfig has Failed! g_pProcessConfig[%s] szSector[%s]", g_pProcessConfig, szSector);
				comErrorPrint(g_szMessage);
				return false;
			}
		}
	}
	return true;
}


bool CHandler::parsingLine(char *pLine)
{
	int i;
	for (i = 0; i < eAType_Max; i++)
	{
		if (m_pAnalyzerList[i]) {
			if (!m_pAnalyzerList[i]->parsingLine(pLine)) {
				sprintf(g_szMessage, "Analyzer Index:%d\n", i);
				comErrorPrint(g_szMessage);
				return false;
			}
		}
	}
	return true;
}

void CHandler::Report(CLogger *pLogger)
{
	int i;
	for (i = 0; i < eAType_Max; i++)
	{
		if (m_pAnalyzerList[i]) {
			m_pAnalyzerList[i]->report(pLogger);
		}
	}
}

