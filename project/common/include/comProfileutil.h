/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

/********************************************************************
2012.11.11 by KEH
-------------------------------------------------------------
INI API
*********************************************************************/

#ifndef _PROFILEUTIL_H_
#define _PROFILEUTIL_H_



#include "types.h"

#ifndef WIN32
 #define SAVEPROFILE	SavePrivateProfile()
#else
//#include <winbase.h>
 #define SAVEPROFILE
#endif


#ifndef WIN32
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define	PROJECT_HOME_DIR	"./"


typedef	struct ConfEntry	ConfEntry;



#ifdef __cplusplus
extern "C" {
#else
typedef	enum { false, true } bool;
#endif

char	*trimString(char *);
ConfEntry *confEntryNew(const char *filename);
unsigned char *confEntryGet(ConfEntry *ce, const char *key);
char	*confEntryGetFilename(ConfEntry *ce);
bool	entryNew(ConfEntry *ce, const char *secName);
void	confEntryFree(ConfEntry *ce);
void	confEntryDestroy();
bool	currentSectionSet(ConfEntry *ce, const char *secName);
bool	confEntryPut(ConfEntry *ce, const char *key, char *val);
bool	confEntrySave(ConfEntry *ce);



const char	*settingsFilename(char *fname);
int			GetPrivateProfileInt(const char *section, const char *key, int initVal, const char *fName);
int			GetPrivateProfileIntEX(const char *section, const char *key, int initVal, const char *fName);

void		GetPrivateProfileString(const char *section, const char *key, const char * initVal, char *ret, int blen, const char *fName);
void		GetPrivateProfileStringEX(const char *section, const char *key, const char * initVal, char *ret, int blen, const char *fName);
void		PutPrivateProfileInt(const char *section, const char *key, int val, const char *fName);

void		PutPrivateProfileString(const char *section, const char *key, const char *val, const char *fName);
void		SavePrivateProfile();

#ifdef __cplusplus
}
#endif



#else  // DEF WIN32

//#define DLLEXPORT __declspec( dllexport )
#ifdef __cplusplus
extern "C" {
#endif

void  PutPrivateProfileInt(const TCHAR *section, const TCHAR *key, int val, const TCHAR *fName);
void  PutPrivateProfileString(const TCHAR *section, const TCHAR *key, const TCHAR *val, const TCHAR *fName);
void  SavePrivateProfile();
void  confEntryDestroy();

#ifdef __cplusplus
}
#endif

#endif // DEF WIN32 END



#endif	//__PROFILEUTIL_H_

