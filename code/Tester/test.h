#pragma once

#include "comLogger.h"
#include "data.h"

extern int g_isEventSend;
extern int g_SleepMilliSec_Param;
extern int g_SleepMilliSec_Event;
extern int g_nSendParamCnt;
extern int g_IsEventSend;
extern int g_nParameterThreadCnt;
extern int g_IsLogProcTime;
extern CMetaData g_cMetaData;

bool init();
void destroy();
void mainLoop();
