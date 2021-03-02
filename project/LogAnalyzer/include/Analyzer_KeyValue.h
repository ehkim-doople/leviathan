#pragma once


#include "Analyzer.h"


// specific value accumulate count & ratio
struct STAnalyzer1_UnitData
{
	int nValue;
	int nTotCount;
	float fPercent;
};

// specific Line accumulate count & statistics
struct STAnalyzer1_Line
{
	char *pKeyword;
	int nTotalLine;
	int nTotCnt_expected;
	int nMax;
	int nMin;
	float fAvg;
	char *pValueKey;
	CSList *m_pUnitDataList;
};

class CAnalyzer_KeyValue : public CAnalyzer
{
public:
	CAnalyzer_KeyValue();
	~CAnalyzer_KeyValue();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

private:
	CSList *m_pLineList;  // specific line list


	void initLine(char *pLineStartKey, char *pValueKey, int nTotCnt_expected);
	void setLine(char *p, STAnalyzer1_Line *pSTLine);
	void setUnitData(CSList *pUnitList, int nValue);
	STAnalyzer1_UnitData *getUnitData(CSList *pUnitList, int nValue);
	void calculate();
};