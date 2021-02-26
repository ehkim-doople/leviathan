
#include "Analysor_KeyValue.h"
#include "global.h"

int g_nSTUnitSize;
int g_nSTLineSize;

CAnalysor_KeyValue::CAnalysor_KeyValue()
{
	g_nSTUnitSize = sizeof(STAnalysor1_UnitData);
	g_nSTLineSize = sizeof(STAnalysor1_Line);
	m_pLineList = NULL;
}

CAnalysor_KeyValue::~CAnalysor_KeyValue()
{
	int i=0;
	STAnalysor1_Line *pLine;
	if (m_pLineList) {
		pLine = (STAnalysor1_Line *)m_pLineList->getObj(i);
		while (pLine)
		{
			if (pLine->pKeyword) {
				gs_pMMgr->delString(pLine->pKeyword);
			}
			if (pLine->pValueKey) {
				gs_pMMgr->delString(pLine->pValueKey);
				if (pLine->m_pUnitDataList) delete pLine->m_pUnitDataList;
			}
			//free(pLine);
			i++;
			pLine = (STAnalysor1_Line *)m_pLineList->getObj(i);
		}


		delete m_pLineList;
		m_pLineList = NULL;
	}
}

bool CAnalysor_KeyValue::initConfig(char *pConfigFile, char *pSector)
{
	int nRes, nCount, i, nValue;
	char szBuf[128], szBuf2[128], szKey[3][24];

	nCount = GetPrivateProfileInt(pSector, _T("LINE_COUNT"), 0, pConfigFile);

	if (!nCount) return false;

	m_pLineList = new (std::nothrow) CSList();
	m_pLineList->alloc(nCount, eAlloc_Type_alloc);

	for (i = 0; i < nCount; i++) {
		sprintf(szKey[0], "LINE_%d_START", i);
		sprintf(szKey[1], "LINE_%d_VALUE_KEY", i);
		sprintf(szKey[2], "LINE_%d_TOTCNT_EXPECTED", i);
		nRes = GetPrivateProfileString(pSector, szKey[0], _T(""), szBuf, sizeof(szBuf), pConfigFile);
		if (nRes) {
			nRes = GetPrivateProfileString(pSector, szKey[1], _T(""), szBuf2, sizeof(szBuf2), pConfigFile);
			nValue = GetPrivateProfileInt(pSector, szKey[2], 0, pConfigFile);
			if (nRes) {
				initLine(szBuf, szBuf2, nValue);
			}
			else {
				initLine(szBuf, NULL, nValue);
			}
		}
	}
	CAnalysor::setAvailable(true);
	return true;
}

void CAnalysor_KeyValue::initLine(char *pLineStartKey, char *pValueKey, int nTotCnt_expected)
{
	char *pToken = gs_pMMgr->newString(pLineStartKey);
	if (!pToken) return;

	STAnalysor1_Line *pSTLine = (STAnalysor1_Line *)calloc(1, g_nSTLineSize);
	memset(pSTLine, 0, g_nSTLineSize);
	pSTLine->pKeyword = pToken;
	pSTLine->nTotCnt_expected = nTotCnt_expected;
	if (pValueKey) {
		pToken = gs_pMMgr->newString(pValueKey);
		if (!pToken) return;
		pSTLine->pValueKey = pToken;
		pSTLine->m_pUnitDataList = new CSList();
		if (pSTLine->m_pUnitDataList) {
			pSTLine->m_pUnitDataList->alloc(24, eAlloc_Type_alloc);
		}
	}
	m_pLineList->push_back(pSTLine);
}

bool CAnalysor_KeyValue::parsingLine(char *p)
{
	int i=0;
	char *pKeyPos;
	STAnalysor1_Line *pLine;
	if (m_pLineList) {
		pLine = (STAnalysor1_Line *)m_pLineList->getObj(i);
		while (pLine)
		{
			pKeyPos = ehstrstr(p, pLine->pKeyword);
			if (pKeyPos) {
				setLine(pKeyPos, pLine);
			}
			i++;
			pLine = (STAnalysor1_Line *)m_pLineList->getObj(i);
		}
	}

	return true;
}

