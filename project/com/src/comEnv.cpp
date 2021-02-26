
#include "comTypes.h"
#include "comEnv.h"
#include "comProfileutil.h"

#ifdef WIN32
#include <Windows.h>
#include <psapi.h>
#pragma comment(lib, "PSAPI.LIB")
#endif

TCHAR *g_pWorkDir = NULL;
TCHAR *g_pWorkDir2 = NULL;
TCHAR *g_pSystem = NULL;
TCHAR *g_pProcessName = NULL;
TCHAR *g_pProcessConfig = NULL;



//static char *pFileName = NULL;
int CEnv::FullName(const TCHAR *fName, TCHAR **fPath)
{
	*fPath = NULL;
	int	nLen;

	nLen = (int)_tcslen(g_pWorkDir) + (int)_tcslen(fName) + 1;
	*fPath = new (std::nothrow) TCHAR[nLen];

	if (*fPath) {
		nLen = _stprintf(*fPath, g_pWorkDir, fName);
	}
	return nLen;
}


bool CEnv::initWorkingPath()
{
	if (g_pWorkDir) return true;

	// setting working path

	TCHAR szBuf[128];
	size_t nLen = 0, nLen2=0;
	TCHAR *pPos = _tgetenv(SOLUTION_HOME);
	TCHAR *pDot;
	if (pPos) {
		nLen = _tcslen(pPos);
	}
	else {

#ifdef WIN32
		nLen = GetModuleFileNameEx(GetCurrentProcess(), NULL, szBuf, 128);
		//HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,GetCurrentProcessId());
		//DWORD BufSize;
		//if (process_handle) {
		//	if (QueryFullProcessImageName(process_handle, 0, szBuf, &BufSize)) {
		//		nLen = BufSize;
		//	}
		//	else {
		//		//gs_cLogger.DebugLog(LEVEL_ERROR, "QueryFullProcessImageName is FAIL!");
		//	}
		//	CloseHandle(process_handle);			
		//}

#else
		if (argv0) {
			if (realpath(argv0, szBuf))	nLen = strlen(szBuf);
			else gs_cLogger.DebugLog(LEVEL_ERROR, "realpath [%s]", argv0);
		}
		else {
			nLen = GetProcessFullName(szBuf);
		}
#endif
		if (nLen) {
			g_pProcessConfig = new (std::nothrow) TCHAR[nLen + 8];
			_tcscpy(g_pProcessConfig, szBuf);
			pDot = _tcsrchr(g_pProcessConfig, '.');
			if(pDot) _tcscpy(pDot, _T(".ini"));
			else _stprintf(g_pProcessConfig+nLen, _T(".ini"));

			pPos = _tcsrchr(szBuf, g_s);
			nLen = pPos - szBuf + 1;
			pPos++;
			nLen2 = _tcslen(pPos);
			g_pProcessName = new (std::nothrow) TCHAR[nLen2 + 1];
			if (g_pProcessName) _tcscpy(g_pProcessName, pPos);
			pDot = _tcsrchr(g_pProcessName, '.');
			if (pPos) *pDot = 0;
		}
	}

	if (nLen) {
		g_pWorkDir = new (std::nothrow) TCHAR[nLen];
		if (!g_pWorkDir) {
			//gs_cLogger.Log_Debug(LOG_ERROR, "new g_pWorkDir");
			return false;
		}
		TCHAR *pRear = _tcsrchr((TCHAR *)szBuf, g_s);
		nLen = pRear - szBuf + 1;
		_tcsncpy(g_pWorkDir, szBuf, nLen);
		g_pWorkDir[nLen] = 0;
		return true;
	}
	
	return false;
}


bool CEnv::setWorkingDir(TCHAR *pPullPath)
{
	if (pPullPath) {
		size_t	nLen = (int)_tcslen(pPullPath);
		if (!nLen) return false;
		if (g_pWorkDir) {
			delete[] g_pWorkDir;
		}
		g_pWorkDir = new (std::nothrow) TCHAR[nLen + 2];

		if (g_pWorkDir == NULL) return false;

		_tcscpy(g_pWorkDir, pPullPath);
	}

	return true;
}

bool CEnv::setWorkingDir2(int upCount)
{
	if (!g_pWorkDir) {
		return false;
	}
	size_t	nLen = (int)_tcslen(g_pWorkDir);
	TCHAR *pRear = g_pWorkDir;
	if (g_pWorkDir2) delete g_pWorkDir2;
	g_pWorkDir2 = new TCHAR[nLen + 1];
	_tcscpy(g_pWorkDir2, g_pWorkDir);

	int i;
	for (i = 0; i < upCount; i++) 
	{
		pRear = _tcsrchr((TCHAR *)g_pWorkDir2, g_s);
		*pRear = 0;
	}
	pRear = _tcsrchr((TCHAR *)g_pWorkDir2, g_s);
	nLen = pRear - g_pWorkDir2 + 1;

	_tcsncpy(g_pWorkDir2, g_pWorkDir, nLen); g_pWorkDir2[nLen] = 0;
	return true;
}

int CEnv::initSystem(TCHAR *pDir)
{
	if (g_pSystem) return 1;

	int nLen;
	TCHAR szConfig[128];
	if (pDir) {
		nLen = _stprintf(szConfig, _T("%ssystem.ini"), pDir);
	}
	else if (g_pWorkDir) {
		nLen = _stprintf(szConfig, _T("%ssystem.ini"), g_pWorkDir);
	}
	else {
		if (CEnv::initWorkingPath()) {
			nLen = _stprintf(szConfig, _T("%ssystem.ini"), g_pWorkDir);
		}
		else return 0;
	}
	g_pSystem = new (std::nothrow) TCHAR[nLen + 1];
	if (!g_pSystem) {
		printf("g_pSystem is NULL\n");
		return 0;
	}
	_tcscpy(g_pSystem, szConfig);

	return 1;
}







#ifndef WIN32
bool getProcessNameByPid(char *name)
{
	char szprocess[256];
	int result = true;
	int pid = getpid();

	_stprintf(szprocess, _T("/proc/%d/cmdline"), pid);

	FILE *fp = _tfopen(szprocess, "r");
	if (!fp) {
		_tprintf(_T("getProcessNameByPid fopen(%s) is FAIL\n"), szprocess);
		result = false;
	}
	else {
		size_t size = fread(name, g_nChSize, 128, fp);
		if (size>0) {
			name[size] = 0;
		}
		else result = false;
		fclose(fp);
	}
	return result;
}

int GetProcessFullName(char *_szFullName)
{
	char szFileName[1024];
	int nResult;
	int pid = getpid();
	sprintf(szFileName, "/proc/%d/exe", pid);
	if (access(szFileName, F_OK) != 0)    // check file's existance
	{
		return false;
	}

	nResult = readlink(szFileName, _szFullName, _nFullNameBufSize);
	_szFullName[nResult] = '\0';
	return nResult;
}

#endif