#include "stdio.h"
#include <Windows.h>
#include "functionTest.h"

void functionTest()
{
	printf("InfiniA_Library Version : %s\n", EqLib_GetVersion());
	//function_LogInit_Test();
	//function_Group1_Test();
	//function_Group2_Test();
	function_Recommended_Init();
	function_SemiParameterSend_Test();
	function_ParameterSend_Test1();
	function_ParameterSend_Test2();
	function_TriggerEvent_Test();

	//*********************************************
	// Redis-related grammar check complete
	// metadata mismatch error occurred!
	//*********************************************
	// function_TriggerTransientEvent_Test();    
	// unction_TriggerTransientEventParameter_Test();	 

	function_TriggerEventParameter_Test();
	function_TriggerException_Test();
	function_TriggerExceptionParameter_Test();
	function_TriggerOnDpSEMIObj_Test();
	function_TriggerOffDpSEMIObj_Test();
	function_SendAdhocDpParameter_Test();
	
	Sleep(10);
	
	EqLib_CommDisconnect();

	Sleep(100);
}
/*
bool __stdcall EqLogInit(const char* _szLogName,  const char *_szLogPath, enum LogType _eLogType, enum LogLevel _eLogLevel, const char *_szFilePrefix, const char *_szFileExtension, int _nSizeLimit);
void __stdcall EqLogEnable(bool _bEnable);
bool __stdcall EqLogEnableOne(bool _bEnable, const char* _szLogName);
bool __stdcall EqLogSwitchLevel(const char* _szLogName,enum LogLevel _eLogLevel);
bool __stdcall EqLog(const char* _szLogName,enum LogLevel _eLogLevel,const char *_szFormat,...);
void __stdcall EqSLog(enum LogLevel _eLogLevel, const char *_szFormat, ...);

*/
void function_LogInit_Test()
{
	bool bRes;

	// 	_szLogPath : Allows both relative and absolute paths
	bRes = EqLogInit("1", "~/Log/SendData", LOG_DAILY, LOG_INFO, "transDataLog", "log", 10000);
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}
	bRes = EqLogInit("2", "~/Log/", LOG_FILE, LOG_INFO, "systemLog", "log", 10000);
	if (!bRes) {
		printf("%s\n",EqLib_GetCurrentError());
	}

	bRes = EqLog("0", LOG_INFO, "test");
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}
	bRes = EqLog("1", LOG_INFO, "1 Log test name[transDataLog]");
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}
	bRes = EqLog("2", LOG_INFO, "2 Log test name[systemLog]");
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}
	EqLogEnable(false);
	EqLog("1", LOG_INFO, "EqLogEnable(false) 1 Log test name[transDataLog]");
	EqLog("2", LOG_INFO, "EqLogEnable(false) 2 Log test name[systemLog]");
	EqLogEnable(true);
	bRes = EqLog("1", LOG_INFO, "EqLogEnable(true) 1 Log test name[transDataLog]");
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}
	bRes = EqLog("2", LOG_INFO, "EqLogEnable(true) 2 Log test name[systemLog]");
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}
	bRes = EqLogEnableOne(false, "3");
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}
	bRes = EqLogEnableOne(false, "2");
	if (bRes) {
		EqLog("2", LOG_INFO, "EqLogEnableOne(false) 2 Log test name[systemLog]");
	}
	else {
		printf("%s\n", EqLib_GetCurrentError());
	}
	EqLogEnable(true);
	EqLog("1", LOG_INFO, "2 EqLogEnable(true) 1 Log test name[transDataLog]");
	EqLog("2", LOG_INFO, "2 EqLogEnable(true) 2 Log test name[systemLog]");

	bRes = EqLogSwitchLevel("3", LOG_WARNING);
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}

	bRes = EqLogSwitchLevel("2", LOG_WARNING);
	if (bRes) {
		EqLog("2", LOG_INFO, "EqLogSwitchLevel[LOG_WARNING] 2 Log test name[systemLog]");
		EqLog("2", LOG_WARNING, "EqLogSwitchLevel[LOG_WARNING] 2 Log test name[systemLog]");
	}
	else {
		printf("%s\n", EqLib_GetCurrentError());
	}

	EqLogSwitchLevel("2", LOG_INFO);
}


