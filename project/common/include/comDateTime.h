/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
2020.09.13 by KEH
-------------------------------------------------------------
TimeDate Library Group
Linux partially verified
*********************************************************************/
#ifndef	__CDateTime_H__
#define	__CDateTime_H__

#include "types.h"

#ifndef WIN32
	#if	defined(__ia64)
	 #include <time.h>
	#else
	 #include <time.h>
	 #include <sys/time.h>
	#endif
#else
 #include <time.h>
 #include <sys/timeb.h>
 #include <winsock2.h>
#include <windows.h>
#include <winbase.h>


#endif

#define NANOSEC_PER_SEC		1000000000L
#define MICROSEC_PER_SEC	1000000L
#define MILLISEC_PER_SEC	1000L

#define UNIT_NANO_SECONDS	0x00000001
#define UNIT_MICRO_SECONDS	0x00000002
#define UNIT_MILLI_SECONDS	0x00000004

enum TZ_TYPE
{
	TZ_NONE = 0,
	TZ_UTC,
	TZ_KST
};

enum DT_TYPE
{
	DT_NONE=0,
	DT_SEC,
	DT_MILLISEC,
	DT_MICROSEC,
	DT_NANOSEC,
};

enum DT_FORMAT {
	LOG_DATE_NONE = 0,
	LOG_DATE_YYYYMMDDHHMMSS,
	LOG_DATE_YYYYMMDDHHMMSS3,
	LOG_DATE_YYYYMMDDHHMMSS6,
	LOG_DATE_YYYYMMDDHHMMSS9,
	LOG_DATE_YYMMDDHHMMSS,
	LOG_DATE_YYMMDDHHMMSS3,
	LOG_DATE_YYMMDDHHMMSS6,
	LOG_DATE_YYMMDDHHMMSS9,
	LOG_DATE_DDHHMMSS,
};

struct STTimeZone
{
	int nTZHour;
	int nTZMin;
	int nTZSec;
	int nTZType;
};

struct STDTime
{
	int nYear;
	int nMonth;
	int nDay;
	int nHour;
	int nMinute;
	int nSec;
	int nLast;
};

inline int LastDay_month(int year) {
	if (!(year % 4) && (year % 100) || !(year % 400)) return 29;
	return 28;
}

class Misc {
public:

	static	void m_Sleep(int iMiliSec); // milliSeconds sleep
	static	void u_Sleep(int iuSec);	// microSeconds sleep
	static inline unsigned int __div_u64_rem(unsigned long long dividend, unsigned int divisor, unsigned long long *remainder)
	{
		unsigned int ret = 0;
#ifndef WIN32
		while (dividend >= divisor) {
			//The following asm() prevents the compiler from
			//  optimising this loop into a modulo operation. 
			asm("" : "+rm"(dividend));
			dividend -= divisor;
			ret++;
		}
		*remainder = dividend;
#else
		*remainder = dividend%divisor;
		ret = (unsigned int)(dividend/divisor);
#endif
		return ret;
	}
	static inline unsigned int __div_u32_rem(unsigned long dividend, unsigned int divisor, unsigned long *remainder)
	{
		unsigned int ret = 0;
#ifndef WIN32
		while (dividend >= divisor) {
			//The following asm() prevents the compiler from
			//  optimising this loop into a modulo operation. 
			asm("" : "+rm"(dividend));
			dividend -= divisor;
			ret++;
	}
		*remainder = dividend;
#else
		*remainder = dividend%divisor;
		ret = (unsigned int)(dividend / divisor);
#endif
		return ret;
	}
	static inline void timespec_add_nanosec(struct timespec *a, unsigned long long ns)
	{
		a->tv_sec += __div_u64_rem(a->tv_nsec + ns, NANOSEC_PER_SEC, &ns);
		a->tv_nsec = (long)ns;
	}
	static inline void timespec_add_microsec(struct timespec *a, unsigned long ms)
	{
		a->tv_sec += __div_u32_rem(a->tv_nsec + ms, MICROSEC_PER_SEC, &ms);
		a->tv_nsec = ms;
	}
	static inline void timespec_add_milisec(struct timespec *a, unsigned long ms)
	{
		a->tv_sec += __div_u32_rem(a->tv_nsec + ms, MILLISEC_PER_SEC, &ms);
		a->tv_nsec = ms;
	}
	static inline void timespec_diff_nanos(struct timespec *start, struct timespec *stop, struct timespec *result)
	{
		timespec_diff(start, stop, result, NANOSEC_PER_SEC);
	}
	static inline void timespec_diff_microsec(struct timespec *start, struct timespec *stop, struct timespec *result)
	{
		timespec_diff(start, stop, result, MICROSEC_PER_SEC);
	}
	static inline void timespec_diff_millisec(struct timespec *start, struct timespec *stop, struct timespec *result)
	{
		timespec_diff(start, stop, result, MILLISEC_PER_SEC);
	}
	static void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result, long last);
	static void getmillisec(struct timespec *cur);
