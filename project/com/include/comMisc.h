/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                   */
/*   Copyright 2012 by coms All Right Reserved                               */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

#ifndef	__MSIC_H__
#define	__MSIC_H__

#define NANOSEC_PER_SEC		1000000000L
#define MICROSEC_PER_SEC	1000000L
#define MILLISEC_PER_SEC	1000L

#define UNIT_NANO_SECONDS	0x00000001
#define UNIT_MICRO_SECONDS	0x00000002
#define UNIT_MILLI_SECONDS	0x00000004


class Misc {
public:

	static	void m_Sleep(int iMiliSec); // milliSeconds sleep
	static	void u_Sleep(int iuSec);	// microSeconds sleep

	static inline unsigned int __div_u64_rem(unsigned long long dividend, unsigned int divisor, unsigned long long *remainder)
	{
		unsigned int ret = 0;
		while (dividend >= divisor) {
#ifndef WIN32
			//The following asm() prevents the compiler from
			//  optimising this loop into a modulo operation. 
			asm("" : "+rm"(dividend));
#endif
			dividend -= divisor;
			ret++;
		}
		*remainder = dividend;
		return ret;
	}
	static inline unsigned int __div_u32_rem(unsigned long dividend, unsigned int divisor, unsigned long *remainder)
	{
		unsigned int ret = 0; // sec
		while (dividend >= divisor) {
#ifndef WIN32
			//linux/include/linux/math64.h 
			//The following asm() prevents the compiler from
			//  optimising this loop into a modulo operation. 
			asm("" : "+rm"(dividend));
#endif
			dividend -= divisor;
			ret++;
		}
		*remainder = dividend;
		return ret;
	}
	static inline void timespec_add_nanosec(struct timespec *a, unsigned long long ns)
	{
		a->tv_sec += __div_u64_rem(a->tv_nsec + ns, NANOSEC_PER_SEC, &ns);
		a->tv_nsec = ns;
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
	static void getmicrosec(struct timespec *cur);
	static void getnanosec(struct timespec *cur);
};



#endif	//__MSIC_H__

