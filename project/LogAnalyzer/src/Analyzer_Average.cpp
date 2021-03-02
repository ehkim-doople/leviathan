#include "Analyzer_Average.h"


CAnalyzer_Average::CAnalyzer_Average()
{

}
CAnalyzer_Average::~CAnalyzer_Average()
{
	int i = 0;
	STAnalyzer_Avg *pStruct;
	if (m_pKeyAvgList) {
		pStruct = (STAnalyzer_Avg *)m_pKeyAvgList->getObj(i++);
		while (pStruct) {
			if (pStruct->pKeyword) {
				gs_pMMgr->delString(pStruct->pKeyword);
			}
			pStruct = (STAnalyzer_Avg *)m_pKeyAvgList->getObj(i++);
		}
		delete m_pKeyAvgList;
		m_pKeyAvgList = NULL;
	}

}


bool CAnalyzer_Average::initConfig(char *pConfigFile, char *pSector)
{
	int nRes, nCount, i;
	char szBuf[128], szKey[24];
	STAnalyzer_Avg *pAvg;

	nCount = GetPrivateProfileInt(pSector, _T("KEYWORD_COUNT"), 0, pConfigFile);

	if (!nCount) return false;

	m_pKeyAvgList = new (std::nothrow) CSList();
	m_pKeyAvgList->alloc(nCount, eAlloc_Type_alloc);

	for (i = 0; i < nCount; i++) {
		sprintf(szKey, "KEYWORD_%d", i);
		nRes = GetPrivateProfileString(pSector, szKey, _T(""), szBuf, sizeof(szBuf), pConfigFile);
		if (nRes) {
			pAvg = (STAnalyzer_Avg *)calloc(1, sizeof(STAnalyzer_Avg));
			pAvg->pKeyword = gs_pMMgr->newString(szBuf);
			m_pKeyAvgList->push_back(pAvg);
		}
	}
	CAnalyzer::setAvailable(true);
	return true;
}

bool CAnalyzer_Average::parsingLine(char *pLine)
{
	int i = 0, nValue;
	STAnalyzer_Avg *pStruct, *pRes=NULL;
	char *pPos = NULL;
	STInterval *pInterval;


	if (m_pKeyAvgList) {
		pStruct = (STAnalyzer_Avg *)m_pKeyAvgList->getObj(i);
		while (pStruct) {
			if (pStruct->pKeyword) {
				pPos = ehstrstr(pLine, pStruct->pKeyword);
				if (pPos) goto FIND_SUCCESS;
			}
			i++;
			pStruct = (STAnalyzer_Avg *)m_pKeyAvgList->getObj(i);
		}

		return false;
	}

FIND_SUCCESS :
	nValue = getInterval(pPos, pStruct->pKeyword);
	pInterval = &pStruct->stInterval;
	pInterval->nInterval[pInterval->nIntervalIdx++] = nValue * 1.0f;
	if (pInterval->nIntervalIdx == AVG_COUNT) {
		pInterval->nAvg[pInterval->nAvgIdx++] = getAvg(AVG_COUNT, pInterval->nInterval);
		pInterval->nIntervalIdx = 0;
	}

	return true;
}


void CAnalyzer_Average::report(CLogger *pLogger)
{
	int i = 0, nPos = 0;
	STAnalyzer_Avg *pStruct;
	char szBuf[1024];
	float fAvg;
	STBuf stStringBuf;

	if (m_pKeyAvgList) {

		nPos += sprintf(szBuf + nPos, "\n\n################################################# \n");
		nPos += sprintf(szBuf + nPos, "  Average Report : \n");
		nPos += sprintf(szBuf + nPos, "################################################# \n");
		pLogger->LogPrint(szBuf); nPos = 0;

		pStruct = (STAnalyzer_Avg *)m_pKeyAvgList->getObj(i);
		while (pStruct) {
			if (pStruct->pKeyword) {
				if (intervalReport(&pStruct->stInterval, &stStringBuf, &fAvg)) {
					pLogger->LogPrint("\n S%d Interval report : \n");
					sprintf(szBuf, "S%d[%s] Interval Avg : %f\n", i, pStruct->pKeyword, fAvg);
					pLogger->LogPrint(szBuf);
					pLogger->LogPrint(stStringBuf.pValue);
					gs_pMMgr->delBufByIndex(stStringBuf.pValue, stStringBuf.nIdx);
				}
			}
			i++;
			pStruct = (STAnalyzer_Avg *)m_pKeyAvgList->getObj(i);
		}
	}
}