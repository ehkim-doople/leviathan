// LibraryTester.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include "test.h"
#include "init.h"
#include "comCore.h"
#include "exception_handler.h"

int g_nTotEventSendCount;
int g_SleepMilliSec_Param;
int g_SleepMilliSec_Event;
int g_nSendParamCnt;
int g_nParameterThreadCnt;
int g_IsLogProcTime; // InfiniA_Library Proc Time threshold
int g_nTotParameterSendCount;
int g_nParameterCntWithEventTrigger;


// 실제 메타데이터를 읽어와서 테스트 한다.
CMetaData g_cMetaData;


// 라이브러리를 테스트 하기 위해 config 설정을 통해 테스트 환경을 구축한다.
bool initTestProgramConfig()
{
	char szDir[256], szBuf[256];
	DWORD nRes;

	// 이벤트 데이터를 보낼지 여부
	g_nTotEventSendCount = GetPrivateProfileInt(_T("PROGRAM"), _T("TOTAL_EVENT_SEND_COUNT"), 0, g_pProcessConfig);

	// 쓰레드 하나가 파라미터를 보내는 주기 설정
	g_SleepMilliSec_Param = GetPrivateProfileInt(_T("PROGRAM"), _T("SLEEP_MILISEC_PARAM"), 0, g_pProcessConfig);

	// 이벤트 Loop 에서 이벤트 하나를 보내는 주기
	g_SleepMilliSec_Event = GetPrivateProfileInt(_T("PROGRAM"), _T("SLEEP_MILISEC_EVENT"), 0, g_pProcessConfig);

	nRes = GetProfilePath(_T("PROGRAM"), _T("METADATA_PATH"), _T("Data//"), szDir, g_pProcessConfig);
	_stprintf(szBuf, _T("%s%s"), szDir, _T("Data_Parameter.txt"));
	g_cMetaData.setParamPath(szBuf);
	nRes = GetProfilePath(_T("PROGRAM"), _T("METADATA_PATH"), _T("Data//"), szDir, g_pProcessConfig);
	_stprintf(szBuf, _T("%s%s"), szDir, _T("Data_EventMap.txt"));
	g_cMetaData.setEventPath(szBuf);

	// 파라미터를 보낼때 한꺼번에 보내는 파라미터 갯수 설정
	g_nSendParamCnt = GetPrivateProfileInt(_T("PROGRAM"), _T("SEND_PARAMCNT"), 100, g_pProcessConfig);

	// 파라미터만 보내는 쓰레드의 총 개수 설정(멀티쓰레드 테스트를 위함)
	g_nParameterThreadCnt = GetPrivateProfileInt(_T("PROGRAM"), _T("PARAMETER_THREAD_COUNT"), 0, g_pProcessConfig);

	// 테스트 프로그램에서 데이터를 만들고 Send 하는데 까지 걸리는 총시간이 아래의 밀리세컨드를 넘어가면 로그 남길지 여부 설정
	// 50 milliseconds 를 설정하여, 데이터를 만들어서 보내는 데까지 걸리는 총시간이 50미리세컨드를 넘어갈때마다 로그를 남기도록 설정한다.
	g_IsLogProcTime = (unsigned int)GetPrivateProfileInt(_T("PROGRAM"), _T("IS_LOG_PROC_TIME"), 0, g_pProcessConfig);
	// 디폴트 0 : 위의 기능을 설정하지 않음

	g_nTotParameterSendCount = GetPrivateProfileInt(_T("PROGRAM"), _T("TOTAL_PARAMETER_SEND_COUNT"), 0, g_pProcessConfig);;
	g_nParameterCntWithEventTrigger = GetPrivateProfileInt(_T("PROGRAM"), _T("SEND_PARAMCNT_WITH_EVENT"), 0, g_pProcessConfig);;

	return true;
}



int main()
{
	exception_handler::instance()->initialize("handler.dmp");
	exception_handler::instance()->run();


	// 테스트 프로그램 구성을 위한 config 설정
	if (!initTestProgramConfig()) {
		return 0;
	}

	// 메타데이터 메모리맵 초기화
	if (!g_cMetaData.init()) {
		return 0;
	}

	// InfinA_Library.dll 을 사용하기 위해 해당 라이브러리 초기화
	if (!initConfig_forInfiniALibrary()) {
		return 0;
	}


	if(EqLib_CommConnect()) {
		printf("EDA Server Connect Success!!\n");
	}
	else {
		printf(EqLib_GetCurrentError()); // error Msg
	}

	// 다음의 mainLoop 에서 주기적으로 이벤트를 전송함
	mainLoop();

	return 0;
}

