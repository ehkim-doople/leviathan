

#include "comDualKeyHash.h"
#include "comLogger.h"

#define ALLOC_HAXH_NEXT_COUNT(max1, max2) (max1+max2+50)

CDKeyHashMap::CDKeyHashMap(int nKey1Max, int nKey2Max, bool bValInternal)
{
	m_nMaxCount1 = nKey1Max;
	m_nMaxCount2 = nKey2Max;
	m_pHashNext = new CBMemPool<SThash_next>(eAlloc_Type_alloc);
	m_pHashNext->alloc(nKey1Max + nKey2Max + 50);
	m_pHashGroup = new CHash(nKey1Max);

	int nBasicCount = nKey2Max << 1;	//(nKey2Max * 2)
	gs_pMMgr->init(IDX_BUF_4, nBasicCount);
	gs_pMMgr->init(IDX_BUF_8, nBasicCount + 50);
	gs_pMMgr->init(IDX_BUF_16, nBasicCount + 50);
	gs_pMMgr->init(IDX_BUF_32, nBasicCount + 50);
	gs_pMMgr->init(IDX_BUF_64, nBasicCount);
	gs_pMMgr->init(IDX_BUF_128, nBasicCount);
	gs_pMMgr->init(IDX_BUF_256, 10);
	gs_pMMgr->init(IDX_BUF_512, 10);

	m_bValInternal = bValInternal;

	if (m_bValInternal) {
		m_pValue = new CBMemPool<STValue>(eAlloc_Type_alloc);
		m_pValue->alloc(nKey2Max + 50);
	}
	else m_pValue = NULL;
}

// 해시맵의 소멸자는 일반적으로
CDKeyHashMap::~CDKeyHashMap()
{
	SThash_next *pNext1, *pNext2, *pDelNode;
	CHash *pHash;
	unsigned int i,j;
	
	// 이 객체가 프로세스와 생명주기를 같이 한다면, 굳이 아래와 같은 수고를 할 필요는 없다. OS 에서 처리해 주기 때문이다.
	// 그럼에도 불구하고 일일이 메모리 해제를 하는 이유는 메모리 관리를 명확히 하여 내 자신이 메모리를 빠짐없이 해제함으로써 관리에 빈틈이 없는지 점검하기 위함이다.
	// 초기 할당량이 부족해 추가로 동적할당이 이루어진 메모리들의 해제를 위해서는 각각의 메모리를 아래와 같이 반환해야한다.
	for (i = 0; i < m_nMaxCount1; i++) {
		pNext1 = m_pHashGroup->getNext(i);
		while (pNext1) {
			pHash = (CHash *)pNext1->pNode.value;
			for (j = 0; j < m_nMaxCount2; j++) {
				pNext2 = pHash->getNext(j);
				while (pNext2) {
					gs_pMMgr->delBuf(pNext2->pNode.pKey, STRING_SIZE(pNext2->pNode.pKey));	// key2 메모리 삭제
					if(m_bValInternal)delValueBuf((STValue *)pNext2->pNode.value);					// key2 의 value 메모리 삭제
					pDelNode = pNext2;
					pNext2 = pNext2->next;
					m_pHashNext->delMem(pDelNode);												// key2,value,next 포인터 node 삭제
				}
			}
			gs_pMMgr->delBuf(pNext1->pNode.pKey, STRING_SIZE(pNext1->pNode.pKey));	// key1 메모리 삭제
			delete pHash;																	// key1 의 value 메모리 삭제
			pDelNode = pNext1;
			pNext1 = pNext1->next;
			m_pHashNext->delMem(pDelNode);												// key1,value,next 포인터 node 삭제
		}
	}

	// 아래 루틴은 초기에 할당된 메모리 공간을 빠르게 해제한다.
	delete m_pValue;
	delete m_pHashNext;
	delete m_pHashGroup;
}