// bool __stdcall EqLogInitByConfig(const char *_szConfigFileName);
void function_Recommended_Init()
{
	// Allows both relative and absolute paths
	// auto Reconnect true!
	bool bRes = EqLogInitByConfig("~/InfiniA-Library.ini");
	if (bRes) {
		EqSLog(LOG_INFO, "EqSLog test1");
	}
	else {
		printf("EqLogInitByConfig Error[%s]", EqLib_GetCurrentError());
	}
	bRes = EqLib_CommInit("192.168.0.204", 6379, 1000);
	if (!bRes) {
		printf("EqLib_CommInit Error[%s]", EqLib_GetCurrentError());
	}
	EqLib_SetStopwatch(true, NULL, 10);
	bRes = EqLib_CommConnect();
	if (!bRes) {
		printf("EqLib_CommConnect Error[%s]", EqLib_GetCurrentError());
	}
}

/*
void __stdcall EqLib_SetStopwatch(bool _bFact, char * _szPath, unsigned int _uThresholdmsec);
bool __stdcall EqLib_CommInit(const char *_szIpAddress, const int _nPort, const int _nTimeoutSec, bool _bLogTransmitted = false, const char* _szLogName = nullptr);
bool __stdcall EqLib_CommConnect();
bool __stdcall EqLib_CommDisconnect();
const char* __stdcall EqLib_GetCurrentError(void);
bool __stdcall EqLib_IsConnectable(void);
bool __stdcall EqLib_IsConnected(void);
*/
void function_Group1_Test()
{
	EqLib_SetStopwatch(true, NULL, 10);
	bool bRes = EqLib_CommInit("192.168.0.204", 6379, 1000, true, "3");
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}

	bRes = EqLib_CommInit("192.168.0.204", 6379, 1000, true, "1");
	if (!bRes) {
		printf("%s\n", EqLib_GetCurrentError());
	}

	while (!EqLib_IsConnectable()) {
		printf("%s", EqLib_GetCurrentError());
		printf("%s", EqLib_IsConnected()?"true":"false");
		Sleep(5000);  // 5 sec
	}
	printf("EqLib_IsConnected bRes[%s]\n", EqLib_IsConnected() ? "true" : "false");
	
	bRes = EqLib_CommDisconnect();
	if (bRes) {
		printf("EqLib_IsConnected bRes[%s]\n", EqLib_IsConnected() ? "true" : "false");
	}
	else {
		printf("%s\n", EqLib_GetCurrentError());
	}

	bRes = EqLib_CommConnect();
	if (bRes) {
		printf("EqLib_IsConnected bRes[%s]\n", EqLib_IsConnected() ? "true" : "false");
	}
	else {
		printf("%s\n", EqLib_GetCurrentError());
	}
}

/*

DpValue __stdcall CreateDpValue(void);
void __stdcall SetDpValueDouble(DpValue &_dpValue, double _dValue);
void __stdcall SetDpValueFloat(DpValue &_dpValue, float _fValue);
void __stdcall SetDpValueLong(DpValue &_edaValue, long _lValue);
void __stdcall SetDpValueUnsignedLong(DpValue &_edaValue, unsigned long _ulValue );
void __stdcall SetDpValueInt(DpValue &_edaValue, int _nValue );
void __stdcall SetDpValueUnsignedInt(DpValue &_edaValue, unsigned int _unValue );
void __stdcall SetDpValueShort(DpValue &_edaValue, short int _nValue );
void __stdcall SetDpValueUnsignedShort(DpValue &_edaValue, unsigned short int _unValue);
void __stdcall SetDpValueByte(DpValue &_edaValue, char _chValue );
bool __stdcall SetDpValueString(DpValue &_edaValue, const char *_szValue);
void __stdcall SetDpValueBool(DpValue &_edaValue, bool _bValue);
bool __stdcall ReleaseDpValue(DpValue &_edaValue);

*/

void function_Group2_Test()
{
	DpValue vDoubleValue = CreateDpValue();
	printf("value address : %p\n", vDoubleValue.ptr);
	SetDpValueDouble(vDoubleValue, 0.000555001);
	printf("SetDpValue address : %p\n", vDoubleValue.ptr);
	ReleaseDpValue(vDoubleValue);
	printf("ReleaseDpValue address : %p\n", vDoubleValue.ptr);
}

