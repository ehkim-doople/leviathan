#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "comHash.h"




CHash::CHash(HASH_TYPE max)
{
	// TODO m_nMaxCount 를 2의 제곱으로 조정한다.
	m_nMaxCount = max;
	m_nElementCount = 0;
	m_pHashData = (SThash_value *)calloc(m_nMaxCount, sizeof(SThash_value));
}

CHash::~CHash()
{
	if(m_pHashData) {
		free(m_pHashData);
		m_pHashData = NULL;
	}
}

void CHash::clear()
{
	memset(m_pHashData, 0, sizeof(m_pHashData));
}

HASH_TYPE CHash::getHashNo(TCHAR *pStr)
{
	HASH_TYPE nLen = (HASH_TYPE)_tcslen(pStr);
	HASH_TYPE i;
	HASH_TYPE key = nLen;
	for(i=0; i < nLen; i++) {
		key += (key << 4) + (int)pStr[i];
	}
	key = key % m_nMaxCount;
	return key;
}

STHash_Node * CHash::hashLookup(TCHAR *pKey)
{
	SThash_next *pNext;
	HASH_TYPE nHashNo = getHashNo(pKey);
	if(nHashNo >= m_nMaxCount) return NULL;

	for (pNext = m_pHashData[nHashNo].pNext; pNext != NULL; pNext = pNext->next)
	{
		if (_tcscmp(pKey, pNext->pNode.pKey) == 0)
			return &pNext->pNode;
	}

	return NULL;

}

STHash_Node * CHash::hashLookup(HASH_TYPE nHashNo, TCHAR *pKey)
{
	SThash_next *pNext;
	if(nHashNo >= m_nMaxCount) return NULL;
	for (pNext = m_pHashData[nHashNo].pNext; pNext != NULL; pNext = pNext->next)
	{
		if (_tcscmp(pKey, pNext->pNode.pKey) == 0)
			return &pNext->pNode;
	}

	return NULL;

}
SThash_next * CHash::getNext(HASH_TYPE nHashKey)
{
	if (nHashKey >= m_nMaxCount) return NULL;

	return m_pHashData[nHashKey].pNext;
}


SThash_next * CHash::getNext(TCHAR *pStr)
{
	HASH_TYPE nHashNo = getHashNo(pStr);
	return m_pHashData[nHashNo].pNext;
}

SThash_next * CHash::getNode(TCHAR *pStr, SThash_next **prevNext) // for delete
{
	SThash_next *pNext;
	HASH_TYPE nHashNo = getHashNo(pStr);

	prevNext = &m_pHashData[nHashNo].pNext;
	for (pNext = m_pHashData[nHashNo].pNext; pNext != NULL; pNext = pNext->next)
	{
		if (_tcscmp(pStr, pNext->pNode.pKey) == 0) return pNext;
		prevNext = &(pNext->next);
	}

	return NULL;
}

HASH_RETURN_CODE CHash::setValue(TCHAR *pKey, void *pData)
{
	STHash_Node *pNode = hashLookup(pKey);
	if (pNode) {
		pNode->value = pData;
		return eHASH_RESULT_SUCESS;
	}
	return eHASH_RESULT_NOT_FOUND;
}

HASH_RETURN_CODE CHash::setValue(HASH_TYPE nHashNo, TCHAR *pKey, void *pData)
{
	SThash_next *pNext;

	for (pNext = m_pHashData[nHashNo].pNext; pNext != NULL; pNext = pNext->next)
	{
		if (_tcscmp(pKey, pNext->pNode.pKey) == 0)
		{
			pNext->pNode.value = pData;
			return eHASH_RESULT_SUCESS;
		}
	}
	return eHASH_RESULT_NOT_FOUND;
}



