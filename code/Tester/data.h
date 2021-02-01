#pragma once

#include "comFileUtil.h"
#include "comMString.h"
#include "comBMemPool.h"

namespace SSCVT {
	enum E_ValueType
	{
		NONE = 0,
		DOUBLE,
		FLOAT,
		LONG,
		INT,
		SHORT,
		BYTE,
		STRING,
		DATETIME,
		BOOLEAN,
		ANYURI,
		BASE64BINARY,
		ENUM_INT,
		ENUM_STRING,
		STRUCTURE,
		ARRAY,
		VARIABLE,
		NOVALUE,
		MAX
	};
}


enum E_FIELD_PARAM
{
	eP_column_Seq = 0,
	eP_column_SID,
	eP_column_PName,
	eP_column_Type1,
	eP_column_Type2
};

enum E_FIELD_EVENT
{
	eE_column_Seq = 0,
	eE_column_Type,
	eE_column_NID,	// nodeId
	eE_column_EID,	// eventId
	eE_column_EName,
	eE_column_ParamRefCnt,
	eE_column_SemiLabel,
	eE_column_ObjType,
	eE_column_ObjId
};

struct STParameterData
{
	char *pSourceId;
	char *pParameterName;
	SSCVT::E_ValueType nType;
};

struct STEventData
{
	char *pNodeId;
	char *pEventId;
	char *pEventName;
	int nParamRefCnt;
	bool bSemiObj;
	char *pObjType;
	char *pObjId;
};


class CMetaData {
public:
	CMetaData();
	~CMetaData();
	bool init();
	STParameterData * getParameter(int nSlot, int nIdx) { return m_pParamPool->getUseMem(nSlot,nIdx); }
	STEventData * getEvent(int nSlot, int nIdx) { return m_pEventPool->getUseMem(nSlot, nIdx); }
	STParameterData * getParameterNext(int *pSlot, int *pIdx) { return m_pParamPool->getSlotNext(pSlot, pIdx); }
	STEventData * getEventNext(int *pSlot, int *pIdx) { return m_pEventPool->getSlotNext(pSlot, pIdx); }
	inline void setParamPath(TCHAR *p) {
		_tcscpy(m_szParamPath, p);
	}
	inline void setEventPath(TCHAR *p) {
		_tcscpy(m_szEventPath, p);
	}
	inline int getParameterSlotCount() { return m_pParamPool->GETSLOTLASTCOUNT(); }
private:
	// The data below is maintained until the end of the process.
	CTList<CStringList> *m_pParamList;  // file string set
	CTList<CStringList> *m_pEventList;
	TCHAR m_szParamPath[128];
	TCHAR m_szEventPath[128];
	CBPoolSlot<STEventData>		*m_pEventPool;	// Points to the column string in CStringList.
	CBPoolSlot<STParameterData>  *m_pParamPool;

	void dataParsingNSet();
};