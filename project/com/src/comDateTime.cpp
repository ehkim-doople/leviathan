#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	"comDateTime.h"

#include <windows.h>
#include <winbase.h>
#ifdef  _WIN64
//#include "timezoneapi.h"
#endif
static STTimeZone g_stTZ;
static int g_nTimeSize = sizeof(time_t);
/*
UTC : =GMT , 
KST : GMT+9:00

ex) UTC : 00 , KST : 09

unix utc time
time(), gmtime(), mtkime()
localtime : localtime(), 
*/
void initTimeZone()
{
#ifndef WIN32
	//struct timespec tp;
	//clock_gettime(CLOCK_REALTIME, &tp); 
	//m_tmval.tv_sec = tp.tv_sec;
	//m_tmval.tv_usec = tp.tv_nsec/1000000L;
	//ltm = ::localtime((const time_t*)&m_tmval.tv_sec);
	//struct timezone tz;
	//gettimeofday(&m_tmval, &tz);
	//m_tm = *localtime((const time_t*)&m_tmval.tv_sec);
	//gettimeofday(&m_tmval, NULL);
	//ltm = ::localtime((const time_t*)&m_tmval.tv_sec);
#else
	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation(&tzi);
	g_stTZ.nTZHour = (-tzi.Bias) / 60;
	g_stTZ.nTZMin = (-tzi.Bias) % 60;
	g_stTZ.nTZSec = (tzi.Bias) * 60;
	g_stTZ.nTZType = 0;
#endif
}

CFileTime::CFileTime()
{
}

int CFileTime::setFileTime(TCHAR *pFilePath)
{
	struct	tm	m_tm, *ltm = &m_tm;
	TCHAR szDate[16];
	FILETIME ftWrite;
	HANDLE hFile = CreateFile(pFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return -1;
	}

	if (!GetFileTime(hFile, NULL, NULL, &ftWrite)) {
		return -2;
	}
	CloseHandle(hFile);

	//SYSTEMTIME	sysTm, localTime;
	SYSTEMTIME	sysTm;
	FileTimeToSystemTime(&ftWrite, &sysTm);  // UTC SystemTime
	//SystemTimeToTzSpecificLocalTime(NULL, &sysTm, &localTime);	// KTC LocalTime

	_stprintf(szDate, "%02d%02d%02d%02d%02d", sysTm.wMonth, sysTm.wDay, sysTm.wHour, sysTm.wMinute, sysTm.wSecond);
	m_nDateTime = atoi(szDate);
	_stprintf(szDate, "%04d", sysTm.wYear);
	m_nYear = atoi(szDate);
	return m_nDateTime;
}

int CFileTime::setFileTime(FILETIME *pftWrite)
{
	TCHAR szDate[16];

	SYSTEMTIME	sysTm;
	FileTimeToSystemTime(pftWrite, &sysTm);
	_stprintf(szDate, "%02d%02d%02d%02d%02d", sysTm.wMonth, sysTm.wDay, sysTm.wHour, sysTm.wMinute, sysTm.wSecond);
	m_nDateTime = atoi(szDate);
	_stprintf(szDate, "%04d", sysTm.wYear);
	m_nYear = atoi(szDate);
	return m_nDateTime;
}

bool setLocalFileTime(time_t *fileTime, int *pYear, int *pDateTime)
{
	struct	tm	tmTime, *ltm = &tmTime;
	TCHAR szDate[16];
#ifndef WIN32
	ltm = localtime(&fileTime);
#else
	errno_t err = localtime_s(ltm, fileTime);
	if (err) { _tprintf(_T("ERROR _localtime_s\n")); return false; }
#endif
	tmTime.tm_year += 1900;
	tmTime.tm_mon += 1;

	_stprintf(szDate, "%02d%02d%02d%02d%02d", tmTime.tm_mon, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec);
	*pDateTime = atoi(szDate);
	_stprintf(szDate, "%04d", tmTime.tm_year);
	*pYear = atoi(szDate);

	return true;
}

int CFileTime::setFileTime(time_t *fileTime)
{
	struct	tm *gm_timeInfo, tmTime;
	TCHAR szDate[16];
#ifndef WIN32
	gm_timeInfo = gmtime_r(&fileTime, gm_timeInfo);
#else
	gm_timeInfo = gmtime(fileTime);
	memcpy(&tmTime, gm_timeInfo, sizeof(tmTime));
#endif
	tmTime.tm_year += 1900;
	tmTime.tm_mon += 1;

	//printf("gm time and date : %s\n", asctime(gm_timeInfo));
	_stprintf(szDate, "%02d%02d%02d%02d%02d", tmTime.tm_mon, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec);
	m_nDateTime = atoi(szDate);
	_stprintf(szDate, "%04d", tmTime.tm_year);
	m_nYear = atoi(szDate);

	return m_nDateTime;
}

