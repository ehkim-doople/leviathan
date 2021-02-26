/* This file is part of Ambulant Player, www.ambulantplayer.org.
 * 
 * Copyright (C) 2003 Stiching CWI, 
 * Kruislaan 413, 1098 SJ Amsterdam, The Netherlands.
 * 
 * Ambulant Player is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Ambulant Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Ambulant Player; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * In addition, as a special exception, if you link Ambulant Player with
 * other files to produce an executable, this library does not by itself
 * cause the resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other reason why
 * the executable file might be covered by the GNU General Public License.
 * 
 * As a special exception, the copyright holders of Ambulant Player give
 * you permission to link Ambulant Player with independent modules that
 * communicate with Ambulant Player solely through the region and renderer
 * interfaces, regardless of the license terms of these independent
 * modules, and to copy and distribute the resulting combined work under
 * terms of your choice, provided that every copy of the combined work is
 * accompanied by a complete copy of the source code of Ambulant Player
 * (the version of Ambulant Player used to produce the combined work),
 * being distributed under the terms of the GNU General Public License plus
 * this exception.  An independent module is a module which is not derived
 * from or based on Ambulant Player.
 * 
 * Note that people who make modified versions of Ambulant Player are not
 * obligated to grant this special exception for their modified versions;
 * it is their choice whether to do so.  The GNU General Public License
 * gives permission to release a modified version without this exception;
 * this exception also makes it possible to release a modified version
 * which carries forward this exception. 
 * 
 */

/* 
 * @$Id: unix_mtsync.h,v 1.11 2005/04/04 14:13:47 jackjansen Exp $ 
 */

#ifndef _LIB_UNIX_MTSYNC_H
#define _LIB_UNIX_MTSYNC_H


#include <pthread.h>
#include <errno.h>
#include "../abstract_mtsync.h"
#include <semaphore.h>

#define INFINITE            0xFFFFFFFF  // Infinite timeout


namespace common {
/*#undef unix*/
namespace unix_na {

class critical_section : public common::abstract_critical_section {
	friend class condition;
  public:
	critical_section();
	virtual ~critical_section();
	
	inline pthread_mutex_t * getMutex() { return &m_cs; }

	bool enter(bool bWait=true);
	void leave();

  private:
	pthread_mutex_t m_cs; 
};

class condition : public common::abstract_condition {
  public:
	condition();
	virtual ~condition();
	
	void signal();		// 시그널 활성화
	void signal_all();	// 쓰레드 종료
	bool wait(int milliseconds, critical_section *p=0);
	//bool wait(int milliseconds);
  private:
	pthread_cond_t m_condition;
	critical_section m_lock;
};

class posix_condition {
  public:
	posix_condition();
	~posix_condition();
	
	void signal();		// 시그널 활성화
	void signal_all();	// 쓰레드 종료
	bool wait(int milliseconds, critical_section *p=0);
	//bool wait(int milliseconds);
  private:
	sem_t m_sem; 
};

class counting_semaphore {
  public:
	counting_semaphore();
	~counting_semaphore();
	
	void down();
	void up();
	int count();
  private:
    critical_section m_lock;
    critical_section m_wait;
    int m_count;
};

class posix_atomic {
  public:
	posix_atomic() { pthread_spin_init(&m_spinlock, 0); m_count = 0;}
	~posix_atomic() { pthread_spin_destroy(&m_spinlock);}
	inline long getLCount() { return m_count; }
	inline int getCount() { return (int)m_count; } 
	inline void setCount(int nVal) { m_count = nVal; }
	inline void init() { pthread_spin_init(&m_spinlock); m_count = 0; }


	long atomic_exchange(int nExchange);
	long atomic_increment();
	long atomic_decrement();
	int atomic_compare_exchange(int nExchange, int nComperand);
	long atomic_compare_exchange(long nExchange, long nComperand);
private:
	pthread_spinlock_t m_spinlock;
    long m_count;
};


unsigned int getTickCount();

} // namespace unix

} // namespace common

#endif // _LIB_UNIX_MTSYNC_H