int CDKeyHashMap::addKey(TCHAR *pKey1, TCHAR *pKey2, CHash **pTarget, void *pHashValue)
{
	SThash_next *pNext;
	CHash *pHash;
	TCHAR *pBuf;
	int nRes1=0, nRes2=0;


	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	if (pNode) {
		pHash = (CHash *)pNode->value;
		if (pHash->hashLookup(pKey2)) {
			gs_cLogger.DebugLog(LEVEL_ERROR, "DUPLICATE_KEYS pKey1[%s] pHash->hashLookup(pKey2[%s])", pKey1, pKey2);
			return eHASH_RESULT_DUPLICATE_KEYS;
		}
	}
	else {
		pHash = new (std::nothrow)CHash(m_nMaxCount2);					// key1 의 value 메모리 할당
		if (!pHash) return eHASH_RESULT_FAIL_ALLOCATION;
		pNext = m_pHashNext->newMem();						// key1,value,next 포인터 node 할당
		if(!pNext) return eHASH_RESULT_FAIL_ALLOCATION;
		pBuf = (TCHAR*)gs_pMMgr->newBuf(STRING_SIZE(pKey1));				// key1 메모리 할당
		if(!pBuf) return eHASH_RESULT_FAIL_ALLOCATION;
		_tcscpy(pBuf, pKey1);
		nRes1 = m_pHashGroup->addNode(pNext, pBuf, (void *)pHash);	// 1차 해시맵에 추가
		if (nRes1 < 0) {
			gs_cLogger.DebugLog(LEVEL_ERROR, "errorNo[%d] new CHash(max:%d) key1:%s", nRes1, m_nMaxCount2, pBuf);
			return nRes1;
		}
		gs_cLogger.PutLogQueue(LEVEL_DEBUG, _T("new CHash(max:%d) key1:%s hashNo[%d]"), m_nMaxCount2, pBuf, nRes1);
	}

	pNext = m_pHashNext->newMem();						// key2, value, next 포인터 node 할당
	if (!pNext) return eHASH_RESULT_FAIL_ALLOCATION;
	pBuf = (TCHAR*)gs_pMMgr->newBuf(STRING_SIZE(pKey2));				// key2 메모리 할당
	if (!pBuf) return eHASH_RESULT_FAIL_ALLOCATION;
	_tcscpy(pBuf, pKey2);
	if (pHashValue) pHash->setHashValue(pHashValue);
	if(pTarget) *pTarget = pHash;
	// 2차 해시맵에 추가 (key2 의 value 는 셋팅하지 않음)
	nRes2 = pHash->addNode(pNext, pBuf); // 2차 해시맵에 추가 (key2 의 value 는 셋팅하지 않음)
	if (nRes2 < 0) {
		gs_cLogger.PutQueue(LEVEL_ERROR, "errorNo[%d] pHash->addNode(pNext, %s)", nRes2, pBuf);
		return nRes2;
	}
	return nRes2;
}

int CDKeyHashMap::addKeyValue(TCHAR *pKey1, TCHAR *pKey2, void *pValue, void *pHashValue)	// key 중복시 return ERROR_CODE
{
	CHash *pHash;
	int nRes; // 1. key2's hashKey -> 2. nRes
	nRes = addKey(pKey1, pKey2, &pHash);
	if (pHash) {
		if(pHashValue) pHash->setHashValue(pHashValue);
		if (nRes >= 0) {
			nRes = pHash->setValue(nRes, pKey2, pValue);
		}
	}
	return nRes;
}

int CDKeyHashMap::setValue(TCHAR *pKey1, TCHAR *pKey2, void *pValue)
{
	CHash *pHash;
	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	if (pNode) {
		pHash = (CHash *)pNode->value;
		pNode = pHash->hashLookup(pKey2);
		if (pNode) {
			if (pNode->value) { delValueBuf((STValue *)pNode->value); }
			pNode->value = pValue; 
			return eHASH_RESULT_SUCESS;
		}
	}
	return eHASH_RESULT_INVALID_KEY;
}