CFileTime::~CFileTime()
{
}


bool CFileTime::isCompare(CFileTime *pCompare, E_OPERATOR eOP)
{
	switch (eOP)
	{
	case eOperator_EQ: if (pCompare->getYear() == m_nYear && pCompare->getDateTime() == m_nDateTime) return true; return false;
	case eOperator_NE: if (pCompare->getYear() != m_nYear && pCompare->getDateTime() != m_nDateTime) return true; return false;
	case eOperator_GT:
		if (pCompare->getYear() < m_nYear) return true;
		if (pCompare->getYear() > m_nYear) return false;
		if (pCompare->getDateTime() < m_nDateTime) return true; return false;
	case eOperator_LT:
		if (pCompare->getYear() > m_nYear) return true;
		if (pCompare->getYear() < m_nYear) return false;
		if (pCompare->getDateTime() > m_nDateTime) return true; return false;
	case eOperator_GE:
		if (pCompare->getYear() < m_nYear) return true;
		if (pCompare->getYear() > m_nYear) return false;
		if (pCompare->getDateTime() <= m_nDateTime) return true; return false;
	case eOperator_LE:
		if (pCompare->getYear() > m_nYear) return true;
		if (pCompare->getYear() < m_nYear) return false;
		if (pCompare->getDateTime() >= m_nDateTime) return true; return false;
	}
	return false;
}

bool CFileTime::isCompare(unsigned int nCompare, E_OPERATOR eOP)
{
	switch (eOP)
	{
	case eOperator_EQ: if (nCompare == m_nDateTime) return true; return false;
	case eOperator_NE: if (nCompare != m_nDateTime) return true; return false;
	case eOperator_GT: if (nCompare < m_nDateTime) return true; return false;
	case eOperator_LT: if (nCompare > m_nDateTime) return true; return false;
	case eOperator_GE: if (nCompare <= m_nDateTime) return true; return false;
	case eOperator_LE: if (nCompare >= m_nDateTime) return true; return false;
	}
	return false;
}

//##############################################################

CDateTime::CDateTime(DT_TYPE nType, TZ_TYPE nTz)
{
	if (nType == DT_NONE) return;
	SetCurrentTime(nType, nTz);
}

void CDateTime::SetCurrentTime(DT_TYPE nType, TZ_TYPE nTz)
{
	struct	tm	*ltm = &m_tm;
	m_szTimezone[0] = 0;

	if (nTz) {
		if (nTz == TZ_UTC) {
			_tcscpy(m_szTimezone, _T("+00:00"));
		}
		else if (nTz == TZ_KST) {
			_tcscpy(m_szTimezone, _T("+09:00"));
		}
	}

	switch (nType) {
	case DT_SEC: { time_t t = time(NULL);  uptoSeconds(nTz, t);	break; }
	case DT_MILLISEC: uptoMilliSeconds(nTz);	break;
	case DT_MICROSEC: case DT_NANOSEC: uptoMicroseconds(nTz, nType); break;
	}
}

void CDateTime::uptoSeconds(TZ_TYPE nTz, time_t t)
{
	struct	tm	*ltm;
	if (nTz == TZ_UTC) {
#ifndef WIN32
		ltm = gmtime_r(&t, ltm);
#else
		ltm = gmtime(&t);
#endif
		memcpy(&m_tm, ltm, sizeof(m_tm));
	}
	else {//if (nTz == TZ_KST) {
		ltm = &m_tm;
#ifndef WIN32
		ltm = localtime(&t);
#else
		errno_t err = localtime_s(ltm, &t);
		if (err) _tprintf(_T("ERROR _localtime_s\n"));
#endif
	}
	m_tm.tm_year += 1900;
	m_tm.tm_mon += 1;
	m_tmval.tv_sec = t;
	m_tmval.tv_nsec = 0;
}


