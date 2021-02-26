
#include "comMString.h"
#include "comLogger.h"

CmString::CmString(void)
{
	m_nSftIdx = 0;
	m_pValue = NULL;
}

CmString::CmString(const TCHAR *str)
{
	int size = STRING_SIZE(str);
	m_nSftIdx = getIndex(size);
	m_pValue = gs_pMMgr->newBufByIndex(m_nSftIdx);
	_tcscpy(m_pValue, str);
}

CmString::CmString(int size)
{
	m_nSftIdx = getIndex(size);
	m_pValue = gs_pMMgr->newBufByIndex(m_nSftIdx);
	m_pValue[0] = 0;
}

CmString::CmString(const CmString &str)
{
	m_nSftIdx = str.capacity_idx();
	m_pValue = gs_pMMgr->newBufByIndex(m_nSftIdx);
	_tcscpy(m_pValue, str.c_str());
}

CmString::CmString(const CmString *str)
{
	m_nSftIdx = str->capacity_idx();
	m_pValue = gs_pMMgr->newBufByIndex(m_nSftIdx);
	_tcscpy(m_pValue, str->c_str());
}

CmString::~CmString(void)
{
	if (m_pValue) {
		gs_pMMgr->delBufByIndex(m_pValue, m_nSftIdx);
		m_pValue = NULL;
		m_nSftIdx = 0;
	}
}



CmString& CmString::operator= (const TCHAR *str)
{
	int size = STRING_SIZE(str);

	if (m_nSftIdx) {
		if (getSizeByIndex(m_nSftIdx) > size) {
			_tcscpy(m_pValue, str);
		}
		else {
			gs_pMMgr->delBufByIndex(m_pValue, m_nSftIdx);
			m_nSftIdx = getIndex(size);
			m_pValue = gs_pMMgr->newBufByIndex(m_nSftIdx);
			_tcscpy(m_pValue, str);
		}
	}
	else {
		m_nSftIdx = getIndex(size);
		m_pValue = gs_pMMgr->newBufByIndex(m_nSftIdx);
		_tcscpy(m_pValue, str);
	}	
	return *this;
}

CmString& CmString::append(const TCHAR *str)
{
	int nLen = (int)_tcslen(str), nOLen = (int)_tcslen(m_pValue);
	int size = nLen * g_nChSize , oSize = nOLen * g_nChSize;

	if (getSizeByIndex(m_nSftIdx) > size + oSize + 1) {
		_tcscpy(m_pValue + nOLen, str);
	}
	else {
		int newIdx = getIndex(size + oSize + 1);
		TCHAR *newBuf = gs_pMMgr->newBufByIndex(newIdx);
		_tcscpy(newBuf, m_pValue);
		_tcscpy(newBuf + nOLen, str);
		gs_pMMgr->delBufByIndex(m_pValue, m_nSftIdx);
		m_pValue = newBuf;
		m_nSftIdx = newIdx;

	}

	return *this;
}



CmString& CmString::replace(int index, int len, const TCHAR *str)
{
	int nLen = (int)_tcslen(str), nOLen = (int)_tcslen(m_pValue);
	int size = nLen * g_nChSize, oSize = nOLen * g_nChSize;

	int newIdx = getIndex(size + oSize - len + 1);
	TCHAR *newBuf = gs_pMMgr->newBufByIndex(newIdx);

	if (index) _tcsncpy(newBuf, m_pValue, index);
	_tcsncpy(newBuf + index, str, len);
	_tcscpy(newBuf + index + len, m_pValue + index + len);

	gs_pMMgr->delBufByIndex(m_pValue, m_nSftIdx);
	m_pValue = newBuf;
	m_nSftIdx = newIdx;
	return *this;
}


void CmString::reserve(int n) 
{
	if (getSizeByIndex(m_nSftIdx) > n) {
		return;
	}
	else {
		gs_pMMgr->delBufByIndex(m_pValue, m_nSftIdx);
		m_nSftIdx = getIndex(n);
		m_pValue = gs_pMMgr->newBufByIndex(m_nSftIdx);
		m_pValue[0] = 0;
	}
}


TCHAR *  CmString::find_next(const TCHAR *str, int index)
{
	TCHAR *pPos = m_pValue+index;
	int nIdx = 0, nLen = (int)_tcslen(str);
	while (*pPos) {
		while (*pPos == str[nIdx++]) {
			pPos++;
			if (nIdx >= nLen) return pPos;
			if (!*pPos) return NULL;
		}
		nIdx = 0;
		pPos++;
	}

	return NULL;
}

