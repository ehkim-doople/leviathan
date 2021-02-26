#pragma once


#include "Analysor.h"

struct STAnalysor_Extract
{
	char *pKeyword;
	int nTotalCount;
	bool bLog;
};

class CAnalysor_ExtractNToFile : public CAnalysor
{
public:
	CAnalysor_ExtractNToFile();
	~CAnalysor_ExtractNToFile();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

	inline void setLogger(CLogger *pLogger) { m_pExtractReport = pLogger; }

private:
	CSList *m_pKeyList;  // specific Key's value average
	CLogger *m_pExtractReport;
};