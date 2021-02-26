#include <stdio.h>
#include <string.h>
#ifndef WIN32
 #include <sys/time.h>
#include "unistd.h"
#else
 #include <time.h>
 #include <sys/timeb.h>
 #include <winsock2.h>
 #include <WinBase.h>
#endif

#include   "comMisc.h"


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
	unsigned long res;
	struct  timeval timedelay;
	timedelay.tv_sec = __div_u32_rem(iuSec, MICROSEC_PER_SEC, &res);
	timedelay.tv_usec = res;
	select(0,(fd_set*)0,(fd_set*)0,(fd_set*)0,&timedelay);
	//WINDOW 에서 10ms 이하는 정확하게 보장할수 없음
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
	unsigned long res;
	clock_gettime(CLOCK_REALTIME, cur);
	__div_u32_rem(cur->tv_nsec, MILLISEC_PER_SEC, &res);
	cur->tv_nsec = res;
#else
	SYSTEMTIME	sysTm;
	GetSystemTime(&sysTm);
	cur->tv_sec = time(NULL);
	cur->tv_nsec = sysTm.wMilliseconds;
	printf("getmillisec : %lld, %ld (milli seconds)\n", cur->tv_sec, cur->tv_nsec);
#endif
}


#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
void Misc::getmicrosec(struct timespec *cur)
{
#ifndef WIN32
	unsigned long res;
	clock_gettime(CLOCK_REALTIME, cur);
	__div_u32_rem(cur->tv_nsec, MICROSEC_PER_SEC, &res);
	cur->tv_nsec = res;
#else
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	unsigned long long tt = ft.dwHighDateTime, res;
	tt <<= 32;
	tt |= ft.dwLowDateTime;
	tt -= DELTA_EPOCH_IN_MICROSECS;
	//tt = 100nano
	tt /= 10; // 100 nano -> 1 micro
	cur->tv_sec = __div_u64_rem(tt, MICROSEC_PER_SEC, &res);
	cur->tv_nsec = res;
#endif
	printf("getmicrosec : %lld, %ld (micro seconds)\n", cur->tv_sec, cur->tv_nsec);
}

void Misc::getnanosec(struct timespec *cur)
{
#ifndef WIN32
	unsigned long res;
	clock_gettime(CLOCK_REALTIME, cur);
	__div_u32_rem(cur->tv_nsec, NANOSEC_PER_SEC, &res);
	cur->tv_nsec = res;
#else
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	unsigned long long tt = ft.dwHighDateTime, res;
	tt <<= 32;
	tt |= ft.dwLowDateTime;
	tt -= DELTA_EPOCH_IN_MICROSECS;
	//tt = 100nano
	tt *= 100; // 100 nano -> 1 nano
	cur->tv_sec = __div_u64_rem(tt, NANOSEC_PER_SEC, &res);
	cur->tv_nsec = res;
#endif
	printf("getnanosec : %lld, %ld (nano seconds)\n", cur->tv_sec, cur->tv_nsec);
}