void CDateTime::uptoMilliSeconds(TZ_TYPE nTz)
{
	// only UTC
	struct	tm	*ltm = &m_tm;
#ifndef WIN32
	clock_gettime(CLOCK_REALTIME, &m_tmval);
	uptoSeconds(nTz, m_tmval.tv_sec);
	m_tmval.tv_nsec = m_tmval.tv_nsec%MILLISEC_PER_SEC;
#else
	//GetSystemTime(&sysTm);
//	time_t t = time(NULL);
	SYSTEMTIME	sysTm;
	if (nTz == TZ_UTC) {
		GetSystemTime(&sysTm);
	}
	else {//if (nTz == TZ_KST)
		GetLocalTime(&sysTm);
	}
	m_tm.tm_year = sysTm.wYear;
	m_tm.tm_mon = sysTm.wMonth;
	m_tm.tm_mday = sysTm.wDay;
	m_tm.tm_wday = sysTm.wDayOfWeek;
	m_tm.tm_hour = sysTm.wHour;
	m_tm.tm_min = sysTm.wMinute;
	m_tm.tm_sec = sysTm.wSecond;
	//uptoSeconds(nTz, t);
	m_tmval.tv_sec = 0;

	m_tmval.tv_nsec = sysTm.wMilliseconds;
#endif
}


void CDateTime::uptoMicroseconds(TZ_TYPE nTz, DT_TYPE nType)
{
	struct	tm	*ltm = &m_tm;
	//long divisor;

#ifndef WIN32
	clock_gettime(CLOCK_REALTIME, &m_tmval);
	uptoSeconds(nTz, m_tmval.tv_sec);
	m_tmval.tv_nsec = m_tmval.tv_nsec%divisor;
	if(nType == DT_MICROSEC) m_tmval.tv_nsec = m_tmval.tv_nsec%MICROSEC_PER_SEC;
#else
	Misc::getFileTime(&m_tmval, nType == DT_MICROSEC?true:false);
	uptoSeconds(nTz, m_tmval.tv_sec);
#endif
	//_stprintf(szBuf, _T("%04d/%02d/%02d %02d:%02d:%02d , %d\n"), m_tm.tm_year, m_tm.tm_mon, m_tm.tm_wday, m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec, m_tmval.tv_usec);
	//_tprintf(szBuf);
}

int CDateTime::setString(TCHAR *pTarget, TCHAR *pFormat, DT_FORMAT nFormat)
{
	int nLen=0;
	switch (nFormat)
	{
	case LOG_DATE_DDHHMMSS :
		nLen = _stprintf(pTarget, pFormat, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec);
		break;
	case LOG_DATE_YYMMDDHHMMSS:
	case LOG_DATE_YYMMDDHHMMSS3:
	case LOG_DATE_YYMMDDHHMMSS6:
	case LOG_DATE_YYMMDDHHMMSS9:
		{
			TCHAR pyear[6];
			_stprintf(pyear, _T("%d"), m_tm.tm_year);
			if (nFormat == LOG_DATE_YYMMDDHHMMSS) {
				nLen = _stprintf(pTarget, pFormat, pyear+2, m_tm.tm_mon, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec);
			}
			else nLen = _stprintf(pTarget, pFormat, pyear+2, m_tm.tm_mon, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec, m_tmval.tv_nsec);
		}
		break;
	case LOG_DATE_YYYYMMDDHHMMSS:
		nLen = _stprintf(pTarget, pFormat, m_tm.tm_year, m_tm.tm_mon, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec);
		break;
	case LOG_DATE_YYYYMMDDHHMMSS3:
	case LOG_DATE_YYYYMMDDHHMMSS6:
	case LOG_DATE_YYYYMMDDHHMMSS9:
	default:
		nLen = _stprintf(pTarget, pFormat, m_tm.tm_year, m_tm.tm_mon, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec, m_tmval.tv_nsec);
		break;
	}
	return nLen;
}

CDateTime::~CDateTime()
{
    
}


void CDateTime::SetTimeValue(time_t *ltmVal)
{
	uptoSeconds(TZ_NONE, *ltmVal);
}

void  CDateTime::SetSecondsValue(int nSec)
{
	m_tm.tm_year = 0;
	m_tm.tm_mon = 0;
	m_tm.tm_mday = nSec/86400; //(24 * 3600)
	nSec = nSec % 86400;
	m_tm.tm_hour = nSec/3600;
	nSec = nSec % 3600;
	m_tm.tm_min = nSec / 60;
	nSec = nSec % 60;
	m_tm.tm_sec = nSec;
}

const TCHAR *CDateTime::DateString(char f)
{
	if (f) {
		_stprintf(m_szDate, "%04d%c%02d%c%02d", m_tm.tm_year, f, m_tm.tm_mon, f, m_tm.tm_mday);
	}
	else {
		_stprintf(m_szDate, "%04d%02d%02d", m_tm.tm_year, m_tm.tm_mon, m_tm.tm_mday);
	}
	return m_szDate;
}


