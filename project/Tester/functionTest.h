/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2021 by Doople													*/
/*																				*/
/********************************************************************************/

#pragma once
#include "InfiniA_Library.h"

void functionTest();

// Not required in plus version
// bool __stdcall EqLib_SetTransmitter(const char *_szTransmitter);
// const char * __stdcall EqLib_GetTransmitter(void);


/*
bool __stdcall EqLogInit(const char* _szLogName,  const char *_szLogPath, enum LogType _eLogType, enum LogLevel _eLogLevel, const char *_szFilePrefix, const char *_szFileExtension, int _nSizeLimit);
void __stdcall EqLogEnable(bool _bEnable);
bool __stdcall EqLogEnableOne(bool _bEnable, const char* _szLogName);
bool __stdcall EqLogSwitchLevel(const char* _szLogName,enum LogLevel _eLogLevel);
bool __stdcall EqLog(const char* _szLogName,enum LogLevel _eLogLevel,const char *_szFormat,...);
void __stdcall EqSLog(enum LogLevel _eLogLevel, const char *_szFormat, ...);
*/
void function_LogInit_Test();

/*
void __stdcall EqLib_SetStopwatch(bool _bFact, char * _szPath, unsigned int _uThresholdmsec);
bool __stdcall EqLib_CommInit(const char *_szIpAddress, const int _nPort, const int _nTimeoutSec, bool _bLogTransmitted = false, const char* _szLogName = nullptr);
bool __stdcall EqLib_CommConnect();
bool __stdcall EqLib_CommDisconnect();
const char* __stdcall EqLib_GetCurrentError(void);
bool __stdcall EqLib_IsConnectable(void);
bool __stdcall EqLib_IsConnected(void);
*/

void function_Group1_Test();
void function_Recommended_Init();


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
bool __stdcall SetDpValueDpValue(DpValue &_edaValue, DpValue &_edaChildValue);
bool __stdcall ReleaseDpValue(DpValue &_edaValue);

*/
void function_Group2_Test();
DpValue function_ComplexData_Set();
DpValue function_ArrayData_Set();

/*
DpParameter __stdcall CreateDpParameter(void);
bool __stdcall SetDpParameter(DpParameter &_edaParameter,	const char *_szSourceId,const char *_szName, DpValue &_edaValue);
bool __stdcall SetTransientDpParameter(DpParameter &_dpParameter, const char *_szSourceId, const char *_szName, const char *_szObjType, const char *_szObjID, DpValue &_dpValue);
bool __stdcall SendDpParameter(DpParameter &_edaParameter);
bool __stdcall DeleteDpParameter(DpParameter &_edaParameter);
bool __stdcall TriggerDpEvent(const char *_szSourceId, const char *_szEventId, char* szErrorReason = nullptr);
bool __stdcall TriggerTransientDpEvent(const char *_szSourceId, const char *_szEventId, const char *_szObjType, const char *_szObjID, char* szErrorReason = nullptr);
bool __stdcall TriggerDpEventParameter(const char *_szSourceId, const char *_szEventId, DpParameter &_edaParameter );
bool __stdcall TriggerTransientDpEventParameter(const char *_szSourceId, const char *_szEventId, const char *_szObjType, const char *_szObjID, DpParameter &_edaParameter );
bool __stdcall TriggerDpException(const char *_szSourceId, const char *_szExceptionId, bool _bSet, char* szErrorReason = nullptr);
bool __stdcall TriggerDpExceptionParameter(const char *_szSourceId, const char *_szExceptionId, bool _bSet, DpParameter &_edaParameter );
bool __stdcall TriggerOnDpSEMIObj(const char *_szSourceId, const char *_szObjType, const char *_szObjID, char* szErrorReason = nullptr);
bool __stdcall TriggerOffDpSEMIObj(const char *_szSourceId, const char *_szObjType, const char *_szObjID, char* szErrorReason = nullptr);
bool __stdcall SendAdhocDpParameter(DpParameter &_edaParameter);

*/
void function_SemiParameterSend_Test();
void function_ParameterSend_Test1();
void function_ParameterSend_Test2();
void function_TriggerEvent_Test();
void function_TriggerTransientEvent_Test();
void function_TriggerEventParameter_Test();
void function_TriggerTransientEventParameter_Test();
void function_TriggerException_Test();
void function_TriggerExceptionParameter_Test();
void function_TriggerOnDpSEMIObj_Test();
void function_TriggerOffDpSEMIObj_Test();
void function_SendAdhocDpParameter_Test();
