
#include "Analysor_DateTime.h"

CAnalysor_DateTime *g_pAnalysor_DateTime;


CAnalysor_DateTime::CAnalysor_DateTime()
{
	m_nCurSlot = 0;
	m_nListIdx = 0;
	m_pTimeSlot = NULL;

	m_nCurSlot2 = 0;
	m_nListIdx2 = 0;
	m_pTimeSlot2 = NULL;

	m_isDetail = false;

	m_pDTE2 = NULL;
	m_pTitle = NULL;
	m_pSecondKeyword = NULL;
}

CAnalysor_DateTime::~CAnalysor_DateTime()
{
	int i, j;
	STMTime **pSlot;
	for (i = 0; i < 2; i++)
	{
		if (i == 0) pSlot = m_pTimeSlot;
		else pSlot = m_pTimeSlot2;

		if (pSlot) {
			for (j = 0; j < UNIT_TIME_SLOT; j++)
			{
				if (pSlot[j]) free(pSlot[j]);
			}
			free(pSlot);
		}
	}

	m_pTimeSlot = NULL;
	m_pTimeSlot2 = NULL;

	if (m_pDTE2) {
		free(m_pDTE2);
		m_pDTE2 = NULL;
	}

	if (m_pTitle) {
		gs_pMMgr->delString(m_pTitle); m_pTitle = NULL;
	}
	if (m_pSecondKeyword) {
		gs_pMMgr->delString(m_pSecondKeyword); m_pSecondKeyword = NULL;
	}
}


bool CAnalysor_DateTime::initConfig(char *pConfigFile, char *pSector)
{
	char szBuf[64];
	int nRes;
	int nSecondUse = GetPrivateProfileInt(pSector, _T("IS_USE_SECOND_TIMESTAMP"), 0, pConfigFile);
	

	if (!initTimeSlot(&m_pTimeSlot)) {
		comErrorPrint("initTimeSlot(&m_pTimeSlot1) has failed");
		return false;
	}

	m_nSendUnitCnt = GetPrivateProfileInt(pSector, _T("SEND_UNIT_COUNT"), 0, pConfigFile);
	nRes = GetPrivateProfileInt(pSector, _T("ISDETAIL_REPORT"), 0, pConfigFile);
	m_isDetail = nRes ? true : false;

	nSecondUse = GetPrivateProfileInt(pSector, _T("IS_USE_SECOND_TIMESTAMP"), 0, pConfigFile);
	if (nSecondUse) {
		nRes = GetPrivateProfileString(pSector, _T("SECOND_DATETIME_EXPRESSION"), _T(""), szBuf, sizeof(szBuf), pConfigFile);
		if (nRes) {
			m_pDTE2 = (STDateTimeExp *)calloc(1, sizeof(STDateTimeExp));
			if (parsingDateTimeExp(szBuf, m_pDTE2)) {
				if (!initTimeSlot(&m_pTimeSlot2)) {
					comErrorPrint("initTimeSlot(&m_pTimeSlot2) has failed");
					return false;
				}
				nRes = GetPrivateProfileString(pSector, _T("SECOND_TIMESTAMP_KEYWORD"), _T(""), szBuf, sizeof(szBuf), pConfigFile);
				if (nRes) { m_pSecondKeyword = gs_pMMgr->newString(szBuf); }
				else {
					comErrorPrint("SECOND_TIMESTAMP_KEYWORD is NULL");
					return false;
				}
			}
		}
	}

	nRes = GetPrivateProfileString(pSector, _T("TIMESTAMP_TITLE"), _T(""), szBuf, sizeof(szBuf), pConfigFile);
	if (nRes) { m_pTitle = gs_pMMgr->newString(szBuf); }
	else {
		m_pTitle = gs_pMMgr->newString("TimeStamp");
	}
	CAnalysor::setAvailable(true);
	return true;
}

bool CAnalysor_DateTime::initTimeSlot(STMTime ***pSlot)
{
	*pSlot = (STMTime **)calloc(UNIT_TIME_SLOT, sizeof(STMTime *));
	if (*pSlot) {
		return true;
	}
	return false;

}

bool CAnalysor_DateTime::parsingLine(char *pLine)
{
	STDTime stTime;

	parsingTimeLog(pLine, &stTime, g_stConfig.pDTE);
	addDataToTimeSlot(&stTime);

	if (m_pDTE2)
	{
		//EX : collectionTime="2019-08-09T10:02:19.661+09:00"
		char *pNext = ehstrstr(pLine, "collectionTime=") + 1;
		if (pNext) {
			parsingTimeLog(pNext, &stTime, m_pDTE2);
			addDataToTimeSlot2(&stTime);
		}
	}


	return true;
}


void CAnalysor_DateTime::addDataToTimeSlot(STDTime *pTime)
{
	// 1. 현재 Idx 를 기준으로 만 서칭
	// TODO :  시간이 안맞으면, 앞, 뒤 서칭 추가

	int nHourIdx = pTime->nHour;
	int nMinuteIdx = pTime->nMinute;

	if (!m_pTimeSlot[nHourIdx]) {
		m_pTimeSlot[nHourIdx] = (STMTime *)calloc(UNIT_M_LIST, sizeof(STMTime));
		if (!m_pTimeSlot[nHourIdx]) {
			// TODO Error Log
			return;
		}
	}

	STMTime *pMTime = &m_pTimeSlot[nHourIdx][nMinuteIdx];
	pMTime->nCnt++;

	nHourIdx = pTime->nSec;
	nMinuteIdx = pTime->nLast / 100;
	pMTime->milliCnt_100[nHourIdx][nMinuteIdx]++;
}






