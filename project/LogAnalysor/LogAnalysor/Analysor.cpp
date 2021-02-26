#include "Analysor.h"


CAnalysor::CAnalysor()
{

}

CAnalysor::~CAnalysor()
{

}

bool CAnalysor::initConfig(char *pConfigFile, char *pSector)
{
	int nRes = GetPrivateProfileInt(pSector, _T("IS_AVAILABLE"), 0, pConfigFile);
	m_bAvailable = nRes ? true : false;
	return m_bAvailable;
}

bool CAnalysor::parsingLine(char *pLine)
{
	return true;
}

void CAnalysor::report(CLogger *pLogger)
{

}