const TCHAR *CDateTime::DateString2(char f)
{
	char pyear[6];
	sprintf(pyear, "%d", m_tm.tm_year);
	sprintf(pyear, "%s", pyear + 2);

	if(f)_stprintf(m_szDate, "%s%c%02d%c%02d", pyear, f, m_tm.tm_mon, f, m_tm.tm_mday);
	else _stprintf(m_szDate, "%0s%02d%02d", pyear, m_tm.tm_mon, m_tm.tm_mday);
	return m_szDate;
}

const TCHAR *CDateTime::TimeString(bool f)
{
	if (f) {
		_stprintf(m_szTime, "%02d:%02d:%02d", m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec);
	}
	else {
		_stprintf(m_szTime, "%02d%02d%02d", m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec);
	}
	return m_szTime;

}

const TCHAR *CDateTime::addDateString(int nAddDay, char f)
{
	time_t timer = time(NULL) + (nAddDay * 24 * 60 * 60);
	struct tm t;

	errno_t err = localtime_s(&t, &timer);
	if (err) _tprintf(_T("%s %d ERROR _localtime_s\n"), __FUNCTION__, __LINE__);

	if (f) {
		_stprintf(m_szDate, "%04d%c%02d%c%02d", t.tm_year + 1900, f, t.tm_mon+1, f, t.tm_mday);
	}
	else {
		_stprintf(m_szDate, "%04d%02d%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
	}
	return m_szDate;
}


//#############################################################################
void Misc::m_Sleep(int iMiliSec)
{
#ifndef WIN32
	usleep(iMiliSec * 1000);
#else
	Sleep(iMiliSec);
#endif
}

void Misc::u_Sleep(int iuSec)
{
#ifndef WIN32
	usleep(iuSec);
#else
	Sleep(iuSec/1000);
#endif
}

void  Misc::timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result, long unit)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + unit;
	}
	else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}
}


void Misc::getmillisec(struct timespec *cur)
{
#ifndef WIN32
	clock_gettime(CLOCK_REALTIME, cur);
	cur->tv_nsec = cur->tv_nsec%MILLISEC_PER_SEC;
#else
	SYSTEMTIME	sysTm;
	GetSystemTime(&sysTm);
	cur->tv_sec = time(NULL);
	cur->tv_nsec = sysTm.wMilliseconds;
#endif
}


#ifdef WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
void Misc::getFileTime(struct timespec *cur, bool bMicro)
{
	FILETIME ft;
	union
	{
		FILETIME asFileTime;
		u_int64 asInt64;
	} myFileTime;
	GetSystemTimeAsFileTime(&ft);
	myFileTime.asFileTime = ft;
	//unsigned long long tt = ft.dwHighDateTime;
	//tt <<= 32;
	//tt |= ft.dwLowDateTime;
	myFileTime.asInt64 -= DELTA_EPOCH_IN_MICROSECS;
	cur->tv_sec = time(NULL);
	if (bMicro) {
		myFileTime.asInt64 /= 10; // microtime
		cur->tv_nsec = myFileTime.asInt64%MICROSEC_PER_SEC;
	}
	else cur->tv_nsec = myFileTime.asInt64%10000000L; //(100 nano)
}
#endif

//#####################################################################
void getCurrentTime(STDTime *pTime)
{
#ifndef WIN32
	struct	tm	ltm;
	ltm = &_tm;
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	pTime->nLast = tp.tv_nsec % MILLISEC_PER_SEC;
	//time_t t = time(NULL);
	errno_t err = localtime_s(&ltm, &tp.tv_sec);
	pTime->nYear = ltm.tm_year + 1900;
	pTime->nMonth = ltm.tm_mon +1;
	pTime->nDay = ltm.tm_mday;
	pTime->nHour = ltm.tm_hour;
	pTime->nMinute = ltm.tm_min;
	pTime->nSec = ltm.tm_sec;
#else
	SYSTEMTIME	sysTm;
	GetSystemTime(&sysTm);
	pTime->nYear = sysTm.wYear;
	pTime->nMonth = sysTm.wMonth;
	pTime->nDay = sysTm.wDay;
	pTime->nHour = sysTm.wHour;
	pTime->nMinute = sysTm.wMinute;
	pTime->nSec = sysTm.wSecond;
	pTime->nLast = sysTm.wMilliseconds;
#endif
}

