#include "Analysor_BetweenLines.h"



CAnalysor_BetweenLines::CAnalysor_BetweenLines()
{
	m_pClassList = NULL;
}

CAnalysor_BetweenLines::~CAnalysor_BetweenLines()
{
	int i = 0;
	STBetween_Class *pStruct;
	if (m_pClassList) {
		pStruct = (STBetween_Class *)m_pClassList->getObj(i++);
		while (pStruct) {
			if (pStruct->pKeyword[0]) {
				gs_pMMgr->delString(pStruct->pKeyword[0]);
			}
			if (pStruct->pKeyword[1]) {
				gs_pMMgr->delString(pStruct->pKeyword[1]);
			}
			pStruct = (STBetween_Class *)m_pClassList->getObj(i++);
		}
		delete m_pClassList;
		m_pClassList = NULL;
	}
}

bool CAnalysor_BetweenLines::initConfig(char *pConfigFile, char *pSector)
{
	int nRes, nCount, i, j;
	char szBuf[128], szKey[2][24];
	STBetween_Class *pClass;

	nCount = GetPrivateProfileInt(pSector, _T("CLASS_COUNT"), 0, pConfigFile);

	if (!nCount) return false;

	m_pClassList = new (std::nothrow) CSList();
	m_pClassList->alloc(nCount, eAlloc_Type_alloc);

	for (i = 0; i < nCount; i++) {
		sprintf(szKey[0], "CLASS_%d_KEYWORD_1", i);
		sprintf(szKey[1], "CLASS_%d_KEYWORD_2", i);

		for (j = 0; j < BPOS_LINE_MAX; j++) {
			nRes = GetPrivateProfileString(pSector, szKey[j], _T(""), szBuf, sizeof(szBuf), pConfigFile);
			if (nRes) {
				pClass = (STBetween_Class *)calloc(1, sizeof(STBetween_Class));
				pClass->pKeyword[j] = gs_pMMgr->newString(szBuf);
			}
			else {
				sprintf(g_szMessage, "szKey[%s] is NULL\n", szKey[j]);
				comErrorPrint(g_szMessage);
				return false;
			}
		}

		m_pClassList->push_back(pClass);
	}
	CAnalysor::setAvailable(true);
	return true;
}

bool CAnalysor_BetweenLines::parsingLine(char *pLine)
{
	int i=0;
	STBetween_Class *pClass;
	if (m_pClassList) {
		pClass = (STBetween_Class *)m_pClassList->getObj(i);
		while (pClass)
		{
			parsingClass(pLine, pClass);
			i++;
			pClass = (STBetween_Class *)m_pClassList->getObj(i);
		}
	}
	return true;
}

void CAnalysor_BetweenLines::parsingClass(char *pLine, STBetween_Class *pClass)
{
	char *pPos;
	STDTime stRes, *pPreTime = &pClass->stPreTime;
	STInterval *pInterval = &pClass->stInterval;


	pPos = ehstrstr(pLine, pClass->pKeyword[BPOS_LINE_1]);

	if (pPos) { // LINE_0
		memcpy(pPreTime, &g_stConfig.stCurTime, g_nTimeSize);
	}
	else { // LINE_1

		pPos = ehstrstr(pLine, pClass->pKeyword[BPOS_LINE_2]);

		if (pPos) {
			getTimeInterval(pPreTime, &g_stConfig.stCurTime, &stRes);
			pInterval->nInterval[pInterval->nIntervalIdx++] = getMiliseconds(&stRes, g_stConfig.pDTE->nLastLen);

			if (pInterval->nIntervalIdx == AVG_COUNT) {
				pInterval->nAvg[pInterval->nAvgIdx++] = getAvg(AVG_COUNT, pInterval->nInterval);
				pInterval->nIntervalIdx = 0;
			}
		}
	}
}



void CAnalysor_BetweenLines::report(CLogger *pLogger)
{

	int i = 0, nPos = 0;
	STBetween_Class *pStruct;
	char szBuf[1024];
	float fAvg;
	STBuf stStringBuf;
	
	if (m_pClassList) {

		nPos += sprintf(szBuf + nPos, "\n\n################################################# \n");
		nPos += sprintf(szBuf + nPos, "  BetweenLines Report : \n");
		nPos += sprintf(szBuf + nPos, "################################################# \n");
		pLogger->LogPrint(szBuf); nPos = 0;


		pStruct = (STBetween_Class *)m_pClassList->getObj(i);
		while (pStruct) {
			if (pStruct->pKeyword) {
				if (intervalReport(&pStruct->stInterval, &stStringBuf, &fAvg)) {
					sprintf(szBuf, "\n S%d[%s] Interval Avg : %f\n", i, pStruct->pKeyword[i], fAvg);
					pLogger->LogPrint(szBuf);
					pLogger->LogPrint(stStringBuf.pValue);
					gs_pMMgr->delBufByIndex(stStringBuf.pValue, stStringBuf.nIdx);
				}
			}
			i++;
			pStruct = (STBetween_Class *)m_pClassList->getObj(i);
		}
	}
}