#ifdef WIN32
	static void getFileTime(struct timespec *cur, bool bMicro);
	static inline void getmicrosec(struct timespec *cur, long divisor) { bool bFlag = false; if (divisor == MICROSEC_PER_SEC) bFlag = true;  getFileTime(cur, bFlag); }
#else
	static inline void getmicrosec(struct timespec *cur, long divisor) {clock_gettime(CLOCK_REALTIME, cur);cur->tv_nsec = cur->tv_nsec%divisor;}

#endif
};

/*
https://support.microsoft.com/ko-kr/help/188768/info-working-with-the-filetime-structure
lpFileTime:Pointer to UTC file time to convert. , lpLocalFileTime:Pointer to converted file time.
BOOL FileTimeToLocalFileTime(CONST FILETIME *lpFileTime,  LPFILETIME lpLocalFileTime);
*/

#ifdef WIN32
typedef FILETIME	COM_FILETIME;  // windows FILETIME : UTC
#else
typedef time_t		COM_FILETIME;
#endif

class CFileTime
{
public:
	CFileTime();
	~CFileTime();
#ifdef WIN32
	int setFileTime(TCHAR *pFilePath);
	int setFileTime(FILETIME *pftWrite);
#endif
	int setFileTime(time_t *fileTime);	
	bool isCompare(CFileTime *pCompare, E_OPERATOR eOP);
	bool isCompare(unsigned int nCompare, E_OPERATOR eOP);
	inline unsigned int getYear() { return m_nYear; }
	inline unsigned int getDateTime() { return m_nDateTime; }
private:
	unsigned int m_nYear;
	unsigned int m_nDateTime;
};

#define DIVISION_SEC_TO_YEAR 

class CDateTime 
{
public:

	CDateTime(DT_TYPE nType= DT_SEC, TZ_TYPE nTz = TZ_NONE);
	~CDateTime();

	void  SetTimeValue(time_t *ltmVal);
	void  SetSecondsValue(int nSec);
	void SetCurrentTime(DT_TYPE nType = DT_SEC, TZ_TYPE nTz = TZ_NONE);

	inline int	Year()		{ return m_tm.tm_year; }
	inline int	Month()		{ return m_tm.tm_mon; }
	inline int	Mday()		{ return m_tm.tm_mday; }
	inline int	Wday()		{ return m_tm.tm_wday; }
	inline int	Hour()		{ return m_tm.tm_hour; }
	inline int	Minute()	{ return m_tm.tm_min; }
	inline int	Sec()		{ return m_tm.tm_sec; }
	//inline int	TimeValue() { return m_tmval.tv_sec; }
	inline const TCHAR *TimezoneString() { return m_szTimezone; }

	const TCHAR *DateString(char f='/');
	const TCHAR *DateString2(char f='/');
	const TCHAR *TimeString(bool f=1);
	const TCHAR *addDateString(int nAddDay, char f = 0);

	int setString(TCHAR *pTarget, TCHAR *pFormat, DT_FORMAT nFormat);
private:

	tm			m_tm;
	timespec	m_tmval;
	TCHAR		m_szDate[12], m_szTime[24], m_szTimezone[8];

	void    uptoSeconds(TZ_TYPE nTz, time_t t);
	void    uptoMilliSeconds(TZ_TYPE nTz);
	void    uptoMicroseconds(TZ_TYPE nTz, DT_TYPE nType);
};

void initTimeZone();
void getCurrentTime(STDTime *pTime);
extern STTimeZone g_stTZ;

#endif	//	__CDateTime_H__


