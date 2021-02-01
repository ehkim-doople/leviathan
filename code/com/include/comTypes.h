/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/


#ifndef	__COMTYPES_H__
#define	__COMTYPES_H__



#include "types.h"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<errno.h>
#include	<time.h>
#include	<stdarg.h>
#include	<fcntl.h>
#include	<new>		// std::nothrow 를 위해 사용

typedef unsigned int TICKTIME_MILLISEC;

#ifndef WIN32			//UNIX, LINUX
 #include	<unistd.h>
 #include	<poll.h>
 #include	<sys/types.h>
 #include	<sys/socket.h>
 #include	<netinet/in.h>
 #include	<arpa/inet.h>
 #include	<pthread.h>
 #include	"com/unix/unix_mtsync.h"
 #include	"com/unix/unix_resource.h"
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>

 #define INFINITE -1
 
 #define SLEEP(sec)	sleep(sec)
 #define nvsnprintf(str, size, format, ap)	vsnprintf( &str, size, &format, ap)

 #define THREAD_RET_TYPE			void *
 #define THREAD_CALLING_CONVENTION
 #define THREAD_RETURN				return NULL
 #define THREAD_CREATE(pthid, procname, param)	(pthread_create(pthid, NULL, procname, param) == 0)
 #define THREAD_EXIT(pthid)						pthread_exit(pthid)
 #define THREAD_CANCEL(pthid)					pthread_cancel(pthid)
 #define THREAD_JOIN(pthid, status)				pthread_join(pthid, status)
 #define THREAD_SELF							pthread_self()
 #define THREAD_WAIT(pthid)		

typedef THREAD_RET_TYPE(*PTHREAD_START_ROUTINE)(void * lpThreadParameter);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

 #define INVALID_SOCKET		-1
 #define SOCKET_ERROR		-1

 #define SOCKETCLOSE(sockfd)		close(sockfd)
 #define SOCKETERROR				errno
 #define SOCKETERRORWOULDBLOCK		EWOULDBLOCK
 #define SOCKETOPTDATA

 #define PROCESS_GET		getpid()
 #define PROCESS_KILL(pid)	kill(pid, SIGKILL)
 typedef pid_t				PROCESS_ID;
 typedef int				SOCKET;

#define GETTICKCOUNT()				common::unix_na::getTickCount()

 #define FILEMOVE(src, dest)		rename(src, dest)	
 #define FILEDELETE(src)			unlink(src)

 typedef common::unix_na::critical_section	COM_CRITICAL_SECTION;
 typedef pthread_t						THREAD_ID;
#ifdef _POSIX
 typedef common::unix_na::posix_condition		THREAD_COND;
#else
 typedef common::unix_na::condition			THREAD_COND;
#endif


#define GET_HDD_USAGE(path)			common::unix_na::get_usage_percent(path)
#define GET_MEM_INFO(tot, used)		common::unix_na::get_mem_info(tot,used)
#define INIT_INTERFACE()			common::unix_na::initInterface()
#define SET_NET1()					common::unix_na::set_network_info1()
#define SET_NET2()					common::unix_na::set_network_info2()
#define GET_NET_INFO(idx, dp_Name, p_rxBytes, p_txBytes)	common::unix_na::get_network_info(idx, dp_Name, p_rxBytes, p_txBytes)
#define GET_CONNECTION_COUNT(nPort) common::unix_na::getConnectionCount(nPort)
 // DEC System에서는 socket 사용시 socklen_t가 정의가 되지 않은 경우 int로 정의한다.
  #ifdef	DEC
   typedef	int	socklen_t;
  #endif

 typedef common::unix_na::posix_atomic	atomic_nr;

#ifdef SPACK9_BELOW
 typedef pthread_mutex_t					SPIN_LOCK;
 //  be careful use this !!!
#define SPIN_LOCK_INIT(pLock)                                   pthread_mutex_init(pLock,0);
#define SPIN_LOCK_ENTER(pLock)                                  pthread_mutex_lock(pLock);       // input factor : lock address
#define SPIN_LOCK_LEAVE(pLock)                                  pthread_mutex_unlock(pLock);     // input factor : lock address
#define SPIN_LOCK_DESTROY(pLock)                                pthread_mutex_destroy(pLock);
 typedef pthread_mutex_t					FAST_LOCK;
 //  be careful use this !!!
#define FAST_LOCK_INIT(pLock)                                   pthread_mutex_init(pLock,0);
#define FAST_LOCK_ENTER(pLock)                                  pthread_mutex_lock(pLock);      // input factor : lock address
#define FAST_LOCK_LEAVE(pLock)                                  pthread_mutex_unlock(pLock);    // input factor : lock address
#define FAST_LOCK_DESTROY(pLock)                                pthread_mutex_destroy(pLock);
#else
 typedef common::unix_na::posix_atomic	SPIN_LOCK;
 //  be careful use this !!!
