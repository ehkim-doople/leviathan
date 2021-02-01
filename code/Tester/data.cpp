#include "data.h"
#include "comCore.h"

CMetaData::CMetaData()
{
	m_pParamList = NULL;
	m_pEventList = NULL;
	m_pEventPool = NULL;
	m_pParamPool = NULL;
	memset(m_szParamPath, 0, sizeof(m_szParamPath));
	memset(m_szEventPath, 0, sizeof(m_szEventPath));
	gs_pCore = new (std::nothrow) CCoreList();
	if (!gs_pCore) {
		comErrorPrint("gs_pCore alloc has Failed!!!");
		exit(0);
	}
}

CMetaData::~CMetaData()
{
	if (m_pEventPool) { delete m_pEventPool; m_pEventPool = 0; }
	if (m_pParamPool) { delete m_pParamPool; m_pParamPool = 0; }
	if (m_pParamList) {	delete m_pParamList; m_pParamList = 0; }
	if (m_pEventList) { delete m_pEventList; m_pEventList = 0; }
}

bool CMetaData::init()
{
	if (m_pParamList) return true;


	if (!gs_pCore->initDefaultPool(256, 1024, 0)) {
		comErrorPrint(_T("g_cCore.initDefaultPool(256, 1024, 0) has Failed!"));
		return false;
	}

	m_pParamList = new (std::nothrow) CTList<CStringList>();
	if (!m_pParamList) {
		printf("init() new m_pParamList error\n"); return false;
		//gs_cLogger.DebugLog(LEVEL_ERROR, _T("new m_pParamList")); 
	}
	if (!m_pParamList->alloc(10000)) {
		delete m_pParamList;
		m_pParamList = NULL;
		printf("init() alloc m_pParamList error\n"); return false;
		//gs_cLogger.DebugLog(LEVEL_ERROR, _T("alloc m_pParamList")); goto END_PROC;
	}

	if (!CFileUtil::fileToFieldRecord(m_szParamPath, m_pParamList, ',')) {
		delete m_pParamList;
		m_pParamList = NULL;
		printf("init() fileToFieldRecord error\n"); return false;
	}

	m_pEventList = new (std::nothrow) CTList<CStringList>();
	if (!m_pEventList) {
		printf("init() new m_pParamList error\n"); return false;
		//gs_cLogger.DebugLog(LEVEL_ERROR, _T("new m_pParamList")); 
	}
	if (!m_pEventList->alloc(512)) {
		delete m_pEventList;
		m_pEventList = NULL;
		printf("init() alloc m_pParamList error\n"); return false;
		//gs_cLogger.DebugLog(LEVEL_ERROR, _T("alloc m_pParamList")); goto END_PROC;
	}

	if (!CFileUtil::fileToFieldRecord(m_szEventPath, m_pEventList, ',')) {
		delete m_pEventList;
		m_pEventList = NULL;
		printf("init() fileToFieldRecord error\n"); return false;
	}

	m_pEventPool = new (std::nothrow) CBPoolSlot<STEventData>();
	if (!m_pEventPool) {
		printf("init() new m_pEventPool error\n"); return false;
	}
	if (!m_pEventPool->alloc(256, eAlloc_Type_alloc)) {
		delete m_pEventPool;
		m_pEventPool = NULL;
		printf("init() alloc m_pEventPool error\n"); return false;
	}
	m_pParamPool = new (std::nothrow) CBPoolSlot<STParameterData>();
	if (!m_pParamPool) {
		printf("init() new m_pParamPool error\n"); return false;
	}
	if (!m_pParamPool->alloc(512, eAlloc_Type_alloc)) {
		delete m_pParamPool;
		m_pParamPool = NULL;
		printf("init() alloc m_pEventPool error\n"); return false;
	}
	dataParsingNSet();
	return true;
}

SSCVT::E_ValueType getType(TCHAR *p) {
	// SSCVT:: -> 7
	switch (p[7]) {
	case 'A': if (p[8] == 'N') return SSCVT::ANYURI;  
		if (p[8] == 'R') return SSCVT::ARRAY; 
		return SSCVT::NONE;
	case 'B': if (p[8] == 'Y') return SSCVT::BYTE;  
		if (p[8] == 'O') return SSCVT::BOOLEAN; 
		if (p[8] == 'A') return SSCVT::BASE64BINARY; 
		return SSCVT::NONE;
	case 'D': if (p[8] == 'O') return SSCVT::DOUBLE;  
		if (p[8] == 'A') return SSCVT::DATETIME; 
		return SSCVT::NONE;
	case 'E': if (p[12] == 'I') return SSCVT::ENUM_INT;  
		if (p[12] == 'S') return SSCVT::ENUM_STRING; 
		return SSCVT::NONE;
	case 'F': return SSCVT::FLOAT;
	case 'I': return SSCVT::INT;
	case 'L': return SSCVT::LONG;
	case 'N': return SSCVT::NOVALUE;
	case 'S': if (p[8] == 'H') return SSCVT::SHORT;  
		if (p[10] == 'I') return SSCVT::STRING; 
		if (p[10] == 'U') return SSCVT::STRUCTURE; 
		return SSCVT::NONE;
	case 'V': if (p[8] == 'A') return SSCVT::VARIABLE; 
		return SSCVT::NONE;
	}
	return SSCVT::NONE;
}

void CMetaData::dataParsingNSet() 
{
	CStringList *pList;
	int i = 0;
	STParameterData *pParam;
	STEventData *pEvent;

	// parameter parsing
	pList = m_pParamList->getNext(&i);
	while (pList) {
		pParam = m_pParamPool->newMem();
		pParam->pSourceId = pList->get_str(eP_column_SID);
		pParam->pParameterName = pList->get_str(eP_column_PName);
		pParam->nType = getType(pList->get_str(eP_column_Type2));
		//gs_cLogger.PutLogQueue(LEVEL_INFO, _T("%s, %s, %d"), pParam->pSourceId, pParam->pParameterName, pParam->nType);
		//_tprintf(_T("[%d] %s, %s, %d\n"), i, pParam->pSourceId, pParam->pParameterName, pParam->nType);
		i++;
		pList = m_pParamList->getNext(&i);
	}
	// event parsing
	i = 0;
	pList = m_pEventList->getNext(&i);
	while (pList) {
		pEvent = m_pEventPool->newMem();
		pEvent->pNodeId = pList->get_str(eE_column_NID);
		pEvent->pEventId = pList->get_str(eE_column_EID);
		//gs_cLogger.PutLogQueue(LEVEL_INFO, _T("%s, %s, %d"), pEvent->pNodeId, pEvent->pEventId, pEvent->nParamRefCnt);
		//_tprintf(_T("[%d] %s, %s, %d\n"), i, pEvent->pNodeId, pEvent->pEventId, pEvent->nParamRefCnt);
		i++;
		pList = m_pEventList->getNext(&i);
	}

}

