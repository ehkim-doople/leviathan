/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
2020.05.13 by KEH
-------------------------------------------------------------
Logger API
thread safty queue use
*********************************************************************/

#ifndef	__COMLOGGER_H__
#define	__COMLOGGER_H__

#include "types.h"
#include "comQueue.h"
#include "comDateTime.h"
#include "comMemManager.h"
#include "comList.h"


#define	SND_SOCKET		0
#define	RCV_SOCKET		1
#define SND_QUEUE       2
#define	RCV_QUEUE		3

#define LEN_LOG_HEADER 64
#define MAX_LOGFILE_SIZE 600000000
enum Default_Log_Option
{
	eLDOPT_LISTMAX = 20,
	eLDOPT_ISPRINTF = 0,
	eLDOPT_STL_NO = 0,
	eLDOPT_LOGLEVEL = 3,
	eLDOPT_SIZE_LIMIT = 0,
	eLDOPT_IS_PRINT_LEVEL = 1,
	eLDOPT_LOGTYPE = 3,
	eLDOPT_TIMESTEMP_NO = 0
};
typedef void(*fp_DataProc) (void *pData, void *pObj);

// Log Level
enum E_LOG_LEVEL {
	LEVEL_NO = 0,
	LEVEL_DEBUG = 1,	// Low Level Debug
	LEVEL_TRACE,		// development Level Debug
	LEVEL_INFO,			// 중요 정보
	LEVEL_WARN,			// 경고 
	LEVEL_ERROR,		// 에러
	LEVEL_FATAL,		// 크리티컬
	LEVEL_ALARM,
};


enum E_LOG_FLAG {
	LOG_FLAG_DAYCHANGE	 = 0x00000001,
	LOG_FLAG_HOURCHANGE  = 0x00000002,
	LOG_FLAG_MINCHANGE	 = 0x00000004,
	LOG_FLAG_SECCHANGE	 = 0x00000008,
	LOG_FLAG_MILLICHANGE = 0x00000010,
	LOG_FLAG_WRITTING	 = 0x00000020,
	LOG_FLAG_MAKEFILE	 = 0x00000100,
	LOG_FLAG_DAILY		 = 0x00000200,
	LOG_FLAG_HOURLY		 = 0x00000400,
	LOG_FLAG_FILE		 = 0x00000800,
	LOG_FLAG_SIZEOVER	 = 0x00001000,
	IS_PRINT			 = 0x00002000,
	IS_PRINT_LEVEL		 = 0x00004000,
	IS_LOG_SET_COMPLETE	 = 0x00020000,
	IS_LOG_OPEN			 = 0x00040000,
	IS_LOG_ENABLE		 = 0x00080000,
	IS_LOG_ENABLE_WAIT	 = 0x00100000,
};

//ENABLE
//DISABLE
enum E_LogCmdCode
{
	eLCode_ENABLE = 1,
	eLCode_DISABLE = 2,
};


enum {
	LOG_TYPE_NONE = 0,
	LOG_TYPE_DAILY,
	LOG_TYPE_HOURLY,
	LOG_TYPE_FILE,
	LOG_TYPE_MAX
};

enum E_LOG_INIT_RES {
	LOG_INIT_SUCCESS = 1,
	LOG_INIT_NO_FILE = 0,
	LOG_INIT_CHANGED_SUCCESS=2,
	LOG_INIT_ERROR = -1
};

//typedef void(*fp_PutQueue)(CLogger *pLogger, TCHAR * pMsg);


const static TCHAR	gs_fmtStr[8][12] = {
	_T(""),
	_T("[DEBUG] "),
	_T("[TRACE] "),
	_T("[INFO] "),
	_T("[WARNING] "),
	_T("[ERROR] "),
	_T("[FATAL] "),
	_T("[ALARM] ")
};

#define MAX_LOGBUF_LEN	8192  // 8KB
#define MAX_SEQ 9999999
#define DebugLog(level, ...) __debugLog(level,__FUNCTION__, __LINE__, __VA_ARGS__)
#define PutQueue(level, ...) PutLogQueue(level, __VA_ARGS__)

struct STTodayTime
{
	int year;	// 년
	int month;
	int mday;	// 월단위
	int hour;
	int minute;
	int second;	 
	int date;
};
struct STLogInfo
{
	TCHAR *pLogDir;
	TCHAR *pLogName;
	TCHAR *pFileExtension;
	int	nLogLevel;
	int nLogType;
	int nSeq;
	int nSizeLimit;
	int nDaysExistence;
};

struct	STLogDateTimeExp
{
	char bBracketEnclosed;
	char cDateDelimeter;
	char bTimeDelimeter;
	char nYearLen;
	DT_TYPE nLastType;
	TZ_TYPE nTZType;
};
extern STTodayTime gs_today_time;
extern char g_szKeepDuratrion[8];
extern char g_szLogExtension[8];

class CTimeExpManager
{
public :
	CTimeExpManager();
	~CTimeExpManager();
	bool initConfig(TCHAR *pConfig);
	STLogDateTimeExp *getDTE(int nIdx);
private:
	CSList *m_pTimeExpList;
};

class CLogger 
{
public:

	CLogger();
	~CLogger();

	bool initDefault(TCHAR *pDir, TCHAR *pName=NULL, int nLogType=eLDOPT_LOGTYPE, int nLogLevel=eLDOPT_LOGLEVEL, int nSizeLimit=eLDOPT_SIZE_LIMIT, TCHAR *pFileExtension=g_szLogExtension);
	E_LOG_INIT_RES initConfig(TCHAR *pConfig, TCHAR *pSection);
	void OpenLogger();