#define SPIN_LOCK_INIT(pLock)				common::unix_na::initSpinLock(pLock)
#define SPIN_LOCK_ENTER(pLock)				common::unix_na::acquireSpinLock(pLock)
#define SPIN_LOCK_LEAVE(pLock)				common::unix_na::releaseSpinLock(pLock)
#define SPIN_LOCK_DESTROY(pLock)                              

 typedef common::unix_na::posix_atomic	FAST_LOCK;
 //  be careful use this !!!
#define FAST_LOCK_INIT(pLock)               common::unix_na::initSpinLock(pLock)
#define FAST_LOCK_ENTER(pLock)              common::unix_na::acquireSpinLock(pLock)     // input factor : lock address
#define FAST_LOCK_LEAVE(pLock)              common::unix_na::releaseSpinLock(pLock)     // input factor : lock address
#define FAST_LOCK_DESTROY(pLock)            
#endif
	
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CURRENT_TIME GetTickCount()		// 현재 시간 
 
 unsigned int GetTickCount()
{

	 struct timeval t;
	 gettimeofday(&t, 0);
	 return ((t.tv_sec * 1000) + (t.tv_usec / 1000));
 }

static const char g_rc = '\r\n';
static const char g_s = '/';

#else // window

// windows 헤더파일 충돌에 의한 추가 -- start

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
//#include <winsock2.h>  --- 충돌에 의해 제거
//#include <process.h>	 --- 충돌에 의해 제거
// windows 헤더파일 충돌에 의한 추가 -- end

//2015-04-13 winsock2.h 재정의에 의해서 추가
//#include "../../AION_Agtsvc/AION_Agtsvc/stdafx.h"

// Windows 헤더 파일:


 #include "com/win32/win32_mtsync.h"
 
 #define SLEEP(sec)	Sleep(sec*1000)
 #define nvsnprintf(str, size, format, ap)	_vsnprintf( str, size, format, ap)

 #define THREAD_RET_TYPE			DWORD
 #define THREAD_CALLING_CONVENTION	__stdcall
 #define THREAD_RETURN				return 0
 #define THREAD_CREATE(pthid, procname, param)	((*pthid=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)procname, param, 0, NULL)) != NULL)
//#define THREAD_CREATE(pthid, procname, param)	((*pthid=_beginthreadex(NULL, 0, (LPTHREAD_START_ROUTINE)procname, param, 0, NULL)) != NULL)	
 #define THREAD_EXIT(pthid)			ExitThread(0)
//#define THREAD_EXIT(pthid)		_endthreadex(0)		
 #define THREAD_CANCEL(pthid)		TerminateThread(pthid, 0)
 #define THREAD_JOIN(pthid, status)	CloseHandle(pthid)
 #define THREAD_SELF				GetCurrentThread()
 #define THREAD_WAIT(pthid)			WaitForSingleObject(pthid, INFINITE);
 
// #define SOCKETCLOSE(sockfd)		closesocket(sockfd)
 #define SOCKETERROR				WSAGetLastError()
 #define SOCKETERRORWOULDBLOCK		WSAEWOULDBLOCK
 #define SOCKETOPTDATA				(char*)

 #define PROCESS_GET				GetCurrentProcess()
 #define PROCESS_KILL(pid)			TerminateProcess(pid, 0)
 typedef HANDLE						PROCESS_ID;

#define GETTICKCOUNT()				GetTickCount()

 #define FILEMOVE(src, dest)			MoveFile(src, dest)
 #define FILEDELETE(src)				DeleteFile(src)

 typedef common::win32::critical_section	COM_CRITICAL_SECTION;
 typedef HANDLE								THREAD_ID;		
 typedef common::win32::condition			THREAD_COND;

#ifdef __X64
 typedef common::win32::win64_atomic		atomic_nr;
#else
 typedef common::win32::win_atomic			atomic_nr;
#endif

typedef common::win32::win64_atomic		atomic64_nr;

 //  be careful use this !!!
 typedef common::win32::win_atomic 			SPIN_LOCK;
#define SPIN_LOCK_INIT(pLock)				common::win32::initSpinLock(pLock)
#define SPIN_LOCK_ENTER(pLock)				common::win32::acquireSpinLock(pLock)
#define SPIN_LOCK_LEAVE(pLock)				common::win32::releaseSpinLock(pLock)
#define SPIN_LOCK_DESTROY(pLock)			

#ifdef __RUN_XP
typedef common::win32::win_atomic 			FAST_LOCK;
#define FAST_LOCK_INIT(pLock)				common::win32::initSpinLock(pLock)
#define FAST_LOCK_ENTER(pLock)				common::win32::acquireSpinLock(pLock)
#define FAST_LOCK_LEAVE(pLock)				common::win32::releaseSpinLock(pLock)
#define FAST_LOCK_DESTROY(pLock)			
#else
typedef SRWLOCK								FAST_LOCK;
#define FAST_LOCK_INIT(pLock)				InitializeSRWLock(pLock)
#define FAST_LOCK_ENTER(pLock)				AcquireSRWLockExclusive(pLock)	// input factor : lock address		
#define FAST_LOCK_LEAVE(pLock)				ReleaseSRWLockExclusive(pLock)	// input factor : lock address	
#define FAST_LOCK_DESTROY(pLock)			// no exist
#endif


