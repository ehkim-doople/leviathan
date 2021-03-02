#pragma once

#include "Analyzer.h"

struct STAnalyzer_Avg
{
	int nTotCount;
	char *pKeyword;
	STInterval stInterval;
};

class CAnalyzer_Average : public CAnalyzer
{
public:
	CAnalyzer_Average();
	~CAnalyzer_Average();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

private:
	CSList *m_pKeyAvgList;  // specific Key's value average

};