void CAnalysor_DateTime::addDataToTimeSlot2(STDTime *pTime)
{
	// 1. 현재 Idx 를 기준으로 만 서칭
	// TODO :  시간이 안맞으면, 앞, 뒤 서칭 추가

	int nHourIdx = pTime->nHour;
	int nMinuteIdx = pTime->nMinute;

	if (!m_pTimeSlot2[nHourIdx]) {
		m_pTimeSlot2[nHourIdx] = (STMTime *)calloc(UNIT_M_LIST, sizeof(STMTime));
		if (!m_pTimeSlot2[nHourIdx]) {
			// TODO Error Log
			return;
		}
	}

	STMTime *pMTime = &m_pTimeSlot2[nHourIdx][nMinuteIdx];
	pMTime->nCnt++;
	nHourIdx = pTime->nSec;
	nMinuteIdx = pTime->nLast / 100;
	pMTime->milliCnt_100[nHourIdx][nMinuteIdx]++;
}


void CAnalysor_DateTime::calculate(STMTime *pMTime)
{
	int i, j, nRes, nMin=0, nMax=0, CumulativeValue=0, isFirst=1;
	for (i = 0; i < 60; i++) {
		for (j = 0; j < 10; j++) {
			if (pMTime->milliCnt_100[i][j]) {
				nRes = pMTime->milliCnt_100[i][j] - m_nSendUnitCnt;
				if (nRes) {
					if (nRes < 0) nRes = -(nRes);
					pMTime->nTotal++;
					if (nMin > nRes) {
						nMin = nRes;
					}
					else if (nMax < nRes) {
						nMax = nRes;
					}
					
					if (isFirst) {
						nMin = nRes;
						nMax = nRes;
						isFirst = 0;
					}
					CumulativeValue += nRes;
				}
			}
		}
	}

	if (pMTime->nTotal) {
		pMTime->nAvg = CumulativeValue / pMTime->nTotal;
		pMTime->nMin = nMin;
		pMTime->nMax = nMax;
	}

}

void CAnalysor_DateTime::reportDetail(CLogger *pLogger, STMTime *pMTime)
{
	int nSize = 600 << 5;
	char *pBuf = gs_pMMgr->newBufByIndex(IDX_BUF_16k);
	int nPos = 0, i = 0, j = 0;
	for (i = 0; i < 60; i++) {
		nPos += sprintf(pBuf + nPos, "   time[s:%02d] {", i);
		for (j = 0; j < 10; j++) {
			nPos += sprintf(pBuf + nPos, "%d,", pMTime->milliCnt_100[i][j]);
		}
		nPos += sprintf(pBuf + nPos, "}\n");
	}
	pLogger->LogPrint(pBuf);

	gs_pMMgr->delBufByIndex(pBuf, IDX_BUF_16k);
}

void CAnalysor_DateTime::report(CLogger *pLogger)
{
	int nPos = 0;
	int i = 0, j = 0, nIdx;

	STMTime **pMTimeList;
	STMTime *pMTime;

	char szBuf[1024];

	nPos += sprintf(szBuf + nPos, "\n\n################################################# \n");
	nPos += sprintf(szBuf + nPos, "  DateTime Report : \n");
	nPos += sprintf(szBuf + nPos, "################################################# \n");
	pLogger->LogPrint(szBuf); nPos = 0;

	for (nIdx = 0; nIdx < 2; nIdx++)
	{
		if (nIdx) {
			pMTimeList = m_pTimeSlot2;
			sprintf(szBuf, "\n #### %s report : ####\n", m_pSecondKeyword);
			pLogger->LogPrint(szBuf);
		}
		else {
			pMTimeList = m_pTimeSlot;
			sprintf(szBuf, "\n #### %s report : ####\n", m_pTitle);
			pLogger->LogPrint(szBuf);
		}

		for (i = 0; i < UNIT_TIME_SLOT; i++) {
			if (!pMTimeList[i]) continue;
			for (j = 0; j < UNIT_M_LIST; j++) {
				pMTime = &pMTimeList[i][j];
				if (pMTime->nCnt) {
					calculate(pMTime);
					sprintf(szBuf, "time[h:%02d:m:%02d] nCount[%u] - (Other reference values in 100 miliseconds TOTAL[%d] AVG[%d] MIN[%d] MAX[%d])\n",
						i, j, pMTime->nCnt, pMTime->nTotal, pMTime->nAvg, pMTime->nMin, pMTime->nMax);
					pLogger->LogPrint(szBuf);

					if (m_isDetail) {
						if (pMTime->nTotal) {
							reportDetail(pLogger, pMTime);
						}
					}
				}
			} // end j for loop
		} // end i for loop
	}// end nIdx for loop

}