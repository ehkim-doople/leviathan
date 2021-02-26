#pragma once


#include "Analysor.h"


// specific value accumulate count & ratio
struct STAnalysor1_UnitData
{
	int nValue;
	int nTotCount;
	float fPercent;
};

// specific Line accumulate count & statistics
struct STAnalysor1_Line
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

class CAnalysor_KeyValue : public CAnalysor
{
public:
	CAnalysor_KeyValue();
	~CAnalysor_KeyValue();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

private:
	CSList *m_pLineList;  // specific line list


	void initLine(char *pLineStartKey, char *pValueKey, int nTotCnt_expected);
	void setLine(char *p, STAnalysor1_Line *pSTLine);
	void setUnitData(CSList *pUnitList, int nValue);
	STAnalysor1_UnitData *getUnitData(CSList *pUnitList, int nValue);
	void calculate();
};