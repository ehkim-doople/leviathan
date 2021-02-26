/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
	2016.12.12 by KEH
	-------------------------------------------------------------
	thread-safety Memory Manager
	NOT USE THIS CLASS!!!!!!!!
	High Dependendy on buf Size

	if you want to use it, you must know the buf Size

*********************************************************************/
#pragma once

#include "comBufPool.h"

class CMemManager
{
public :
	CMemManager();
	~CMemManager();
	void init(STBufConf *pConf);
	inline void init(E_IDX_BUF eIdx, int nMax) { m_stConfig.nMaxList[eIdx] += nMax; }
	int init(TCHAR *pStrSize, int nCount);

	bool alloc();
	inline CBufPool * getBufPool(int nIdx) { if (IDX_BUF_4 > nIdx || IDX_BUF_2m < nIdx) return NULL; return m_pBufList[nIdx]; }
	inline char *newBuf(int nSize) { int nIdx = getIndex(nSize);  return __newBuf(nIdx, nSize); }
	inline char *newBuf(int nSize, int *pIndex) { *pIndex = getIndex(nSize);  return __newBuf(*pIndex, nSize); }
	inline char *newBufByIndex(int nIdx) { return __newBuf(nIdx); }
	void newBuf(int nSize, STBuf *pBuf);

	inline void delBuf(char *pBuf, int nSize) { int nIdx = getIndex(nSize); delBufByIndex(pBuf, nIdx); }
	inline void delBuf(STBuf *pRes) {delBufByIndex(pRes->pValue, pRes->nIdx); pRes->pValue = 0; pRes->nIdx = 0;}
	void delBufByIndex(char *pBuf, int nIndex);

	int getStrToInt(TCHAR *pData);
	char *	newString(char *pString, int *pIdx=NULL);
	void	delString(char *pString, int nIdx=0);
	void newString(STBuf *pRes, char *pSource, int nSourceSize=0);

private :
	CBufPool *m_pBufList[IDX_BUF_MAX];
	STBufConf m_stConfig;
	char * __newBuf(int nIdx, size_t size=0);
	int m_nOverMaxIdx;
};


extern CMemManager *gs_pMMgr;
void OBJ_DELETE(E_ALLOC_TYPE type, void *p);
