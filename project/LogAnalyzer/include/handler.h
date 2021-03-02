#pragma once

#include "Analyzer.h"
#include "Analyzer_Average.h"
#include "Analyzer_BetweenLines.h"
#include "Analyzer_DateTime.h"
#include "Analyzer_ExtractNToFile.h"
#include "Analyzer_KeyValue.h"


// TODO List
// Enhanced configuration verification function!

class CHandler
{
public:
	CHandler();
	~CHandler();

	bool init();
	bool initAnalistConfig();
	void Report(CLogger *pLogger);
	bool parsingLine(char *pLine);

private:
	CAnalyzer *m_pAnalyzerList[eAType_Max];
};

extern CHandler *g_pHandle;