//bool __stdcall SetDpValueDpValue(DpValue &_edaValue, DpValue &_edaChildValue);
DpValue function_ComplexData_Set()
{
	DpValue vIntValue, vUIntValue, vFloatValue, vLongValue, vULongValue, vShortValue, vUShortValue, vByteValue, vBoolValue;
	DpValue vComplexValue = CreateDpValue();
	DpValue vSubComplexValue_step1 = CreateDpValue();
	DpValue vSubComplexValue_step2 = CreateDpValue();

	vFloatValue = CreateDpValue();
	SetDpValueFloat(vFloatValue, (float)0.258);

	vLongValue = CreateDpValue();
	SetDpValueLong(vLongValue, 10000);

	vULongValue = CreateDpValue();
	SetDpValueUnsignedLong(vULongValue, 100000000);

	vIntValue = CreateDpValue();
	SetDpValueInt(vIntValue, 2000000);

	vUIntValue = CreateDpValue();
	SetDpValueUnsignedInt(vUIntValue, 200000000);

	vShortValue = CreateDpValue();
	SetDpValueShort(vShortValue, 3000);

	vUShortValue = CreateDpValue();
	SetDpValueUnsignedShort(vUShortValue, 30000);

	vByteValue = CreateDpValue();
	SetDpValueByte(vByteValue, 1);

	vBoolValue = CreateDpValue();
	SetDpValueBool(vBoolValue, 0);

	// error case !
	bool bRes = SetDpValueDpValue(vComplexValue, vSubComplexValue_step1);
	if (!bRes) {
		printf("EqLib_GetCurrentError() : %s\n", EqLib_GetCurrentError());
		printf("g_vComplexValue.szErrorReason : %s\n", vComplexValue.szErrorReason);
	}

	bRes = SetDpValueDpValue(vComplexValue, vShortValue);
	if (!bRes) {
		printf("%s\n", vComplexValue.szErrorReason);
	}
	bRes = SetDpValueDpValue(vComplexValue, vUIntValue);
	if (!bRes) {
		printf("%s\n", vComplexValue.szErrorReason);
	}
	bRes = SetDpValueDpValue(vSubComplexValue_step2, vFloatValue);
	if (!bRes) {
		printf("%s\n", vSubComplexValue_step2.szErrorReason);
	}
	bRes = SetDpValueDpValue(vSubComplexValue_step2, vLongValue);
	if (!bRes) {
		printf("%s\n", vSubComplexValue_step2.szErrorReason);
	}
	bRes = SetDpValueDpValue(vSubComplexValue_step1, vSubComplexValue_step2);
	if (!bRes) {
		printf("%s\n", vSubComplexValue_step1.szErrorReason);
	}
	bRes = SetDpValueDpValue(vSubComplexValue_step1, vBoolValue);
	if (!bRes) {
		printf("%s\n", vSubComplexValue_step1.szErrorReason);
	}
	bRes = SetDpValueDpValue(vComplexValue, vSubComplexValue_step1);
	if (!bRes) {
		printf("%s\n", vComplexValue.szErrorReason);
	}
	/* RESULT STRING
	//< ? xml version = "1.0" ? >
	//	-<PV>
	//		<PV>3000</PV>
	//		<PV>200000000</PV>
	//		-<PV>
	//			-<PV>
	//				<PV>0.25800</PV>
	//				<PV>10000</PV>
	//			</PV>
	//			<PV>false</PV>
	//		</PV>
	//	</PV>
	*/
	return vComplexValue;
}

