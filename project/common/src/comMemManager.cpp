#include "comMemManager.h"
#include "comLogger.h"

CMemManager *gs_pMMgr;
TCHAR g_szSystemError[LEN_SYSTEMERROR_MESSAGE];
TCHAR g_szMessage[LEN_MEM_MESSAGE];

CMemManager::CMemManager()
{
	m_nOverMaxIdx = 0;
	memset(m_pBufList, 0, sizeof(m_pBufList));
	memset(&m_stConfig, 0, sizeof(STBufConf));
	int i;

	m_stConfig.nMaxList[IDX_BUF_2] = 256;
	m_stConfig.nMaxList[IDX_BUF_4] = 256;
	m_stConfig.nMaxList[IDX_BUF_8] = 2048;
	m_stConfig.nMaxList[IDX_BUF_16] = 2048;
	for (i = IDX_BUF_32; i < IDX_BUF_16k; i++)
	{
		m_stConfig.nMaxList[i] = 64;
	}
	for (i = IDX_BUF_16k; i < IDX_BUF_64k; i++)
	{
		m_stConfig.nMaxList[i] = 4;
	}
	for (i = IDX_BUF_64k; i < IDX_BUF_512k; i++)
	{
		m_stConfig.nMaxList[i] = 2;
	}
	m_stConfig.nMaxList[IDX_BUF_1m] = 1;
}

CMemManager::~CMemManager()
{
	int i =  0;
	for (i = IDX_BUF_2; i < IDX_BUF_MAX; i++) {
		if (m_pBufList[i]) {
			delete m_pBufList[i]; m_pBufList[i] = 0;
		}
	}
}

int CMemManager::getStrToInt(TCHAR *pData)
{
	TCHAR *pPos = pData;
	TCHAR szNum[32], cUnit = 0;
	int i = 0, nRes=0;
	while (*pPos) {
		if ('0' <= *pPos && *pPos <= '9') szNum[i++] = *pPos;
		else cUnit = *pPos;
		pPos++;
	}
	nRes = _ttoi(szNum);
	if (cUnit) {
		switch (cUnit) {
		case 'k': case 'K': nRes = nRes * 1024; break;
		case 'm': case 'M': nRes = nRes * 1024 * 1024; break;
		case 'g': case 'G': nRes = nRes * 1024 * 1024 * 1024; break;
		default: printf("getStrToInt parsing FAIL!!! cUnit[%c]\n", cUnit);
		}
	}
	return nRes;
}

void CMemManager::init(STBufConf *pConf)
{
	int i;
	for (i = 0; i < IDX_BUF_MAX; i++)
	{
		m_stConfig.nMaxList[i] += pConf->nMaxList[i];
	}
}

int CMemManager::init(TCHAR *pStrSize, int nCount)
{
	int nSize = getStrToInt(pStrSize);
	int nIdx;
	if (nSize) {
		nIdx = getIndex(nSize);
		m_stConfig.nMaxList[nIdx] += nCount;
	}
	return nSize;
}

bool CMemManager::alloc()
{
	int i;
	for (i = IDX_BUF_2; i < IDX_BUF_MAX; i++)
	{
		if (!m_stConfig.nMaxList[i]) continue;

		if (m_pBufList[i]) {
			delete m_pBufList[i];
			m_pBufList[i] = NULL;
		}
		m_pBufList[i] = new (std::nothrow) CBufPool();
		if (m_pBufList[i]) {
			if (!m_pBufList[i]->alloc(i, m_stConfig.nMaxList[i])) {
				delete m_pBufList[i];
				m_pBufList[i] = NULL;
				return false;
			}
		}
	}
	return true;
}


char * CMemManager::newString(char *pString, int *pIdx)
{
	int nIdx = (int)strlen(pString) + 1;
	nIdx = (int)getIndex(nIdx);
	char *pRes = __newBuf(nIdx);
	strcpy(pRes, pString);
	if (pIdx) *pIdx = nIdx;
	return pRes;
}

void CMemManager::delString(char *pString, int nIdx)
{
	if(nIdx) delBufByIndex(pString, nIdx);
	else {
		int nIdx = (int)strlen(pString) + 1;
		nIdx = getIndex(nIdx);
		delBufByIndex(pString, nIdx);
	}
}

void CMemManager::newString(STBuf *pRes, char *pSource, int nSourceSize)
{
	if (pRes->pValue) {
		delBufByIndex(pRes->pValue, pRes->nIdx);
	}
	if (nSourceSize) {
		pRes->nIdx = (int)getIndex(nSourceSize);
		pRes->pValue = __newBuf(pRes->nIdx);
		strcpy(pRes->pValue, pSource);
	}
	else {
		pRes->pValue = newString(pSource, &pRes->nIdx);
	}
}

void CMemManager::newBuf(int nSize, STBuf *pBuf)
{
	if (pBuf->pValue) {
		delBufByIndex(pBuf->pValue, pBuf->nIdx);
	}
	pBuf->nIdx = (int)getIndex(nSize);
	pBuf->pValue = __newBuf(pBuf->nIdx);
}

char * CMemManager::__newBuf(int nIdx, size_t size)
{
	if (IDX_BUF_2m < nIdx || !m_pBufList[nIdx]) {
		if (m_nOverMaxIdx < nIdx) m_nOverMaxIdx = nIdx;
		if (!size) size = 1 << nIdx;
		char *p = (char *)calloc(1, size);
/*		if (p) {
			gs_cLogger.DebugLog(LEVEL_TRACE, "overIdx:%d, overSize:%lld calloc address[%p]", m_nOverMaxIdx, size, p);
		}
		else {
			gs_cLogger.DebugLog(LEVEL_ERROR, "calloc has Failed");
		}	*/	
		return p;
	}
	return m_pBufList[nIdx]->newMem();
}


void CMemManager::delBufByIndex(char *pBuf, int nIdx)
{
	if (IDX_BUF_2m < nIdx || !m_pBufList[nIdx]) { if (pBuf) free(pBuf); return; }
	if(m_pBufList[nIdx]) m_pBufList[nIdx]->delMem(pBuf);
}


void OBJ_DELETE(E_ALLOC_TYPE type, void *p)
{
	switch (type) {
	case eAlloc_Type_new		: delete p; break;
	case eAlloc_Type_alloc		: free(p); break;
	case eAlloc_Type_BufPool	: gs_pMMgr->delBuf((char *)p, STRING_SIZE((TCHAR *)p)); break;
	case eAlloc_Type_newArray	: delete[] p; break;
	case eAlloc_Type_none		: break;
	case eAlloc_Type_MemPool	: break;
	}
}