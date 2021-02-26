
#include "global.h"

int g_nTimeSize = sizeof(STDTime);
bool parsingDateTimeExp(char *pExp, STDateTimeExp *pDTE)
{
	memset(pDTE, 0, sizeof(STDateTimeExp));
	bool bSuccess = false;
	char *pPos = pExp;
	int nPos = 0;
	while (*pPos) {
		switch (*pPos)
		{
		case 'y': if (!pDTE->nYearLen) pDTE->nDateStart = nPos;  pDTE->nYearLen++;  break;
		case 'm':
			if (nPos < 8) pDTE->nMonthLen++; else pDTE->nMinuteLen++;
			break;
		case 'd': pDTE->nDateLen++;	break;
		case 'h': if (!pDTE->nHourLen) pDTE->nTimeStart = nPos; pDTE->nHourLen++;	break;
		case 's': pDTE->nSecLen++;	bSuccess = true;  break;
		default:
		{
			if (IS_DIGIT(*pPos)) {
				pDTE->nLastLen = atoi(pPos);
				return true;
			}
			else {
				if (nPos < 8) pDTE->cDateDelimeter = *pPos;
				else pDTE->cTimeDelimeter = *pPos;
			}
			break;
		}
		}
		pPos++;	nPos++;
	}
	return bSuccess;
}


void parsingTimeLog(char *pLog, STDTime *pTime, STDateTimeExp *pDTE)
{
	char szBuf[8];
	char *pPos = pLog + pDTE->nDateStart;
	int nDateDelimeter = 0;
	int nTimeDelimeter = 0;
	if (pDTE->cDateDelimeter) nDateDelimeter = 1;
	if (pDTE->cTimeDelimeter) nTimeDelimeter = 1;
	strncpy(szBuf, pPos, pDTE->nYearLen); szBuf[pDTE->nYearLen] = 0; pTime->nYear = atoi(szBuf);
	pPos += pDTE->nYearLen + nDateDelimeter;
	strncpy(szBuf, pPos, pDTE->nMonthLen); szBuf[pDTE->nMonthLen] = 0; pTime->nMonth = atoi(szBuf);
	pPos += pDTE->nMonthLen + nDateDelimeter;
	strncpy(szBuf, pPos, pDTE->nDateLen); szBuf[pDTE->nDateLen] = 0; pTime->nDay = atoi(szBuf);
	
	pPos = pLog + pDTE->nTimeStart;
	strncpy(szBuf, pPos, pDTE->nHourLen); szBuf[pDTE->nHourLen] = 0; pTime->nHour = atoi(szBuf);
	pPos += pDTE->nHourLen + nTimeDelimeter;
	strncpy(szBuf, pPos, pDTE->nMinuteLen); szBuf[pDTE->nMinuteLen] = 0; pTime->nMinute = atoi(szBuf);
	pPos += pDTE->nMinuteLen + nTimeDelimeter;
	strncpy(szBuf, pPos, pDTE->nSecLen); szBuf[pDTE->nSecLen] = 0; pTime->nSec = atoi(szBuf);
	pPos += pDTE->nSecLen + nTimeDelimeter;
	strncpy(szBuf, pPos, pDTE->nLastLen); szBuf[pDTE->nLastLen] = 0; pTime->nLast = atoi(szBuf);
}

void setBeginningTime(STDTime *pTarget, STDTime *pNewTime)
{
	// TODO
	// To be changed to shift operation

	if (!pTarget->nHour) {
		memcpy(pTarget, pNewTime, g_nTimeSize);
		return;
	}

	if (pTarget->nDay == pNewTime->nDay) {

		if (pTarget->nHour == pNewTime->nHour) {
			if (pTarget->nMinute == pNewTime->nMinute) {
				if (pTarget->nSec == pNewTime->nSec) {
					if (pTarget->nLast > pNewTime->nLast) {
						memcpy(pTarget, pNewTime, g_nTimeSize);
					}
				}
				else if (pTarget->nSec > pNewTime->nSec) {
					memcpy(pTarget, pNewTime, g_nTimeSize);
				}
			}
			else if (pTarget->nMinute > pNewTime->nMinute) {
				memcpy(pTarget, pNewTime, g_nTimeSize);
			}
		}
		else if (pTarget->nHour > pNewTime->nHour) {
			memcpy(pTarget, pNewTime, g_nTimeSize);
		}
	}
	else if (pTarget->nDay > pNewTime->nDay) {
		memcpy(pTarget, pNewTime, g_nTimeSize);
	}
}

void setEndingTime(STDTime *pTarget, STDTime *pNewTime)
{
	// TODO
	// To be changed to shift operation
	if (!pTarget->nHour) {
		memcpy(pTarget, pNewTime, g_nTimeSize);
		return;
	}

	if (pTarget->nDay == pNewTime->nDay) {
		if (pTarget->nHour == pNewTime->nHour) {
			if (pTarget->nMinute == pNewTime->nMinute) {
				if (pTarget->nSec == pNewTime->nSec) {
					if (pTarget->nLast < pNewTime->nLast) {
						memcpy(pTarget, pNewTime, g_nTimeSize);
					}
				}
				else if (pTarget->nSec < pNewTime->nSec) {
					memcpy(pTarget, pNewTime, g_nTimeSize);
				}
			}
			else if (pTarget->nMinute < pNewTime->nMinute) {
				memcpy(pTarget, pNewTime, g_nTimeSize);
			}
		}
		else if (pTarget->nHour < pNewTime->nHour) {
			memcpy(pTarget, pNewTime, g_nTimeSize);
		}
	}
	else if (pTarget->nDay < pNewTime->nDay) {
		memcpy(pTarget, pNewTime, g_nTimeSize);
	}
}

