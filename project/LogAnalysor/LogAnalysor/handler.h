#pragma once

#include "Analysor.h"
#include "Analysor_Average.h"
#include "Analysor_BetweenLines.h"
#include "Analysor_DateTime.h"
#include "Analysor_ExtractNToFile.h"
#include "Analysor_KeyValue.h"


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
	CAnalysor *m_pAnalysorList[eAType_Max];
};

extern CHandler *g_pHandle;
