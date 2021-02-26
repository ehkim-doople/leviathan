#include "FileProc.h"
#include "comFileUtil.h"
#include "global.h"

CFileProc::CFileProc()
{
	m_nFileSize = sizeof(STBuf);

	memset(&m_szFileBuf, 0, sizeof(STBuf));
	memset(&m_szLine, 0, sizeof(STLine));
	m_nFileSize = 0;
}

CFileProc::~CFileProc()
{
	if (m_szFileBuf.pValue) {
		gs_pMMgr->delBuf(&m_szFileBuf);
	}
	if (m_szLine.m_szLineBuf.pValue) {
		gs_pMMgr->delBuf(&m_szLine.m_szLineBuf);
	}
	m_nFileSize = 0;
}

bool CFileProc::init(char *pReadFilePath, unsigned int nSize)
{
	m_pFilePathName = pReadFilePath;
	m_nFileSize = CFileUtil::readNalloc(pReadFilePath, &m_szFileBuf, nSize);
	if (!m_nFileSize) return false;
	if (m_nFileSize != nSize) {
		g_stConfig.pReport->LogPrint(LEVEL_ERROR, "Does not mactch!  %s, m_nFileSize[%u] nSize[%u]", pReadFilePath, m_nFileSize, nSize);
	}
	m_szLine.pSource = m_szFileBuf.pValue;
	m_szLine.pStart = g_stConfig.pLineStartKey;
	m_szLine.pEnd = g_stConfig.pLineEndKey;
	m_szLine.nSkipLen = g_stConfig.nSkippingLen;
	return true;
}

// start timestemp
// end line : g_rc
void CFileProc::FileProc()
{
	STDTime stTIme;
	parsingTimeLog(m_szLine.pSource, &stTIme, g_stConfig.pDTE);
	setBeginningTime(&g_stConfig.stStartTime, &stTIme);
	int nLineCount = 0;
	char *pNextLine;

	int nLineSize = CFileUtil::getNextLine(&m_szLine);
	while (1) {
		pNextLine = m_szLine.m_szLineBuf.pValue;
		parsingTimeLog(pNextLine, &g_stConfig.stCurTime, g_stConfig.pDTE);

		nLineCount++;
		if (!g_pHandle->parsingLine(pNextLine)) {
			printf(m_szLine.m_szLineBuf.pValue);
			printf("\n");
			sprintf(g_szMessage, "m_szLine.pStart : %s, m_szLine.pEnd : %s, m_szLine.nSkipLen:%d\n", m_szLine.pStart, m_szLine.pEnd, m_szLine.nSkipLen);
			comErrorPrint(g_szMessage);
			return;
		}
		if (!nLineSize) break;
		nLineSize = CFileUtil::getNextLine(&m_szLine);
	}
	setEndingTime(&g_stConfig.stEndTime, &g_stConfig.stCurTime);
	g_stConfig.pReport->LogPrint(LEVEL_INFO, "parsing complete!  %s, nLineCount[%d]", m_pFilePathName, nLineCount);
	g_stConfig.pReport->LogPrint(LEVEL_INFO, "%s", pNextLine);
}



// NO Start TimeStemp
void CFileProc::FileProc2()
{
	while (CFileUtil::getNextLine(&m_szLine)) {
		if (!g_pHandle->parsingLine(m_szLine.m_szLineBuf.pValue)) {
			printf(m_szLine.m_szLineBuf.pValue);
			printf("\n");
			sprintf(g_szMessage, "m_szLine.pStart : %s, m_szLine.pEnd : %s, m_szLine.nSkipLen:%d\n", m_szLine.pStart, m_szLine.pEnd, m_szLine.nSkipLen);
			comErrorPrint(g_szMessage);
			return;
		}
	}
}

