#include "win32_mtsync.h"

void __stdcall common::win32::initSpinLock(win_atomic *pLock)
{
	pLock->init();
}

void __stdcall common::win32::acquireSpinLock(win_atomic *pLock)
{
	int n;
	loop:
	if(!pLock->atomic_compare_exchange(1,0)) return;
			
	n = 1024;
	while(n--) YieldProcessor();
	goto loop;
}

void __stdcall common::win32::releaseSpinLock(win_atomic *pLock)
{
	pLock->init();
}

