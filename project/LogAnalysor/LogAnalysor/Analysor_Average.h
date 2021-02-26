#pragma once

#include "Analysor.h"

struct STAnalysor_Avg
{
	int nTotCount;
	char *pKeyword;
	STInterval stInterval;
};

class CAnalysor_Average : public CAnalysor
{
public:
	CAnalysor_Average();
	~CAnalysor_Average();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

private:
	CSList *m_pKeyAvgList;  // specific Key's value average

};