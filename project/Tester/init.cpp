#include "init.h"
#include "comTypes.h"


// Connect To EDA Server
char g_szServerIP[24];
int g_nServerPort;
int g_nConnectTimeoutMilliSec;

// for EdaLibraryFunction
char g_szNameTransmitted[32];
bool g_bSendTimeoutWatch;
bool g_bSendTimeoutwatchLog;
int g_nSendThresholdMillisec;

void init()
{
	memset(g_szServerIP, 0, sizeof(g_szServerIP));
	g_nServerPort = 0;
	g_nConnectTimeoutMilliSec = 1000;

	memset(g_szNameTransmitted, 0, sizeof(g_szNameTransmitted));
	g_bSendTimeoutWatch = false;
	g_bSendTimeoutwatchLog = false;
	g_nSendThresholdMillisec = 0;
}



// 사용자 정의 INI 파일 Read
bool initConfig_UserDefine(const char *pConfigFile)
{
	int nLen = GetPrivateProfileString(_T("EDA_SERVER"), _T("IP"), "", g_szServerIP, 32, pConfigFile);
	if (!nLen) {
		comErrorPrint("IP is empty [EDA_SERVER:IP]");
		return false;
	}
	g_nServerPort = GetPrivateProfileInt(_T("EDA_SERVER"), _T("PORT"), 0, pConfigFile);
	if (g_nServerPort < 1024 || 65535 < g_nServerPort) {
		comErrorPrint("Invalid port; Given port number is out of range-[1024, 65535]");
		return false;
	}

	g_nConnectTimeoutMilliSec = GetPrivateProfileInt(_T("EDA_SERVER"), _T("CONNECT_TIMEOUT_MILLISECONDS"), 1000, pConfigFile);

	nLen = GetPrivateProfileString(_T("EDA_LIBRARY_FUNCTION"), _T("NAME_TRANSMITTED"), "", g_szNameTransmitted, 24, pConfigFile);

	nLen = GetPrivateProfileInt(_T("EDA_LIBRARY_FUNCTION"), _T("IS_SEND_TIMEOUT_WATCH"), 0, pConfigFile);
	if (!nLen) {}
	else if (nLen == 1) {
		nLen = GetPrivateProfileInt(_T("EDA_LIBRARY_FUNCTION"), _T("SEND_THRESHOLD_MILLISECONDS"), 0, pConfigFile);
		if (!nLen || nLen < 0) {
			comErrorPrint("Invalid value; [SEND_THRESHOLD_MILLISECONDS]");
			return false;
		}
		g_bSendTimeoutWatch = true;
		g_nSendThresholdMillisec = nLen;
	}
	else {
		comErrorPrint("Invalid value; [IS_SEND_TIMEOUT_WATCH] Given value is out of range[0-1]");
		return false;
	}
	return true;
}
//##########################################
// InfiniA_Library Initialize
//##########################################
bool initConfig_forInfiniALibrary()
{
	char szConfigFile[128];

	init();

	//############################################################
	// User Define config File initialize
	//############################################################
	initConfig_UserDefine(g_pProcessConfig);



	//############################################################
	// InfiniA_Library Initialize
	//############################################################
	// g_pWorkDir : current exe directory
	sprintf(szConfigFile, "%s%s", g_pWorkDir, "InfiniA-Library.ini");

	printf("InfiniA_Library Version : %s\n", EqLib_GetVersion());

	// 1. initConnectToEDAServer
	if (!EqLib_CommInit(g_szServerIP, g_nServerPort, g_nConnectTimeoutMilliSec, false, NULL)) {
		printf(EqLib_GetCurrentError()); // error Msg
		return false;
	}
	
	//2. initLog
	if (!EqLogInitByConfig(szConfigFile)) {
		printf(EqLib_GetCurrentError()); // error Msg
		return false;
	}

	// 3. initEtc
	if (!EqLib_SetTransmitter(g_szNameTransmitted)) {
		printf(EqLib_GetCurrentError()); // error Msg
	}
	EqLib_SetStopwatch(g_bSendTimeoutWatch, NULL, g_nSendThresholdMillisec); // NULL : default system log

	return true;
}