// pTime2 - pTime1 = pTimeTarget
void getTimeInterval(STDTime *pTime1, STDTime *pTime2, STDTime *pTimeTarget)
{
	int nValue = 0, i;
	int nLast = g_stConfig.pDTE->nLastLen;
	if (pTime2->nLast < pTime1->nLast) {
		nValue = 1;
		for (i = 0; i < nLast; i++)
		{
			nValue = nValue * 10;
		}
		pTimeTarget->nLast = (pTime2->nLast + nValue) - pTime1->nLast;
		pTime2->nSec -= 1;
	}
	else {
		pTimeTarget->nLast = pTime2->nLast - pTime1->nLast;
	}

	if (pTime2->nSec < pTime1->nSec) {
		pTimeTarget->nSec = (pTime2->nSec + 60) - pTime1->nSec;
		pTime2->nMinute -= 1;
	}
	else {
		pTimeTarget->nSec = pTime2->nSec - pTime1->nSec;
	}

	if (pTime2->nMinute < pTime1->nMinute) {
		pTimeTarget->nMinute = (pTime2->nMinute + 60) - pTime1->nMinute;
		pTime2->nHour -= 1;
	}
	else {
		pTimeTarget->nMinute = pTime2->nMinute - pTime1->nMinute;
	}

	if (pTime2->nHour < pTime1->nHour) {
		pTimeTarget->nHour = (pTime2->nHour + 24) - pTime1->nHour;
		pTime2->nDay -= 1;
	}
	else {
		pTimeTarget->nHour = pTime2->nHour - pTime1->nHour;
	}	
	pTimeTarget->nDay = pTime2->nDay - pTime1->nDay;
}

int getInterval(char *pSorceString, char *pKey)
{
	char szBuf[24];
	char *p2, *p = strstr(pSorceString, pKey);
	if (!p) return -1;
	p = strchr(p, '[') + 1;
	p2 = strchr(p, ']');
	if (!p2) return -1;
	int nLen = p2 - p;
	strncpy(szBuf, p, nLen);
	szBuf[nLen] = 0;
	if (IS_NDIGIT(szBuf)) {
		gs_cLogger.DebugLog(LEVEL_ERROR, "szBuf[%s]", szBuf);
		return -1;
	}
	return atoi(szBuf);
}


int getMiliseconds(STDTime *pTime, int nLastLen)
{
	int nValue;
	int nRes = 0;
	if (pTime->nHour) {
		nRes = pTime->nHour * 3600000;// 60 * 60 * 1000;
	}
	if (pTime->nMinute) {
		nRes += pTime->nMinute * 60000;//60 * 1000;
	}
	if (pTime->nSec) {
		nRes += pTime->nSec * 1000;
	}

	if (nLastLen == 3) {
		nValue = pTime->nLast;
	}
	else if (nLastLen == 6) {
		nValue = pTime->nLast / 1000;
	}
	else if (nLastLen == 9) {
		nValue = pTime->nLast / 1000000;
	}
	return nRes + nValue;
}

float getAvg(int nCount, float *pNum)
{
	int i, nSum = 0;
	float fSum = 0;
	for (i = 0; i < nCount; i++)
	{
		fSum += pNum[i];
	}
	return fSum / nCount;
}

// The sum of the unit counts to be performed during a specific time
int getTimeCount(STDTime *pTime, int nUnitCount, int nCountSec)
{
	int nCount = 0;

	nCount += pTime->nSec * nCountSec * nUnitCount;
	nCount += pTime->nMinute * nCountSec * 60 * nUnitCount;
	nCount += pTime->nHour * nCountSec * 3600 * nUnitCount; /*60 * 60*/

	return nCount;
}



int intervalReport(STInterval *p, STBuf *pTarget, float *fAvg)
{
	int i, nIdx = 0;
	int nPos = 0;
	float fSum = 0;
	char *pBuf;

	int nSize = p->nAvgIdx;
	if (p->nAvgIdx < p->nIntervalIdx) nSize = p->nIntervalIdx;
	nSize <<= 5;
	pTarget->pValue = gs_pMMgr->newBuf(nSize, &pTarget->nIdx);
	pBuf = pTarget->pValue;

	CLogger *pLogger = g_stConfig.pReport;

	if (0 < p->nAvgIdx) {
		for (i = 0; i < p->nAvgIdx; i++) {
			nPos += sprintf(pBuf + nPos, " [%d] - %d avg: %f\n", i, AVG_COUNT, p->nAvg[i]);
			fSum += p->nAvg[i];
		}
		*fAvg = fSum / i;
	}
	else if (p->nIntervalIdx) {
		for (i = 0; i < p->nIntervalIdx; i++) {
			nPos += sprintf(pBuf + nPos, " [%d]  : %f\n", i, p->nInterval[i]);
			fSum += p->nInterval[i];
		}
		*fAvg = fSum / i;
	}

	return nPos;
}