void CAnalysor_KeyValue::setLine(char *p, STAnalysor1_Line *pSTLine)
{
	int nValue;
	if (pSTLine->pValueKey)	{
		nValue = getInterval(p, pSTLine->pValueKey);
		setUnitData(pSTLine->m_pUnitDataList, nValue);
	}
	pSTLine->nTotalLine++;
	if (pSTLine->nMin > nValue) {
		pSTLine->nMin = nValue;
	}
	else if (pSTLine->nMax < nValue) {
		pSTLine->nMax = nValue;
	}
}
STAnalysor1_UnitData *CAnalysor_KeyValue::getUnitData(CSList *pUnitList, int nValue)
{
	int i = 0;
	STAnalysor1_UnitData *pUnitData = (STAnalysor1_UnitData *)pUnitList->getObj(i);
	while (pUnitData) {
		if (pUnitData->nValue == nValue) {
			return pUnitData;
		}
		i++;
		pUnitData = (STAnalysor1_UnitData *)pUnitList->getObj(i);
	}
	return NULL;
}
void CAnalysor_KeyValue::setUnitData(CSList *pUnitList, int nValue)
{
	STAnalysor1_UnitData *pRes = getUnitData(pUnitList, nValue);

	if (!pRes) {
		pRes = (STAnalysor1_UnitData *)calloc(1, g_nSTUnitSize);
		if (!pRes) return;
		pRes->nValue = nValue;
		pUnitList->push_back(pRes);
	}
	pRes->nTotCount++;
}

void CAnalysor_KeyValue::calculate()
{
	int i = 0, j;
	STAnalysor1_Line *pLine;
	STAnalysor1_UnitData *pUnitData;
	unsigned int nAccumulate = 0;

	float fValue1, fValue2;

	if (m_pLineList) {
		pLine = (STAnalysor1_Line *)m_pLineList->getObj(i);
		while (pLine)
		{
			if (pLine->nTotalLine)
			{
				fValue2 = (float)pLine->nTotalLine;
				j = 0;
				if (pLine->m_pUnitDataList) {
					pUnitData = (STAnalysor1_UnitData *)pLine->m_pUnitDataList->getObj(j);
					while (pUnitData) {
						if (pUnitData->nTotCount) {
							fValue1 = (float)pUnitData->nTotCount;
							pUnitData->fPercent = (fValue1 / fValue2) * 100.0f;
							nAccumulate += pUnitData->nValue * pUnitData->nTotCount;
						}
						j++;
						pUnitData = (STAnalysor1_UnitData *)pLine->m_pUnitDataList->getObj(j);
					}
				}
				if (nAccumulate) {
					fValue1 = (float)nAccumulate;
					pLine->fAvg = nAccumulate / fValue2;
					nAccumulate = 0;
				}
			}
			i++;
			pLine = (STAnalysor1_Line *)m_pLineList->getObj(i);
		}
	}
}

void CAnalysor_KeyValue::report(CLogger *pLogger)
{
	char *szBuf = gs_pMMgr->newBufByIndex(IDX_BUF_16k);

	int i = 0, j, nPos=0, nDiff, fAvg;
	STAnalysor1_Line *pLine;
	STAnalysor1_UnitData *pUnitData;
	unsigned int nAccumulate = 0;
	float fRatio;

	if (m_pLineList) {
		calculate();
		nPos += sprintf(szBuf + nPos, "\n\n################################################# \n");
		nPos += sprintf(szBuf + nPos,	  "  KeyValue Report : \n");
		nPos += sprintf(szBuf + nPos,	  "################################################# \n");
		pLogger->LogPrint(szBuf); nPos = 0;

		pLine = (STAnalysor1_Line *)m_pLineList->getObj(i);
		while (pLine)
		{
			nPos += sprintf(szBuf + nPos, "Key[%s]	nTotCount[%d] Average[%f] Max[%d] min[%d] : \n", pLine->pKeyword, pLine->nTotalLine, pLine->fAvg, pLine->nMax, pLine->nMin);	
			if (pLine->nTotCnt_expected) {
				nDiff = pLine->nTotalLine - pLine->nTotCnt_expected;
				fRatio = ((float)pLine->nTotalLine / (float)pLine->nTotCnt_expected) * 100.0f;
				fAvg = 
				nPos += sprintf(szBuf + nPos, "	correction: nExpectedCount[%d] Difference[%d] ratio[%f] Average[%f]\n", pLine->nTotCnt_expected, nDiff, fRatio);
			}
			j = 0;	
			if (pLine->m_pUnitDataList) {
				pUnitData = (STAnalysor1_UnitData *)pLine->m_pUnitDataList->getObj(j);
				while (pUnitData) {
					nPos += sprintf(szBuf + nPos, "		KeyValue[%s] report : nValue[%d] nTotCount[%d] percent[%f]\n", pLine->pValueKey, pUnitData->nValue, pUnitData->nTotCount, pUnitData->fPercent);
					j++;
					pUnitData = (STAnalysor1_UnitData *)pLine->m_pUnitDataList->getObj(j);
				}
			}

			nPos += sprintf(szBuf + nPos, "\n");

			i++;
			pLine = (STAnalysor1_Line *)m_pLineList->getObj(i);
		}
	}

	pLogger->LogPrint(szBuf); nPos = 0;
	gs_pMMgr->delBufByIndex(szBuf, IDX_BUF_16k);

}
