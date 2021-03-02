#pragma once


#include "Analyzer.h"

struct STAnalyzer_Extract
{
	char *pKeyword;
	int nTotalCount;
	bool bLog;
};

class CAnalyzer_ExtractNToFile : public CAnalyzer
{
public:
	CAnalyzer_ExtractNToFile();
	~CAnalyzer_ExtractNToFile();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

	inline void setLogger(CLogger *pLogger) { m_pExtractReport = pLogger; }

private:
	CSList *m_pKeyList;  // specific Key's value average
	CLogger *m_pExtractReport;
};