TCHAR *  CmString::find(const TCHAR *str, int index)
{
	TCHAR *pPos = m_pValue + index;
	int nIdx = 0, nLen = (int)_tcslen(str);
	while (*pPos) {
		while (*pPos == str[nIdx++]) {
			pPos++;
			if (nIdx >= nLen) return pPos-nLen;
			if (!*pPos) return NULL;
		}
		nIdx = 0;
		pPos++;
	}

	return NULL;
}
bool CmString::operator== (const TCHAR *str)
{
	size_t nLen = _tcslen(str), i, orignLen = _tcslen(m_pValue);
	if (nLen != orignLen) return false;
	for (i = 0; i < nLen; i++) {
		if (m_pValue[i] != str[i]) return false;
	}
	return true;
}
//eOperator_EQ,
//eOperator_NE,
//eOperator_GT,
//eOperator_LT,
//eOperator_GE,
//eOperator_LE,

bool CmString::compare(const TCHAR *str, E_OPERATOR eOP)
{
	switch (eOP) {
	case eOperator_EQ: if (str == m_pValue) return true; break;
	case eOperator_NE: if (str != m_pValue) return true; break;
	case eOperator_SUBSET: if(ehstrstr(m_pValue, str)) return true; break;   // return trure; if included
	case eOperator_NSUBSET: if(!ehstrstr(m_pValue, str)) return true; break;  // return trure; if not included
	// TODO
	case eOperator_GT: break;
	case eOperator_LT: break;
	case eOperator_GE: break;
	case eOperator_LE: break;
	}
	return false;
}

int CmString::compare(const TCHAR *str)
{
	size_t nLen = _tcslen(str),i, orignLen = _tcslen(m_pValue);
	if (nLen > orignLen)  nLen = orignLen;

	for (i = 0; i < nLen; i++) {
		if (m_pValue[i] != str[i]) {
			if (m_pValue[i] > str[i]) return 1;
			else return -1;
		}
	}
	return 0;
}

int CmString::compare(int index, int len, const TCHAR *str)
{
	int i, idx=index, nLen = (int)_tcslen(str);
	if (nLen < len)  len = nLen;

	for (i = 0; i < len; i++) {
		if (m_pValue[idx] != str[i]) {
			if (m_pValue[idx] > str[i]) return 1;
			else return -1;
		}
		idx++;
	}
	return 0;
}

CStringList::CStringList()
{
	m_nListCnt = 0; m_nLast = 0; m_nDelIdx = 0; m_pList = NULL; m_nUseCnt = 0;
}

CStringList::~CStringList()
{
	if (!m_pList) return;
	int i;
	for (i = 0; i < m_nListCnt; i++) {
		if (m_pList[i]) {
			delete m_pList[i];
			m_pList[i] = NULL;
		}
	}
	free(m_pList);
	m_pList = NULL;
	m_nListCnt = 0;
}
int CStringList::__alloc(int nMaxCount)
{
	CmString** pList = (CmString**)calloc(nMaxCount, sizeof(CmString*));
	if (!pList) {
		return 0;
	}

	if (m_pList) {
		memcpy(pList, m_pList, m_nListCnt * sizeof(CmString*));
		free(m_pList);
	}
	m_pList = pList;
	return nMaxCount;
}

#define UNIT_ADD_SIZE 32



CmString * CStringList::__add(TCHAR* pString, int nIdx)
{
	if (0 > nIdx) return NULL;
	CmString *pNewString = new CmString(pString);
	m_pList[nIdx] = pNewString;
	m_nLast = nIdx + 1;
	m_nUseCnt++;
	return pNewString;
}

int CStringList::newIdx()
{
	int nIdx = m_nLast, i;
	for (i = 0; i < m_nListCnt; i++)
	{
		if (nIdx >= m_nListCnt) nIdx = 0;
		if (!m_pList[nIdx]) {
			return nIdx;
		}
		nIdx++;
	}
	// realloc
	if (!__alloc(m_nListCnt + UNIT_ADD_SIZE)) {
		return -1;
	}
	nIdx = m_nListCnt;
	m_nListCnt += UNIT_ADD_SIZE;
	return nIdx;
}


bool CStringList::del(CmString*p)
{
	int i;
	for (i = 0; i < m_nListCnt; i++) {
		if (m_pList[i] == p) {
			_DEL(i);
			return true;
		}
	}
	//gs_cLogger.DebugLog(LEVEL_ERROR, " del FAIL! i[%d] m_nListCnt[%d]", i, m_nListCnt);
	return false;
}

bool CStringList::del(int i)
{
	if (!ISINCLUDE(i, m_nListCnt)) {
		//gs_cLogger.DebugLog(LEVEL_ERROR, "i[%d] m_nListCnt[%d]", i, m_nListCnt);
		return false;
	}
	_DEL(i);
	return true;
}


// return : substr last char position
char * ehstrstr(const char *pTarget, const char *psubstr)
{
	char *pPos = (char *)pTarget;
	int nIdx = 0, nLen = (int)strlen(psubstr);
	while (*pPos) {
		while (*pPos == psubstr[nIdx++]) {
			if (nIdx >= nLen) return pPos;
			pPos++; if (!*pPos) return NULL;
		}
		nIdx = 0;
		pPos++;
	}
	return NULL;
}
