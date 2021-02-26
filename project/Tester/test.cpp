
#include "test.h"
#include "InfiniA_Library.h"
#include "comCore.h"

struct STParameterThreadObject
{
	int nStartSlotIdx;
	int nCurSendCount;
};

int g_nTraceThreadCount;


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

atomic_nr g_nSeq;
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

	if (0 < g_nTotParameterSendCount) {
		if (g_nTotParameterSendCount <= pObj->nCurSendCount) {
			ptr->bActive = false;
			g_nTraceThreadCount--;
			return;
		}
		pObj->nCurSendCount++;
	}
	DpParameter dpParameter = CreateDpParameter();
	if (test_setParameter(&dpParameter, g_nSendParamCnt, pObj->nStartSlotIdx))
	{
		bRes = SendDpParameter(dpParameter);
		// runtime over check
		if (g_IsLogProcTime) {
			nTotTime = CURRENT_TIME - nTotTime;
			EqSLog(LOG_INFO, _T("[%d] Parameter proc time procTotTime[%d]milliseconds"), g_nSeq.atomic_increment(), nTotTime);
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

	if (g_nParameterCntWithEventTrigger) {
		DpParameter dpParameter = CreateDpParameter();
		if (test_setParameter(&dpParameter, g_nParameterCntWithEventTrigger, 0)) {  // any paramter setting
			TriggerDpEventParameter(pEvent->pNodeId, pEvent->pEventId, dpParameter);
		}
	}
	else {
		TriggerDpEvent(pEvent->pNodeId, pEvent->pEventId);
	}

	if (g_IsLogProcTime) {
		nTotTime = CURRENT_TIME - nTotTime;
		EqSLog(LOG_INFO, _T("[%d] Event proc time procTotTime[%d]milliseconds"), g_nSeq.atomic_increment(), nTotTime);

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

	g_nTraceThreadCount = 0;

	int nSlotCnt = g_cMetaData.getParameterSlotCount();
	STParameterThreadObject *pObj;
	int i;
	gs_pCore->initComThread(g_nParameterThreadCnt);	// Create thread objects as many as the number of threads
	for (i = 0; i < g_nParameterThreadCnt; i++) {


		pObj = new STParameterThreadObject();
		if(i < nSlotCnt) pObj->nStartSlotIdx = i;
		else pObj->nStartSlotIdx = i-1;

		pObj->nCurSendCount = 0;
		
		// param1:functionPointer, param2:classOject(option), param3:thread sleep millisec, param4:object2
		gs_pCore->addComThread(parameterThreadProc, NULL, g_SleepMilliSec_Param, pObj);
		g_nTraceThreadCount++;
	}
	return true;
}

void eventSendLoop()
{
	STEventData *pEvent;
	int nPIdx, nIdx, i = 0;
	nPIdx = 0; nIdx = 0;
	pEvent = g_cMetaData.getEventNext(&nPIdx, &nIdx);

	int nSendCount = 0;

	while (pEvent) {
		if (i == 2) {
			i = 0;
			Sleep(g_SleepMilliSec_Event);
		}
		else i++;

		if (0 < g_nTotEventSendCount) {
			if (g_nTotEventSendCount <= nSendCount) {
				EqSLog(LOG_INFO, _T("Event Send Proc Complete nSendCount[%d]"), nSendCount);
				break;
			}
			nSendCount++;
		}

		eventProc(pEvent);

		nIdx++;
		pEvent = g_cMetaData.getEventNext(&nPIdx, &nIdx);
		if (!pEvent) { // from the beginning again
			nPIdx = 0; nIdx = 0;
			pEvent = g_cMetaData.getEventNext(&nPIdx, &nIdx);
		}
	}
}


//##########################################
// 테스트 프로그램 시작 및 Loop
//##########################################
void mainLoop()
{
	parameterSendThreadInitNStart();

	//Sleep(2000);

	bool bLoop = true;
	while (bLoop) {

		// eventProcLoop
		if (g_nTotEventSendCount) {
			eventSendLoop();
			g_nTotEventSendCount = 0;
		}
		else {
			if (!g_nTraceThreadCount) {
				EqLib_CommDisconnect();
				g_nTraceThreadCount = -1;
			}
			Sleep(60000); // 1minute
			printf("Sleep\n");
		}
	}
}