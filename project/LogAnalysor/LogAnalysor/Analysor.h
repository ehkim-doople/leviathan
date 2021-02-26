#pragma once

#include "global.h"
#include "comProfileutil.h"

/*
TODO : sequence number check

*/

enum E_ANLYSOR_TYPE
{
	eAType_KeyValue = 0,
	eAType_ExtractNToFile,
	eAType_DateTime,
	eAType_BetweenLines,
	eAType_Average,
	eAType_Max
};

class CAnalysor
{
public:
	CAnalysor();
	virtual ~CAnalysor();

	virtual bool initConfig(char *pConfigFile, char *pSector);
	virtual bool parsingLine(char *p);
	virtual void report(CLogger *pLogger);

	inline bool isAvailable() {	return m_bAvailable; }
	inline void setAvailable(bool bRes) { m_bAvailable = bRes; }
private :
	bool m_bAvailable;
};

//extern CAnalysor *g_pAnalysorList[eAType_Max];