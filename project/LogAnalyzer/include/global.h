#pragma once

#include "comList.h"
#include "comLogger.h"
#include "comFileUtil.h"

enum ESourceFlag
{
	eSourceFlag_NONE=0,
	eSourceFlag_FILE=1,
	eSourceFlag_DIRECTORY=2
};

#define MAX_KEYWORD 16
#define MAX_TIME_LEN 32
#define MAX_PRE_UNIT 3
#define AVG_COUNT 600
#define MAX_AVG_COUNT 4096


struct STDateTimeExp
{
	int nDateStart;
	int nTimeStart;
	char cDateDelimeter;
	char cTimeDelimeter;
	int nYearLen;
	int nMonthLen;
	int nDateLen;
	int nHourLen;
	int nMinuteLen;
	int nSecLen;
	int nLastLen;
};

struct STFailReport
{
	int nSeq;
	int nCount;
	int nInterval;
};

struct STInterval
{
	int nIntervalIdx;
	int nAvgIdx;
	float nInterval[AVG_COUNT];
	float nAvg[MAX_AVG_COUNT];
};


struct STConfig
{
	int nSourceFlag;
	char *pSourceDirectory;
	char *pSourceFile;
	STDTime stStartTime;
	STDTime stEndTime;
	STDTime stTotTime;
	STDTime stCurTime;
	CLogger *pReport;
	CLogger *pExtractLog;
	char *pLineStartKey;
	char *pLineEndKey;
	int nSkippingLen;
	STDateTimeExp *pDTE;
};

extern STConfig g_stConfig;
extern int g_nTimeSize;

extern STDTime g_stProcStartTime;
extern STDTime g_stProcEndTime;

bool parsingDateTimeExp(char *pExp, STDateTimeExp *pDTE);
void parsingTimeLog(char *pLog, STDTime *pTime, STDateTimeExp *pDTE);
void setBeginningTime(STDTime *pTarget, STDTime *pNewTime);
void setEndingTime(STDTime *pTarget, STDTime *pNewTime);
void getTimeInterval(STDTime *pTime1, STDTime *pTime2, STDTime *pTimeTarget);
int getInterval(char *pSorceString, char *pKey);
float getAvg(int nCount, float *pNum);
int getMiliseconds(STDTime *pTime, int nLastLen);
int getTimeCount(STDTime *pTime, int nUnitCount, int nCountSec);
int intervalReport(STInterval *p, STBuf* pTarget, float *fAvg);