DpValue function_ArrayData_Set()
{
	DpValue vArrayValue = CreateDpValue();
	DpValue vStringValue1 = CreateDpValue();
	DpValue vStringValue2 = CreateDpValue();
	DpValue vStringValue3 = CreateDpValue();

	bool bRes;

	bRes = SetDpValueString(vStringValue1, "<hihihi &hihihi \"hihihi\">");
	bRes = SetDpValueDpValue(vArrayValue, vStringValue1);
	if (!bRes) {
		printf("%s\n", vArrayValue.szErrorReason);
	}
	bRes = SetDpValueString(vStringValue2, "StringValue2");
	bRes = SetDpValueDpValue(vArrayValue, vStringValue2);
	if (!bRes) {
		printf("%s\n", vArrayValue.szErrorReason);
	}
	bRes = SetDpValueString(vStringValue3, "StringValue3");
	bRes = SetDpValueDpValue(vArrayValue, vStringValue3);
	if (!bRes) {
		printf("%s\n", vArrayValue.szErrorReason);
	}
	/* RESULT STRING
	<PV>
		<PV>&lt; hihihi &amp; hihihi &quot; hihihi&quot; &gt;</PV>
		<PV>StringValue2</PV>
		<PV>StringValue3</PV>
	</PV>
	*/
	return vArrayValue;
}

