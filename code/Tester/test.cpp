
#include "test.h"
#include "InfiniA_Library.h"
#include "comCore.h"

struct STParameterThreadObject
{
	int nStartSlotIdx;
	int nTotSendCount;
	int nCurSendCount;
};

bool g_bEventLoop;


//##########################################
// config 파일에 설정한 파라미터 개수만큼 파라미터를 생성하여
// 한번에 Send 한다.
//##########################################
bool test_setParameter(DpParameter *dpParameter, int nParameterCount, int nStartSlotIdx)
{
	DpValue dpValue;
	STParameterData *pParam;
	int nPIdx = nStartSlotIdx, nIdx = 0;
	bool bResult;

	for (int i = 0; i < nParameterCount; i++)
	{
		pParam = g_cMetaData.getParameterNext(&nPIdx, &nIdx);
		if (!pParam) {
			nPIdx = 0; nIdx = 0;
			pParam = g_cMetaData.getParameterNext(&nPIdx, &nIdx);
			if (!pParam) {
				gs_cLogger.DebugLog(LEVEL_ERROR, "pParam is null [%d:%d]", nPIdx, nIdx);
				return false;
			}		
		}

		dpValue = CreateDpValue();
		switch (pParam->nType) {
			case SSCVT::BOOLEAN: SetDpValueBool(dpValue, true);				break;
			case SSCVT::BYTE: SetDpValueByte(dpValue, '1');					break;
			case SSCVT::DOUBLE: SetDpValueDouble(dpValue, 123456.123456);	break;
			case SSCVT::FLOAT: SetDpValueFloat(dpValue, (float)123.123);	break;
			case SSCVT::INT: SetDpValueInt(dpValue, 123456);				break;
			case SSCVT::SHORT: SetDpValueShort(dpValue, 123);				break;
			case SSCVT::LONG : SetDpValueLong(dpValue, 123456);				break;
			case SSCVT::STRING: SetDpValueString(dpValue, "TEST_VALUE");	break;
			default: SetDpValueInt(dpValue, 123456);						break;
		}

		// Add a new parameter to the list inside dpParameter
		bResult = SetDpParameter(*dpParameter, pParam->pSourceId, pParam->pParameterName, dpValue);
		if (!bResult)
		{
			DeleteDpParameter(*dpParameter);
			return false;
		}
		nIdx++;
	}

	return true;
}



atomic_nr g_cAtomicSeq;
#define SEND_PARAMEMTER_TOTCNT 108000
//##########################################
// Thread logic processing function that sends parameters
//##########################################
void parameterThreadProc(STThreadInfo *ptr)
{
	TICKTIME_MILLISEC nTotTime = 0;
	bool bRes;
	// runtime
	if (g_IsLogProcTime) {
		nTotTime = CURRENT_TIME;
	}

	//##########################################################################
	// Library make Data & Send Time
	//##########################################################################
	STParameterThreadObject *pObj = (STParameterThreadObject *)ptr->pObj;

	if (pObj->nCurSendCount == SEND_PARAMEMTER_TOTCNT) {
		ptr->bActive = false;
		return;
	}
	pObj->nCurSendCount++;
	DpParameter dpParameter = CreateDpParameter();
	if (test_setParameter(&dpParameter, g_nSendParamCnt, pObj->nStartSlotIdx))
	{
		bRes = SendDpParameter(dpParameter);
		// runtime over check
		if (g_IsLogProcTime) {
			nTotTime = CURRENT_TIME - nTotTime;
			EqSLog(LOG_INFO, _T("Parameter proc time procTotTime[%d]milliseconds"), nTotTime);
		}
	}
	//##########################################################################

}

//##########################################
// Logic processing function to trigger event
//##########################################
void eventProc(STEventData *pEvent)
{
	if (!EqLib_IsConnected()) return;   // if (AutoReconnect == active) try autoReconnect!

	TICKTIME_MILLISEC nTotTime;
	// runtime
	if (g_IsLogProcTime) {
		nTotTime = CURRENT_TIME;
	}

	if (pEvent->nParamRefCnt) {
		DpParameter dpParameter = CreateDpParameter();
		if (test_setParameter(&dpParameter, pEvent->nParamRefCnt, 0)) {  // any paramter setting
			TriggerDpEventParameter(pEvent->pNodeId, pEvent->pEventId, dpParameter);
		}
	}
	else {
		TriggerDpEvent(pEvent->pNodeId, pEvent->pEventId);
	}

	if (g_IsLogProcTime) {
		nTotTime = CURRENT_TIME - nTotTime;
		// LogName is NULL : [SYSTEM_LOG]
		EqSLog(LOG_INFO, _T("Event proc time procTotTime[%d]milliseconds"), nTotTime);

	}
}



//##########################################
// Resource return
//##########################################
void destroy()
{
	//EqLib_CommDisconnect();
	//g_cCore.destroy();
}

//##########################################
// Create thread to send parameter and start thread
// Start threads by creating thread objects that use the same function as many as the set number
//##########################################
bool parameterSendThreadInitNStart()  // multi thread create & init & start
{
	if (!g_nParameterThreadCnt) return true;

	int nSlotCnt = g_cMetaData.getParameterSlotCount();
	STParameterThreadObject *pObj;
	int i;
	gs_pCore->initComThread(g_nParameterThreadCnt);	// Create thread objects as many as the number of threads
	for (i = 0; i < g_nParameterThreadCnt; i++) {


		pObj = new STParameterThreadObject();
		if(i < nSlotCnt) pObj->nStartSlotIdx = i;
		else pObj->nStartSlotIdx = i-1;

		pObj->nTotSendCount = 36000;
		pObj->nCurSendCount = 0;
		
		// param1:functionPointer, param2:classOject(option), param3:thread sleep millisec, param4:object2
		gs_pCore->addComThread(parameterThreadProc, NULL, g_SleepMilliSec_Param, pObj);
	}
	return true;
}



//##########################################
// 테스트 프로그램 시작 및 Loop
//##########################################
void mainLoop()
{
	int nPIdx, nIdx, i=0;
	STEventData *pEvent;
	parameterSendThreadInitNStart();

	bool bLoop = true;
	while (bLoop) {

		// eventProcLoop
		if (g_isEventSend) {
			nPIdx = 0; nIdx = 0;
			pEvent = g_cMetaData.getEventNext(&nPIdx, &nIdx);
			while (pEvent) {
				if (i == 2) {
					i = 0; 
					Sleep(g_SleepMilliSec_Event);
				}
				else i++;

				eventProc(pEvent);
				nIdx++;
				pEvent = g_cMetaData.getEventNext(&nPIdx, &nIdx);				
			}
		}
		else Sleep(60000); // 1minute
	}
}