bool CDKeyHashMap::isNode(TCHAR *pKey1, TCHAR *pKey2)
{
	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	if (pNode) {
		CHash *pHash = (CHash *)pNode->value;
		if (pHash->hashLookup(pKey2)) return true;
	}
	return false;
}

CHash * CDKeyHashMap::getHashMap(TCHAR *pKey1)
{
	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	return (CHash *)pNode->value;		// return NULL 가능
}

STHash_Node * CDKeyHashMap::getNode(TCHAR *pKey1, TCHAR *pKey2)
{
	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	if (pNode) {
		CHash *pHash = (CHash *)pNode->value;
		return pHash->hashLookup(pKey2);
	}
	return NULL;
}

bool CDKeyHashMap::deleteHash(TCHAR *pKey1)
{
	SThash_next **ppPrev=NULL;
	SThash_next *pNext2;
	SThash_next *pCur = m_pHashGroup->getNode(pKey1, ppPrev);
	CHash *pHash;
	unsigned int j;

	if (pCur) {
		*ppPrev = pCur->next; // NULL 셋팅 가능.

		pHash = (CHash *)pCur->pNode.value;
		for (j = 0; j < m_nMaxCount2; j++) {
			pNext2 = pHash->getNext(j);
			while (pNext2) {
				gs_pMMgr->delBuf(pNext2->pNode.pKey, STRING_SIZE(pNext2->pNode.pKey));	// key2 메모리 삭제
				if (m_bValInternal) delValueBuf((STValue *)pNext2->pNode.value);	// key2 의 value 메모리 삭제
				m_pHashNext->delMem(pNext2);										// key2,value,next 포인터 node 삭제
				pNext2 = pNext2->next;
			}
		}
		gs_pMMgr->delBuf(pCur->pNode.pKey, STRING_SIZE(pCur->pNode.pKey));	// key1 메모리 삭제
		delete pHash;															// key1 의 value 메모리 삭제
		m_pHashNext->delMem(pCur);												// key1,value,next 포인터 node 삭제

		return true;
	}		
	return false;
}



bool CDKeyHashMap::deleteNode(TCHAR *pKey1, TCHAR *pKey2)
{
	SThash_next **ppPrev=NULL;
	SThash_next *pNext1, *pCur2;

	pNext1 = m_pHashGroup->getNext(pKey1);

	if (pNext1) {
		CHash *pHash = (CHash *)pNext1->pNode.value;
		pCur2 = pHash->getNode(pKey2, ppPrev);
		if (pCur2) {
			*ppPrev = pCur2->next;  // NULL 셋팅 가능.
			gs_pMMgr->delBuf(pCur2->pNode.pKey, STRING_SIZE(pCur2->pNode.pKey));	// key2 메모리 삭제
			if (m_bValInternal)delValueBuf((STValue *)pCur2->pNode.value);	// key2 의 value 메모리 삭제
			m_pHashNext->delMem(pCur2);										// key2,value,next 포인터 node 삭제

			return true;
		}
	}
	return false;
}


STValue * CDKeyHashMap::newValueBuf(int size, TCHAR *pValue)			// pValue 는 string 또는 구조체 가능; 해당 사이즈 크기의 메모리 할당하여 memcpy 함
{
	int nIdx;
	STValue *pSTValue = m_pValue->newMem();
	TCHAR *pValueBuf = gs_pMMgr->newBuf(size, &nIdx);
	c_memcpy(pValueBuf, pValue, size);
	pSTValue->nIdx = nIdx;
	pSTValue->pValue = pValueBuf;

	return pSTValue;
}


void CDKeyHashMap::delValueBuf(STValue *pValue) 
{ 
	if (m_bValInternal) {
		gs_pMMgr->delBufByIndex((TCHAR *)pValue->pValue, pValue->nIdx); m_pValue->delMem(pValue);
	}

	// 이외는 외부 할당... 외부에서 반환처리
} // value 로 사용한 동적 메모리 반환
