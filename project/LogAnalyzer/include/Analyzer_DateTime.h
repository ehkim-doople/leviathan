#pragma once

#include "Analyzer.h"

#define UNIT_M_LIST 60		// 60m
#define UNIT_TIME_SLOT 24	// 24h


struct STMTime
{
	unsigned int nCnt;

	// for - other reference values
	int nTotal; 
	int nMin;
	int nMax;
	int nAvg;
	int milliCnt_100[60][10];
};

class CAnalyzer_DateTime : public CAnalyzer
{
public:
	CAnalyzer_DateTime();
	~CAnalyzer_DateTime();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

private:
	bool m_isDetail;
	int m_nSendUnitCnt;

	STMTime **m_pTimeSlot;
	int m_nCurSlot;
	int m_nListIdx;
	char *m_pTitle;

	STMTime **m_pTimeSlot2;
	int m_nCurSlot2;
	int m_nListIdx2;
	STDateTimeExp *m_pDTE2;
	char *m_pSecondKeyword;

	bool initTimeSlot(STMTime ***pSlot);
	void addDataToTimeSlot(STDTime *pTime);
	void addDataToTimeSlot2(STDTime *pTime);
	void calculate(STMTime *pMTime);
	void reportDetail(CLogger *pLogger, STMTime *pMTime);
};

