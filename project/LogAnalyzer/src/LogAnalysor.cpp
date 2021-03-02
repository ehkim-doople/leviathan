// LogAnalyzer.cpp : Defines the entry point for the console application.
//

#include "DirectoryProc.h"
#include "handler.h"
#include "global.h"


int main()
{
	getCurrentTime(&g_stProcStartTime);

	g_pHandle = new CHandler();
	if (g_pHandle->init()) {
		g_pDProc = new CDirectoryProc();
		if (g_pDProc) {
			g_pDProc->init();
			g_pDProc->proc(g_stConfig.pSourceDirectory);
		}
	}

	if (g_pHandle) {delete g_pHandle; g_pHandle = NULL;}
	if (g_pDProc) {	delete g_pDProc; g_pDProc = NULL;}
    return 0;
}