/*
commmon
DpParameter __stdcall CreateDpParameter(void);
bool __stdcall SetDpParameter(DpParameter &_edaParameter,	const char *_szSourceId,const char *_szName, DpValue &_edaValue);
bool __stdcall SetTransientDpParameter(DpParameter &_dpParameter, const char *_szSourceId, const char *_szName, const char *_szObjType, const char *_szObjID, DpValue &_dpValue);
bool __stdcall DeleteDpParameter(DpParameter &_edaParameter);
bool __stdcall SendDpParameter(DpParameter &_edaParameter);
*/
void function_SemiParameterSend_Test()
{
	DpParameter dpParameter = CreateDpParameter();
	DpValue vIntValue = CreateDpValue();
	SetDpValueInt(vIntValue, 2000000);
	if (!SetTransientDpParameter(dpParameter, "sourceId_SemiP", "parameterName_SemiP", "SEMIOBJ", "SEMIID", vIntValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return ;
	}
	if (!SendDpParameter(dpParameter)) {
		printf("%s\n", dpParameter.szErrorReason);
	}
	printf("success! function_SemiParameterSend_Test()\n");
}

void function_ParameterSend_Test1()
{
	DpParameter dpParameter = CreateDpParameter();
	DpValue vIntValue = CreateDpValue();
	DpValue vComplexValue = function_ComplexData_Set();
	
	SetDpValueInt(vIntValue, 2000000);
	if (!SetDpParameter(dpParameter, "sourceId_IP", "parameterName_IP", vIntValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return ;
	}

	if (!SetDpParameter(dpParameter, "sourceId_ComP", "parameterName_ComP", vComplexValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return ;
	}
	if (!SendDpParameter(dpParameter)) {
		printf("%s\n", dpParameter.szErrorReason);
	}
	printf("success! function_ParameterSend_Test1()\n");
}

void function_ParameterSend_Test2()
{
	DpParameter dpParameter = CreateDpParameter();
	DpValue vIntValue = CreateDpValue();
	DpValue vArrayValue = function_ArrayData_Set();

	SetDpValueInt(vIntValue, 2000000);
	if (!SetDpParameter(dpParameter, "sourceId_IP", "parameterName_IP", vIntValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}

	if (!SetDpParameter(dpParameter, "sourceId_ArrayP", "parameterName_ArrayP", vArrayValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}
	if (!SendDpParameter(dpParameter)) {
		printf("%s\n", dpParameter.szErrorReason);
	}
	printf("success! function_ParameterSend_Test2()\n");
}
//bool __stdcall TriggerDpEvent(const char *_szSourceId, const char *_szEventId, char* szErrorReason = nullptr);
void function_TriggerEvent_Test()
{
	char szError[1024];
	if (!TriggerDpEvent("TETRA/CTC", "Process_job_aborted", szError)) {
		printf("%s\n", szError);
	}
}

//bool __stdcall TriggerTransientDpEvent(const char *_szSourceId, const char *_szEventId, const char *_szObjType, const char *_szObjID, char* szErrorReason = nullptr);
void function_TriggerTransientEvent_Test()
{
	char szError[1024];
	if (!TriggerTransientDpEvent("sourceId_tEvent", "parameterName_tEvent", "SEMIOBJ", "SEMIID", szError)) {
		printf("%s\n", szError);
	}
}

//bool __stdcall TriggerDpEventParameter(const char *_szSourceId, const char *_szEventId, DpParameter &_edaParameter);
void function_TriggerEventParameter_Test()
{
	DpParameter dpParameter = CreateDpParameter();
	DpValue vSValue = CreateDpValue();
	SetDpValueString(vSValue, "TESTJobName");
	if (!SetDpParameter(dpParameter, "TETRA/CTC", "DV_ctcProcessJobID", vSValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}

	if (!TriggerDpEventParameter("TETRA/CTC","Process_job_completed", dpParameter)) {
		printf("%s\n", dpParameter.szErrorReason);
	}
}

//bool __stdcall TriggerTransientDpEventParameter(const char *_szSourceId, const char *_szEventId, const char *_szObjType, const char *_szObjID, DpParameter &_edaParameter);
void function_TriggerTransientEventParameter_Test()
{
	DpParameter dpParameter = CreateDpParameter();
	DpValue vIntValue = CreateDpValue();
	SetDpValueInt(vIntValue, 2000000);
	if (!SetTransientDpParameter(dpParameter, "TETRA/CHA", "DV_ctcA1_CarrID", "SEMIOBJ", "SEMIID", vIntValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}
	if (!TriggerTransientDpEventParameter("TETRA/CHA","CH_A_process_ended","SEMIOBJ","SEMIID", dpParameter)) {
		printf("%s\n", dpParameter.szErrorReason);
	}
}

//bool __stdcall TriggerDpException(const char *_szSourceId, const char *_szExceptionId, bool _bSet, char* szErrorReason = nullptr);
void function_TriggerException_Test()
{
	char szError[1024];
	if (!TriggerDpException("TETRA/CHC", "AL82009", true, szError)) {
		printf("%s\n", szError);
	}
}

//bool __stdcall TriggerDpExceptionParameter(const char *_szSourceId, const char *_szExceptionId, bool _bSet, DpParameter &_edaParameter);
void function_TriggerExceptionParameter_Test()
{
	DpParameter dpParameter = CreateDpParameter();
	DpValue vValue = CreateDpValue();
	SetDpValueInt(vValue, 2000000);
	if (!SetDpParameter(dpParameter, "TETRA/CHC", "CTC_chC1_WaferNo", vValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}
	SetDpValueString(vValue, "TESTCARRID");
	if (!SetDpParameter(dpParameter, "TETRA/CHC", "CTC_chC1_CarrID", vValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}
	if (!TriggerDpExceptionParameter("TETRA/CHC", "AL72000", true, dpParameter)) {
		printf("%s\n", dpParameter.szErrorReason);
	}
}

//bool __stdcall TriggerOnDpSEMIObj(const char *_szSourceId, const char *_szObjType, const char *_szObjID, char* szErrorReason = nullptr);
void function_TriggerOnDpSEMIObj_Test()
{
	char szError[1024];
	if (!TriggerOnDpSEMIObj("ProcessJob", "SEMIOBJ", "SEMIID", szError)) {
		printf("%s\n", szError);
	}
}

//bool __stdcall TriggerOffDpSEMIObj(const char *_szSourceId, const char *_szObjType, const char *_szObjID, char* szErrorReason = nullptr);
void function_TriggerOffDpSEMIObj_Test()
{
	char szError[1024];
	if (!TriggerOffDpSEMIObj("ProcessJob", "SEMIOBJ", "SEMIID", szError)) {
		printf("%s\n", szError);
	}
}

//bool __stdcall SendAdhocDpParameter(DpParameter &_edaParameter);
void function_SendAdhocDpParameter_Test()
{
	DpParameter dpParameter = CreateDpParameter();
	DpValue vStringValue = CreateDpValue();
	SetDpValueString(vStringValue, "testID");
	if (!SetDpParameter(dpParameter, "TETRA/CHA", "DV_ctcA3_PJID", vStringValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}
	SetDpValueString(vStringValue, "testID2");
	if (!SetDpParameter(dpParameter, "TETRA/CHA", "DV_ctcA3_CJID", vStringValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}
	SetDpValueString(vStringValue, "testID3");
	if (!SetDpParameter(dpParameter, "TETRA/CHA", "DV_ctcA3_ProcRcpID", vStringValue)) {
		printf("%s\n", dpParameter.szErrorReason);
		DeleteDpParameter(dpParameter);
		return;
	}
	SendAdhocDpParameter(dpParameter);
}
