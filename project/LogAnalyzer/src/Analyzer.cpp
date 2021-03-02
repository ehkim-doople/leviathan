#include "Analyzer.h"


CAnalyzer::CAnalyzer()
{

}

CAnalyzer::~CAnalyzer()
{

}

bool CAnalyzer::initConfig(char *pConfigFile, char *pSector)
{
	int nRes = GetPrivateProfileInt(pSector, _T("IS_AVAILABLE"), 0, pConfigFile);
	m_bAvailable = nRes ? true : false;
	return m_bAvailable;
}

bool CAnalyzer::parsingLine(char *pLine)
{
	return true;
}

void CAnalyzer::report(CLogger *pLogger)
{

}