#include "EqMonitor.h"



int main(int _nArgc, char **_pszArgv)
{
    g_cEqMonitor.setVersion("Integrator version : 1.0.0");
	if(_nArgc >= 2 && strcmp(_pszArgv[1], "-v") == 0) {
		//printf("Integrator version : 2.0\n");
		printf(g_cEqMonitor.getVersion());
		printf("\n");
		return 0;
	}
	g_cEqMonitor.Init();
	g_cEqMonitor.Start();

	while (1)
	{
		sleep(60);
	}


   return 0;
}
