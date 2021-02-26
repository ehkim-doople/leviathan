/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

/********************************************************************
2019.11.11 by KEH
-------------------------------------------------------------
String API
*********************************************************************/
#pragma once

#include "comMemManager.h"
char * ehstrstr(const char *pTarget, const char *psubstr);


class CmString
{
public:

	// 메모리 생성
	CmString(void);
	CmString(const TCHAR *str);
	CmString(int size);
	CmString(const CmString &str);
	CmString(const CmString *str);
	~CmString(void);

	inline void push_back(TCHAR c) { if (m_pValue) { m_pValue[_tcslen(m_pValue)] = c; } }
	inline void pop_back() { if (m_pValue) { m_pValue[_tcslen(m_pValue) - 1] = 0; } }

	inline const TCHAR* c_str() const { return m_pValue; }
	inline TCHAR* str() const { return m_pValue; }
	inline int capacity_idx() const {return m_nSftIdx;}
	inline size_t capacity() const { return getSizeByIndex(m_nSftIdx); }
	inline size_t length() const { return _tcslen(m_pValue);  }
	inline size_t size() const { return _tcslen(m_pValue); }
	inline void clear() { if(m_pValue) m_pValue[0] = 0; }
	inline bool empty() const { if (m_pValue && !m_pValue[0]) return true; return false; }

	inline TCHAR& at(size_t index) { return m_pValue[index]; }
	inline TCHAR& back() { return m_pValue[_tcslen(m_pValue) - 1];  }
	inline TCHAR& front() { return m_pValue[0]; }
	inline TCHAR& operator[] (size_t index) { return m_pValue[index]; }
	inline void Print() { _tprintf(_T("%s\n"), m_pValue); }

	inline bool operator!= (const TCHAR *str) { return (this->operator==(str)) ? false : true; }
	inline bool operator!= (CmString &str) { return this->operator!=(str.c_str()); }
	inline bool operator!= (CmString *str) { return this->operator!=(str->c_str()); }
	bool operator== (const TCHAR *str);
	inline bool operator== (CmString &str) { return this->operator==(str.c_str()); }
	inline bool operator== (CmString *str) { return this->operator==(str->c_str()); }
	CmString& operator= (const TCHAR *str);
	inline CmString& operator= (CmString &str) { return this->operator=(str.c_str()); }
	inline CmString& operator= (CmString *str) { return this->operator=(str->c_str()); }
	inline CmString& operator+= (const TCHAR *str) { return append(str); }
	inline CmString& operator+= (const CmString &str) { return append(str); }
	inline CmString& operator+= (const CmString *str) { return append(str); }
	CmString& append(const TCHAR *str);
	inline CmString& append(const CmString &str) { return this->append(str.c_str()); }
	inline CmString& append(const CmString *str) { return this->append(str->c_str()); }
	CmString& replace(int index, int len, const TCHAR *str);
	void reserve(int n = 0);
	TCHAR * find(const TCHAR *str, int index = 0);
	TCHAR * find_next(const TCHAR *str, int index = 0); // 
	inline size_t find_pos(const TCHAR *str, int index = 0) {	return find(str, index) - m_pValue; }
	inline size_t find_nextpos(const TCHAR *str, int index = 0) { return find_next(str, index) - m_pValue; }
	bool compare(const TCHAR *str, E_OPERATOR eOP);
	int compare(const TCHAR *str);
	int compare(int index, int len, const TCHAR *str);
	inline int compare(const CmString *str) { return compare(str->c_str()); }
	inline int compare(const CmString &str) { return compare(str.c_str()); }


private:
	TCHAR *m_pValue;
	int m_nSftIdx;
};


class CStringList
{
public:
	CStringList();
	~CStringList();
	int alloc(int nMaxCount) { m_nListCnt = __alloc(nMaxCount); return m_nListCnt; }
	CmString * CStringList::add_new(TCHAR* pString) {if (!m_pList[m_nDelIdx]) return __add(pString, m_nDelIdx);return __add(pString, newIdx());}
	CmString * CStringList::push_back(TCHAR* pString) {if (!m_pList[m_nLast]) return __add(pString, m_nLast);return __add(pString, newIdx());}
	bool del(CmString *p);
	bool del(int i);
	inline CmString *getNext(int *i) { return __getNext(i);	}
	inline TCHAR *getNext_str(int *i) { CmString *p = __getNext(i); if (p) return p->str(); return NULL; }
	inline CmString *getString(int i) { return m_pList[i]; }
	inline TCHAR *get_str(int i) { CmString *p = m_pList[i]; if (p) return p->str(); return NULL;}
	inline int GETUSECNT() { return m_nUseCnt;	}
	inline int GETMAXCNT() { return m_nListCnt; }
	inline int GETLASTCNT() { return m_nLast; }
private:
	int m_nListCnt;
	int m_nUseCnt;
	int	m_nLast;
	int	m_nDelIdx;
	CmString **m_pList;
	int __alloc(int nMaxCount);
	inline void _DEL(int i) { if (m_pList[i]) { delete m_pList[i];  m_pList[i] = 0; m_nDelIdx = i; m_nUseCnt--;	} }
	inline CmString *__getNext(int *i) {
		for (*i; *i < m_nListCnt; (*i)++) {
			if (m_pList[*i]) { return m_pList[*i]; }
		}
		return NULL;
	}
	CmString *__add(TCHAR *p, int nIdx);
	int newIdx();

};

