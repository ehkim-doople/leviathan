#include "Analyzer_ExtractNToFile.h"
#include "comCore.h"

CAnalyzer_ExtractNToFile::CAnalyzer_ExtractNToFile()
{
	m_pExtractReport = NULL;
	m_pKeyList = NULL; 

}

CAnalyzer_ExtractNToFile::~CAnalyzer_ExtractNToFile()
{
	int i=0;
	STAnalyzer_Extract *pStruct;
	if (m_pKeyList) {
		pStruct = (STAnalyzer_Extract *)m_pKeyList->getObj(i++);
		while (pStruct) {
			if (pStruct->pKeyword) {
				gs_pMMgr->delString(pStruct->pKeyword);
			}
			pStruct = (STAnalyzer_Extract *)m_pKeyList->getObj(i++);
		}
		delete m_pKeyList;
		m_pKeyList = NULL;
	}

	if (m_pExtractReport) {
		delete m_pExtractReport;
		m_pExtractReport = NULL;
	}

}

bool CAnalyzer_ExtractNToFile::initConfig(char *pConfigFile, char *pSector)
{
	int i, nCount, nRes, nSize = sizeof(STAnalyzer_Extract);
	char szKey[32], szBuf[128];
	STAnalyzer_Extract *p;
	bool bLog = false;

	nCount = GetPrivateProfileInt(pSector, _T("KEYWORD_COUNT"), 0, pConfigFile);
	if (!nCount) return true;

	m_pKeyList = new CSList();
	if (!m_pKeyList->alloc(nCount, eAlloc_Type_alloc)) {
		sprintf(g_szMessage,"m_pKeyList->alloc has Failed nCount:[%d]\n", nCount);
		comErrorPrint(g_szMessage);
		return false;
	}

	for (i = 0; i < nCount; i++)
	{
		sprintf(szKey, "KEYWORD_%d", i);
		nRes = GetPrivateProfileString(pSector, szKey, _T(""), szBuf, sizeof(szBuf), pConfigFile);
		if (nRes) {
			p = (STAnalyzer_Extract *)calloc(1, nSize);
			if (p) {
				sprintf(szKey, "IS_LINE_TO_LOG_%d", i);
				p->pKeyword = gs_pMMgr->newString(szBuf);
				nRes = GetPrivateProfileInt(pSector, szKey, 0, pConfigFile);
				p->bLog = nRes ? true : false;
				m_pKeyList->push_back(p);
				if (p->bLog) bLog = true;
			}
			else {
				comErrorPrint("STAnalyzer_Extract calloc failed");
				return false;
			}
		}
		else {
			comErrorPrint("Check the following items in the configuration file");
			return false;
		}
	}

	// init Error Keyword Private Log
	if (bLog) {
		if (!g_stConfig.pExtractLog) {
			comErrorPrint("g_stConfig.pExtractLog is NULL");
			return false;
		}
		m_pExtractReport = g_stConfig.pExtractLog;
	}
	
	CAnalyzer::setAvailable(true);
	return true;
}

bool CAnalyzer_ExtractNToFile::parsingLine(char *pLine)
{
	int i = 0;
	STAnalyzer_Extract *pStruct;
	char *pPos = NULL;

	if (m_pKeyList) {
		pStruct = (STAnalyzer_Extract *)m_pKeyList->getObj(i);
		while (pStruct) {
			if (pStruct->pKeyword) {
				pPos = ehstrstr(pLine, pStruct->pKeyword);
				if (pPos) {
					pStruct->nTotalCount++;
					if (pStruct->bLog) {
						m_pExtractReport->LogPrint(pLine);
					}
				}
			}
			i++;
			pStruct = (STAnalyzer_Extract *)m_pKeyList->getObj(i);
		}
	}

	return true;
}


void CAnalyzer_ExtractNToFile::report(CLogger *pLogger)
{
	char *pBuf = gs_pMMgr->newBufByIndex(IDX_BUF_8k);

	int i = 0, nPos=0;
	STAnalyzer_Extract *pStruct;
	char *pPos = NULL;

	nPos += sprintf(pBuf + nPos, "\n\n################################################# \n");
	nPos += sprintf(pBuf + nPos, "  ExtractNToFile Report : \n");
	nPos += sprintf(pBuf + nPos, "################################################# \n\n");
	pLogger->LogPrint(pBuf); nPos = 0;

	if (m_pKeyList) {
		pStruct = (STAnalyzer_Extract *)m_pKeyList->getObj(i);
		while (pStruct) {
			if (pStruct->pKeyword) {
				nPos += sprintf(pBuf+nPos, "[%d] Extract Report : keyword[%s] nTotCount[%d]\n",i, pStruct->pKeyword, pStruct->nTotalCount);
			}
			i++;
			pStruct = (STAnalyzer_Extract *)m_pKeyList->getObj(i);
		}
		pLogger->LogPrint(pBuf); nPos = 0;
	}
	gs_pMMgr->delBufByIndex(pBuf, IDX_BUF_8k);
}