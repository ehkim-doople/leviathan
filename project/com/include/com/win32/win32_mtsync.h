/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

#ifndef _LIB_WIN32_MTSYNC_H
#define _LIB_WIN32_MTSYNC_H

#ifndef _INC_WINDOWS

#include <windows.h>
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#endif // _INC_WINDOWS


#include "../abstract_mtsync.h"

namespace common {

namespace win32 {

class critical_section : public common::abstract_critical_section {
	friend class condition;
  public:
	critical_section() { InitializeCriticalSection(&m_cs); }
	virtual ~critical_section() { DeleteCriticalSection(&m_cs);}

	bool enter(bool bWait=true) 
	{ 
	     BOOL	bRet = TRUE;

	     if (bWait) 
		     EnterCriticalSection(&m_cs);
	     else
    #if(_WIN32_WINNT >= 0x0400)
		     bRet = TryEnterCriticalSection(&m_cs);
    #else
		    EnterCriticalSection(&m_cs);
    #endif

	     return (bRet)?true:false;

	} 

	void leave() { LeaveCriticalSection(&m_cs);}

  private:
	CRITICAL_SECTION m_cs;
};

class condition : public common::abstract_condition {
  public:
	  condition() { m_event = CreateEvent(NULL, TRUE, FALSE, NULL);}
	  ~condition() { CloseHandle(m_event); }
	
	  void signal() { SetEvent(m_event); }
	  void signal_all() { abort(); }
	  void	clear_signal()	{ ResetEvent(m_event); }
//	  bool wait(int milliseconds, critical_section *p) {
	  bool wait(int milliseconds, critical_section *p) 
	  {
			DWORD timeout = milliseconds < 0 ? INFINITE : milliseconds;
			if(p) p->enter();
			bool rv = WaitForSingleObject(m_event, timeout) == WAIT_OBJECT_0;
			ResetEvent(m_event);
			if(p) p->leave();
			return rv;
	  }
  private:
	HANDLE m_event;
};



class win_atomic {
  public:
	win_atomic() {m_count = 0;}
	~win_atomic() {}
	inline long getLCount() { return m_count; }
	inline unsigned int getUCount() { return (unsigned int)m_count; }
	inline int getCount() { return (int)m_count; }
	inline void setCount(long nVal) { m_count = nVal; }
	inline void setCount(int nVal) { m_count = (long)nVal; }
	inline void init() { m_count = 0; }

	inline long atomic_exchange(int nExchange) { return InterlockedExchange(&m_count, nExchange);}
	inline long atomic_increment() { return InterlockedIncrement(&m_count);}
	inline long atomic_decrement() { return InterlockedDecrement(&m_count);}
	inline int atomic_compare_exchange(int nExchange, int nComperand) {return (int)InterlockedCompareExchange(&m_count, nExchange, nComperand);} // return long
	inline long atomic_compare_exchange(long nExchange, long nComperand) {return InterlockedCompareExchange(&m_count, nExchange, nComperand);} // return long
private:
    volatile long m_count;
};

class win64_atomic {
  public:
	win64_atomic() {m_count = 0;}
	~win64_atomic() {}
	inline long long getLCount() { return m_count; }
	inline unsigned long long getUCount() { return (unsigned long long)m_count; }
	inline int getCount() { return (int)m_count; }
	inline void setCount(long long nVal) { m_count = nVal; }
	inline void setCount(int nVal) { m_count = (long long)nVal; }
	inline void init() { m_count = 0; }

	inline long long atomic_exchange(long long nExchange) {return InterlockedExchange64(&m_count, nExchange);}
	inline long long atomic_increment() { return InterlockedIncrement64(&m_count);}
	inline long long atomic_decrement() { return InterlockedDecrement64(&m_count);}
	//inline int atomic_compare_exchange(int nExchange, int nComperand) {return (idatent)InterlockedCompareExchange64(&m_count, nExchange, nComperand);} // return long
	inline long long atomic_compare_exchange(long long nExchange, long long nComperand) {return InterlockedCompareExchange64(&m_count, nExchange, nComperand);} // return long
private:
    volatile long long m_count;
};

#ifdef __cplusplus
extern "C" {
#endif

void __stdcall initSpinLock(_Out_ win_atomic *pLock);
void __stdcall acquireSpinLock(_Out_ win_atomic *pLock);
void __stdcall releaseSpinLock(_Out_ win_atomic *pLock);


#ifdef __cplusplus
};
#endif

} // namespace win32

} // namespace lib

#endif // _LIB_WIN32_MTSYNC_H
