/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
2019.01.13 by KEH
-------------------------------------------------------------
Dual Hansh Map Library
*********************************************************************/

#pragma once

#include "comHash.h"
#include "comBMemPool.h"
//#include "comMemPool.h"
#include "comMemManager.h"

// 이 클래스를 사용할때, 메모리 할당에 대해 전혀 고려하지 말아야 한다.
// write 를 위한 작업에 대해 이 클래스는 싱글 쓰레드 전용이다. 멀티쓰레드 접근시 큐를 써서 싱글 쓰레드로 처리하도록, 단, 조회는 관계없음
struct STValue
{
	int nIdx; // 메모리 할당 index, -1 이면 외부할당
	void *pValue;
};



class CDKeyHashMap
{
public :
	CDKeyHashMap(int nKey1Max, int nKey2Max, bool bValInternal=false);					// 해시키 개수를 초기에 설정해야 한다.
	~CDKeyHashMap();

	int addKey(TCHAR *pKey1, TCHAR *pKey2, CHash **pTarget=NULL, void *pHashValue=NULL);							// 내부에서 메모리 자동 할당

	// key1, key2 내부에서 메모리 자동 할당
	int addKeyValue(TCHAR *pKey1, TCHAR *pKey2, void *pValue, void *pHashValue=NULL); 

	// 이미 key1,key2 가 등록이 되어있어야만 성공
	int setValue(TCHAR *pKey1, TCHAR *pKey2, void *pValue);
	bool deleteHash(TCHAR *pKey1);
	bool deleteNode(TCHAR *pKey1, TCHAR *pKey2);

	// pValue 는 string 또는 구조체 가능; 해당 사이즈 크기의 메모리 할당하여 memcpy 함
	STValue * newValueBuf(int size, TCHAR *pValue);
	void delValueBuf(STValue *pValue);
	inline STValue * getValueBuf() { return m_pValue->newMem(); }

	bool isNode(TCHAR *pKey1, TCHAR *pKey2);
	CHash * getHashMap(TCHAR *pKey1);
	STHash_Node * getNode(TCHAR *pKey1, TCHAR *pKey2);

	// for Loop
	CHash *getHashGroup() { return m_pHashGroup; }

private :
	CBMemPool<SThash_next> *m_pHashNext;	
	CBMemPool<STValue> *m_pValue;
	CHash *m_pHashGroup;
	HASH_TYPE m_nMaxCount1, m_nMaxCount2;
	bool m_bValInternal;

	inline SThash_next *newNode() { return m_pHashNext->newMem();  }
	inline bool delNode(SThash_next *p) { return m_pHashNext->delMem(p); }
};