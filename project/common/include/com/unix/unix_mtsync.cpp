#include "unix_mtsync.h"
#include <stdio.h>
#include <sched.h> 



common::unix_na::critical_section::critical_section()
{
	pthread_mutex_init(&m_cs, NULL);
}

common::unix_na::critical_section::~critical_section()
{
	pthread_mutex_destroy(&m_cs);
}

bool common::unix_na::critical_section::enter(bool bWait)
{
	bool bRes = true;
	int nRes = 1;
	if(bWait) {
		pthread_mutex_lock(&m_cs);
	}
	else {
		nRes = pthread_mutex_trylock(&m_cs);
		(nRes==0) ? bRes = true : bRes = false;
	}
	return bRes;
}

void common::unix_na::critical_section::leave()
{
	pthread_mutex_unlock(&m_cs);
}

common::unix_na::condition::condition()
{
	pthread_cond_init(&m_condition, NULL);
}

common::unix_na::condition::~condition()
{
	pthread_cond_destroy(&m_condition);
}

void common::unix_na::condition::signal()
{
	pthread_cond_signal(&m_condition);
}

void common::unix_na::condition::signal_all()
{
	pthread_cond_broadcast(&m_condition);
}

bool common::unix_na::condition::wait(int milliseconds, critical_section *p)
{
	
	if(INFINITE == milliseconds) {
		m_lock.enter();	
		pthread_cond_wait(&m_condition, m_lock.getMutex()); // this function blocks until the condition is signaled. It atomically releases the associated mutex lock before blocking, and atomically acquires it again before returning.
	}
	else {
		/*
		struct timespec   ts;
		struct timeval    tp;
		// Convert from timeval to timespec 
		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += milliseconds;
		*/
		struct timespec tm;
		int val1, val2;
		clock_gettime ( CLOCK_REALTIME, &tm );
		val1 = milliseconds/1000;
		val2 = milliseconds%1000;
		tm.tv_sec += val1;
		tm.tv_nsec += val2 * 1000000ul;

		m_lock.enter();	
		int bRes = pthread_cond_timedwait(&m_condition, m_lock.getMutex(), &tm);
		if(ETIMEDOUT == bRes) 
		{
			m_lock.leave();
			return false;
		}
	}
	m_lock.leave();
	return true;
}

common::unix_na::counting_semaphore::counting_semaphore()
{

}

common::unix_na::counting_semaphore::~counting_semaphore()
{

}

void common::unix_na::counting_semaphore::down()
{

}

void common::unix_na::counting_semaphore::up()
{

}

int common::unix_na::counting_semaphore::count()
{
	return 0;
}




common::unix_na::posix_condition::posix_condition()
{
	sem_init(&m_sem, 0,0);
}

common::unix_na::posix_condition::~posix_condition()
{
	sem_destroy(&m_sem);
}

void common::unix_na::posix_condition::signal()
{
	sem_post(&m_sem);
}

void common::unix_na::posix_condition::signal_all()
{
}

bool common::unix_na::posix_condition::wait(int milliseconds, critical_section *p)
{
	if(-1 == milliseconds) {
		sem_wait (&m_sem);
	}
	else 
	{
		struct timespec tm;
		int val1, val2;
		clock_gettime ( CLOCK_REALTIME, &tm );
		val1 = milliseconds/1000;
		val2 = milliseconds%1000;
		tm.tv_sec += val1;
		tm.tv_nsec += val2 * 1000ul * 1000ul;/*
#ifdef AIX
	    	if (tm.tv_nsec > 979999999) {
			tm.tv_sec += 60;
			tm.tv_nsec = (tm.tv_nsec + milliseconds*1000000) % 1000000000;
		} else {
			tm.tv_nsec += milliseconds*1000000; 
		}

#else
		tm.tv_nsec += milliseconds * 1000000;
		tm.tv_sec += tm.tv_nsec / 1000000000;
		tm.tv_nsec %= 1000000000;
#endif
*/
#ifndef SPACK9_BELOW
		sem_timedwait(&m_sem, &tm);
#endif
	}
	return true;
}

long common::unix_na::posix_atomic::atomic_exchange(int nExchange)
{
	long res;
	pthread_spin_lock(&m_spinlock);
    m_count = nExchange;
	res = m_count;
	pthread_spin_unlock(&m_spinlock);
	return res;
}

long common::unix_na::posix_atomic::atomic_increment()
{
	long res;
	pthread_spin_lock(&m_spinlock);
    m_count++;
	res = m_count;
	pthread_spin_unlock(&m_spinlock);
	return res;
}

long common::unix_na::posix_atomic::atomic_decrement()
{
	long res;
	pthread_spin_lock(&m_spinlock);
	m_count--; 
	res = m_count;
	pthread_spin_unlock(&m_spinlock);
	return res;
}

int common::unix_na::posix_atomic::atomic_compare_exchange(int nExchange, int nComperand)
{
    int nRet;
	pthread_spin_lock(&m_spinlock);
    nRet = m_count;
    if(nRet == nComperand) {
		m_count = nExchange;
    }
	pthread_spin_unlock(&m_spinlock);
    return nRet;
}

long common::unix_na::posix_atomic::atomic_compare_exchange(long nExchange, long nComperand)
{
	int nRet;
	pthread_spin_lock(&m_spinlock);
	nRet = m_count;
	if (nRet == nComperand) {
		m_count = nExchange;
	}
	pthread_spin_unlock(&m_spinlock);
	return nRet;
}

void common::unix_na::acquireSpinLock(posix_atomic *pLock)
{
	int n;
loop:
	if (!pLock->atomic_compare_exchange(1, 0)) return;

	n = 1024;
	while (n--) sched_yield();
	goto loop;
}

void common::unix_na::getTickCount()
{
	struct timeval t;
	gettimeofday(&t, 0);
	return ((t.tv_sec * 1000) + (t.tv_usec / 1000));
}