	inline void setFunction(fp_DataProc pProc, void *pObj = NULL) {	m_fpDataProc = pProc; m_pObj = pObj;}
	inline void addLogSize(int nLen) {m_nLogSize += nLen;}
	inline int getLogSize() { return m_nLogSize; }
	inline int getSTLNo() {	return m_nStlNo; }
	inline void setLogFlag(E_LOG_FLAG nFlag) { m_nLogFlag |= nFlag; }
	inline void clearLogFlag(E_LOG_FLAG nFlag) { m_nLogFlag &= ~nFlag; }
	inline bool isLogFlag(E_LOG_FLAG nFlag) { return (m_nLogFlag & nFlag)?true:false; }
	inline void setLogLevel(int nLevel) { m_nLogLevel = nLevel; }
	inline void setLogSizeLimit(int nLimitSize) { m_nSizeLimit = nLimitSize; }
	int setHeader(TCHAR *pHeader, int nLevel);

	void PutLogQueue(int level, const char *_szFormat, ...); // total length under 4096
	void PutLogQueue(int level, TCHAR *pBody);
	void Log(int nLevel, const char *body, const char *header = NULL, const char *tail = NULL, bool bToFile=false); // total length upper 4096
	void LogPrint(TCHAR *pLog);
	void LogPrint(int level, const TCHAR *fmt, ...);
	void WritePacket(int nLevel, int nKind, int nSocket, TCHAR *szBuffer, int nPacketSize);
	void disable();

	inline TCHAR * pop() { return (TCHAR *)m_sLogQueue.pop(); } // write 하는 함수는 STLogger : 

	void checkChangeProc(int nFlag);
	void __debugLog(int level, const char *_szFunc, const int _nLine, const char *_szFormat, ...);

	inline TCHAR *getLogName() { return m_pLogName;}
	int getString(TCHAR *pDest);

	inline void setCondition(THREAD_COND *p) { m_pCondition = p; }
	void putQueue(TCHAR *p) { if (m_sLogQueue.push(p)) m_pCondition->signal(); }
private:
	CQueueS	m_sLogQueue;
	fp_DataProc m_fpDataProc;
	THREAD_COND *m_pCondition;
	void *	m_pObj;	// 함수포인터에 넘길 객체 등록
	int		m_nLogLevel;
	int 	m_nDateTimeFormat;
	STLogDateTimeExp *m_pDTE;
	FILE	*m_hFile;
	TCHAR	*m_pFName;
	TCHAR	*m_pLogDir;
	TCHAR	*m_pLogName;
	TCHAR	m_szFileExtension[12];
	int		m_nLogFlag;
	int		m_nSeq;
	int		m_nStlNo;
	unsigned int m_nSizeLimit;
	unsigned int m_nLogSize;
	TCHAR	m_szHeaderFormat[64];
	time_t	m_tDeleteGoal;
	int m_nDeleteInterval;
	//COM_CRITICAL_SECTION m_cLock;

	void _initCom(TCHAR *pDir, TCHAR *pName, TCHAR *pFileExtension = g_szLogExtension, TCHAR *pKeepDuration = g_szKeepDuratrion);
	bool _setHeaderFormat(int nTimeStempNo);
	void _setLoggerBasic(int nLogType, int nPrintLevel, int nSizeLimit);
	bool _setLoggerQueue(int nMax);
	void setLogName(TCHAR *pTarget);
	void setDateTimeFormat(STLogDateTimeExp *pLogDTE);
	int GetKeepInfo(const char *pValue);
	void checkDeleteProc();
	void __openLogger(const TCHAR *pFileName, int nSeq = 0);	
};

extern CLogger gs_cLogger;
extern void changeToAbsolutePath(TCHAR *ret, const TCHAR *path);
extern void changeToAbsolutePathFile(TCHAR *ret, const TCHAR *path);
extern int	GetProfilePath(const TCHAR *section, const TCHAR *key, const TCHAR * initVal, TCHAR *ret, const TCHAR *fName);
extern void	detachFullPath(const TCHAR *fullpath, TCHAR *dir, TCHAR *name);
extern CTimeExpManager gs_DTEManager;


inline void __comPutError(const TCHAR *pMsg, TCHAR *pResMsg, const char *pFunc, int nLine) {
	int nLen = sprintf(pResMsg, "%s:%d [%s]", pFunc, nLine, pMsg);
	if (gs_cLogger.isLogFlag(IS_LOG_ENABLE)) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, pResMsg);
		pResMsg[nLen++] = '\n';
		pResMsg[nLen] = 0;
	}
	//else {
	//	pResMsg[nLen++] = '\n';
	//	pResMsg[nLen] = 0;
	//	_tprintf(pResMsg);
	//}
}

inline void __comPutError2(const TCHAR *pMsg, TCHAR *pResMsg, const char *pFunc, int nLine) {
	sprintf(pResMsg, "%s:%d [%s]", pFunc, nLine, pMsg);
	gs_cLogger.PutLogQueue(LEVEL_ERROR, pResMsg);
}

#define comPutError(pMsg, pRes) __comPutError(pMsg, pRes, __FUNCTION__, __LINE__)
#define comPutError2(pMsg, pRes) __comPutError2(pMsg, pRes, __FUNCTION__, __LINE__)

#endif	//	__COMLOGGER_H__