#define GET_CPU_USAGE()			common::win32::get_cpu_usage_percent()
#define CURRENT_TIME GetTickCount()		// 현재 시간 
static const TCHAR g_rc = '\n';
static const TCHAR g_s = '\\';
#endif // windows end



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OS COMMON
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum E_ALLOC_TYPE
{
	eAlloc_Type_none = 0,
	eAlloc_Type_MemPool,
	////////////////////
	eAlloc_Type_new,			// delete
	eAlloc_Type_alloc,			// free
	eAlloc_Type_BufPool,		// delMem
	eAlloc_Type_newArray,		// delete []
};
inline bool ISABLETODELETE(E_ALLOC_TYPE e) { if (eAlloc_Type_MemPool < e) return true; return false; }

enum E_SLOT_UNIT
{
	SLOT_LIST_UNIT = 10,
	SLOT_POOL_UNIT = 32,
	SLOT_POOL_UNIT2 = 64,
	SLOT_POOL_UNIT3 = 128,
	SLOT_POOL_UNIT4 = 256,
	SLOT_POOL_UNIT5 = 512,
};
#define LIST_ADD_COUNT 10
#define CORE_PLUS_POOL_COUNT 10

// For Single Thread, 
// if multithreads use this, you must do synchronized mechanism programming
typedef struct {
	int nLast;
	int nFront;
	int nMax;
	void **pList;
	inline int size() { int res = nLast - nFront; if (res < 0) return -res; return res; } // single thread (No Lock required)
	inline void pop()					{ pList[nFront] = NULL; nFront++; if(nFront==nMax) nFront=0; } // single thread (No Lock required)
	inline void push(void *pData)		{ pList[nLast] = pData; nLast++; if(nLast==nMax) nLast=0; } // multi thread (Lock required)
	inline bool isFull()				{ return (nMax == size()); }
	inline void * front()				{ return pList[nFront]; } // single thread (No Lock required)
    inline void printfInfo()           { printf("nSize[%d] nMax[%d] nFront[%d] nLast[%d]\n", size(), nMax, nFront, nLast); }
} StCOMQueue;

struct STSringBuf
{
	char *pValue;
	int nIdx;
};

struct STMemInfo
{
	char *pSource;
	char *pTarget;
	size_t aBufSize;
};

struct STStrInfo
{
	TCHAR *pSource;
	TCHAR *pTarget;
	int aBufSize;
};

static const int g_nChSize = (int)sizeof(TCHAR);
extern TCHAR *g_pWorkDir;
extern TCHAR *g_pWorkDir2;
extern TCHAR *g_pSystem;
extern TCHAR *g_pProcessName;
extern TCHAR *g_pProcessConfig;

#define STRING_SIZE(str) {(int)(_tcslen(str) + 1) * g_nChSize}
#define LEN_TO_SIZE(len) {len * g_nChSize}
#define LEN_MEM_MESSAGE				128
#define LEN_SYSTEMERROR_MESSAGE		1024
extern TCHAR g_szSystemError[LEN_SYSTEMERROR_MESSAGE];
extern TCHAR g_szMessage[LEN_MEM_MESSAGE];
inline void *c_memcpy(void *de, const void *so, int l) { char *d = (char*)de; const char *s = (char*)so; while (l--) *d++ = *s++; return de;}

// s : pointer size, l : memory total size
inline void *self_shiftR(void *de, int s, int l) { char *d = (char*)de;  while (l--) { *d = *(d - s); d--; } return (void *)d; }
inline void *self_shiftL(void *de, int s, int l) { char *d = (char*)de;  while (l--) { *d = *(d + s); d++; } return de; }
inline bool isBitSet_flag(int n, char *p) { return p[n >> 3] & (n & 7)?true:false; }  // 8bit char 배열 연산을 위해.
inline void bitClear_flag(int n, char *p) {  p[n >> 3] &= ~(n & 7); }
inline void bitSet_flag(int n, char *p) {  p[n >> 3] |= (n & 7); }

inline bool IS_DIGIT(char ch) { if (ch < '0' || ch > '9') return false; return true; }
inline bool IS_HEXA(char ch) { if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) return true; return false; }
inline bool IS_NDIGIT(char *p) { int ln, i; ln = (int)strlen(p); for (i = 0; i < ln; i++) { if (!IS_DIGIT(p[i])) return true; } return false; }
inline bool IS_NHEXA(char *p) { int ln, i; ln = (int)strlen(p); if (strncmp(p, "0x", 2)) return true; for (i = 2; i < ln; i++) { if (!IS_HEXA(p[i])) return true; } return false; }
inline bool ISINCLUDE(int i, int max) { if (0 <= i && i < max) return true; return false; }

inline void comGlobalErrorProc(char *pMsg, const char *_szFunc, const int _nLine) {
	sprintf(g_szSystemError, "ERROR! [%s:%d] %s\n", _szFunc, _nLine, pMsg);
	printf(g_szSystemError);
}
//#define comErrorPrint(msg) comGlobalErrorProc(msg)
#define comErrorPrint(msg) comGlobalErrorProc(msg, __FUNCTION__, __LINE__)





#endif	// __COMTYPES_H__