// 할당 완료된 pNext, 할당 완료된 pKey, 할당 완료된 pData
int CHash::addNode(SThash_next *pNext, TCHAR *pKey, void *pData)
{
	SThash_next **pNextAddr;

	HASH_TYPE nHashNo = getHashNo(pKey);
	if (nHashNo >= m_nMaxCount) return eHASH_RESULT_INVALID_KEY;
	if (!pNext) return eHASH_RESULT_EMPTY_NODE;

	
	for (pNextAddr = &(m_pHashData[nHashNo].pNext); *pNextAddr != NULL; pNextAddr = &(*pNextAddr)->next) {}
	pNext->pNode.pKey = pKey;
	pNext->pNode.value = pData;
	pNext->next = NULL;
	*pNextAddr = pNext;

	if (!m_pHashData[nHashNo].nHashKey) m_pHashData[nHashNo].nHashKey = nHashNo;
	m_pHashData[nHashNo].nElementCnt++;
	m_nElementCount++;
	return nHashNo;
}

// 할당 완료된 pNext, 할당 완료된 pKey, 할당 완료된 pData
int CHash::uniqueAddNode(SThash_next *pNext, TCHAR *pKey, void *pData)
{
	SThash_next **pNextAddr;

	HASH_TYPE nHashNo = getHashNo(pKey);
	if (nHashNo >= m_nMaxCount) return eHASH_RESULT_INVALID_KEY;
	if (!pNext) return eHASH_RESULT_EMPTY_NODE;


	for (pNextAddr = &(m_pHashData[nHashNo].pNext); *pNextAddr != NULL; pNextAddr = &(*pNextAddr)->next) 
	{
		if (_tcscmp(pKey, pNext->pNode.pKey) == 0) return eHASH_RESULT_DUPLICATE_KEYS;
	}

	pNext->pNode.pKey = pKey;
	pNext->pNode.value = pData;
	pNext->next = NULL;
	*pNextAddr = pNext;

	if (!m_pHashData[nHashNo].nHashKey) m_pHashData[nHashNo].nHashKey = nHashNo;
	m_pHashData[nHashNo].nElementCnt++;
	m_nElementCount++;
	return nHashNo;
}

size_t CHash::getString(TCHAR *pTitle, TCHAR *pBuf, unsigned int nBufMax)
{
	/*
	TCHAR szBuf[128];
	size_t nSize[5];
	memset(nSize, 0, sizeof(nSize));
	int i, cnt = 0;
	nSize[cnt++] = sizeof(m_pHashData);
	nSize[cnt++] = sizeof(SThash_value);
	_stprintf(szBuf, _T("%s m_pHashData's size:%ul maxCount:%d, unitSize:%d\n"), pTitle, nSize[0], m_nMaxCount, nSize[1]);
	_tcscpy(pBuf, szBuf);
	for (i = 0; i < cnt; i++)
	{
		nSize[4] += nSize[i];
	}
	return nSize[4];
	*/

	SThash_next *pNext;
	TCHAR szBuf[4096], szUnit[512];
	unsigned int i,j, nNextPos=0, nStrSize, nUseCount=0;

	_stprintf(szUnit, _T("%s hashMap key distribution m_nMaxCount[%d]\n"), pTitle, m_nMaxCount);
	nStrSize = (int)_tcslen(szUnit);
	_tcscpy(szBuf + nNextPos, szUnit);
	nNextPos += nStrSize;

	for (i = 0; i < m_nMaxCount; i++)
	{
		j = 0;
		for (pNext = m_pHashData[i].pNext; pNext != NULL; pNext = pNext->next) { j++; }
		if (j) {
			_stprintf(szUnit, _T(" --- m_pHashData[%d] useCount[%d]\n"), i, j);
			nStrSize = (int)_tcslen(szUnit);
			if (nBufMax < nNextPos + nStrSize + 1) goto ENDPROC;
			_tcscpy(szBuf+nNextPos, szUnit);
			nNextPos += nStrSize;

			nUseCount++;
		}
	}
	_stprintf(szUnit, _T(" m_nMaxCount[%d] usedKeyCount[%d]\n"), m_nMaxCount, nUseCount);
	nStrSize = (int)_tcslen(szUnit);
	if (nBufMax < nNextPos + nStrSize + 1) goto ENDPROC;
	_tcscpy(szBuf + nNextPos, szUnit);
	nNextPos += nStrSize;

ENDPROC:
	_tcsncpy(pBuf, szBuf, nNextPos);
	pBuf[nNextPos] = 0;
	return nNextPos;
}




