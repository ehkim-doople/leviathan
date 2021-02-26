/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
2020.01.13 by KEH
-------------------------------------------------------------
Auto Running Path Complete API
*********************************************************************/

#ifndef	__COMENV_H__
#define	__COMENV_H__



#include "types.h"


#define PROJECT_HOME	_T("PROJECT_HOME")
#define SOLUTION_HOME	_T("SOLUTION_HOME")

class CEnv {
public :
	static int FullName(const TCHAR *, TCHAR **);
	//static void	free_FullName(TCHAR *ptr);
	static bool setWorkingDir(TCHAR *pPullPath);
	static bool setWorkingDir2(int upCount);
	static bool initWorkingPath();
	static int initSystem(TCHAR *pDir = NULL);
	static int getStrToInt(TCHAR *pData);

};

#ifndef WIN32
bool getProcessNameByPid(char *name);
int GetProcessFullName(char *_szFullName);
#endif

#endif	//__COMENV_H__

