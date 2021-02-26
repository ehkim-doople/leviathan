#pragma once

#include "Analysor.h"

/*
keyword 1 Line
Keyword 2 Line 
between (keyword1 , keyword2 ) Lines tiemstemp Interval
*/
enum E_BETWEEN_POS
{
	BPOS_LINE_1 = 0,
	BPOS_LINE_2,
	BPOS_LINE_MAX
};

struct STBetween_Class
{
	char *pKeyword[BPOS_LINE_MAX];
	STDTime stPreTime;
	STInterval stInterval;
};



class CAnalysor_BetweenLines : public CAnalysor
{
public:
	CAnalysor_BetweenLines();
	~CAnalysor_BetweenLines();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

private:
	CSList *m_pClassList;  // specific Key's value average

	void parsingClass(char *p, STBetween_Class *pClass);

};