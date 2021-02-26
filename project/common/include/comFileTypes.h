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
File API Define
/******************************************************************************/



#ifndef	__COMFILETYPES_H__
#define	__COMFILETYPES_H__



#include "types.h"


#include	<stdio.h>

#ifdef WIN32
#include <winsock2.h>
#include <Share.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

// To window warning disable,  use define: _CRT_NON_CONFORMING_SWPRINTFS;_CRT_SECURE_NO_WARNINGS;
	
#ifdef UNICODE

#define _tcscasecmp(str1, str2)				_tcsicmp(str1, str2)

// File
typedef HANDLE  JPDIR;
typedef WIN32_FIND_DATA JFILEDATA;

inline bool com_mkdir(const TCHAR *path)	{int res = CreateDirectory(path, NULL); return (res)?true:false; }
inline bool com_open(FILE *fp, const TCHAR *pPath, const TCHAR *pMode)	{fp = _tfopen(pPath, pMode); return (!fp)?false:true; }
#define com_dirclose(dir)  FindClose(dir)
inline bool com_isFile(TCHAR *file)			{ int res = GetFileAttributes(file); return (res==-1)?false:true; }
inline JPDIR com_findFirstFile(const TCHAR *pDirName, JFILEDATA *pFileData) { return FindFirstFile(pDirName, pFileData); }
inline bool com_nextFile(JPDIR dir, JFILEDATA *pFileData) { return (FindNextFile(dir, pFileData))?true:false; }
inline void com_getFileName(TCHAR **pFileName, JFILEDATA *pFileData) { *pFileName = pFileData->cFileName;}

inline bool jISREG(JFILEDATA *pFD)		{return (pFD->dwFileAttributes & FILE_ATTRIBUTE_NORMAL)?true:false;} //regular file?
inline bool jISDIR(JFILEDATA *pFD)		{return (pFD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)?true:false;} //directory?

#define com_ISREG(m)		jISREG(m) //regular file?
#define com_ISDIR(m)		jISDIR(m) //directory?
#define com_rmdir(d)		::RemoveDirectory(d)


#elif WIN32


#define _tcscasecmp(str1, str2)				_stricmp(str1, str2)


typedef HANDLE  JPDIR;
typedef WIN32_FIND_DATA JFILEDATA;
typedef LPWIN32_FIND_DATAA LPJFILEDATA;

inline bool com_mkdir(const TCHAR *path)	{int res = CreateDirectory(path, NULL); return (res)?true:false; }
inline bool com_open(FILE *fp, const TCHAR *pPath, const TCHAR *pMode)	{errno_t res = fopen_s(&fp, pPath, pMode); return (!fp || res)?false:true; }
#define com_dirclose(dir)  FindClose(dir)
inline bool com_isInvalid(const TCHAR *fath) { if (GetFileAttributes(fath) == INVALID_FILE_ATTRIBUTES)return true; return false; }
inline bool com_isFile(const TCHAR *file) { int res = GetFileAttributes(file); if (res == INVALID_FILE_ATTRIBUTES || res == FILE_ATTRIBUTE_DIRECTORY)return false; return true; }
inline bool com_isDirectory(TCHAR *dir) { int res = GetFileAttributes(dir); if (res == FILE_ATTRIBUTE_DIRECTORY)return true; return false; }
inline JPDIR com_findFirstFile(const TCHAR *pDirName, LPJFILEDATA pFileData) { return FindFirstFile(pDirName, pFileData); }
inline bool com_nextFile(JPDIR dir, LPJFILEDATA pFileData) { return (FindNextFile(dir, pFileData))?true:false; }
inline void com_getFileName(TCHAR **pFileName, LPJFILEDATA pFileData) { *pFileName = pFileData->cFileName;}

inline bool jISREG(LPJFILEDATA pFD)		{return (pFD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? false : true;} //regular file?
inline bool jISDIR(LPJFILEDATA pFD)		{return (pFD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)?true:false;} //directory?

#define com_ISREG(m)		jISREG(m) //regular file?
#define com_ISDIR(m)		jISDIR(m) //directory?
#define com_rmdir(d)		::RemoveDirectory(d)

#else // Unix & Linux


#define _tcscasecmp(str1, str2)				strcasecmp(str1, str2)

// File
typedef DIR*	JPDIR;
typedef struct dirent JFILEDATA;


inline bool com_mkdir(const TCHAR *path)	{ int res = mkdir(path, 0777); return (res)?false:true; }  // O
inline bool com_open(FILE *fp, const TCHAR *pPath, const TCHAR *pMode)	{fp = fopen(pPath, pMode); return (!fp)?false:true; }
#define com_dirclose(dir)  closedir(dir)
inline bool com_isFile(TCHAR *path)			{ int res = access(path, F_OK); return (res)?false:true; } // O
inline bool com_isDirectory(TCHAR *path) { struct stat st; stat(path, &st); if (S_ISDIR(st.st_mode)) return true; return false; }
inline bool com_isInvalid(const TCHAR *fath) {  return (access(path, 0)) ? false : true; }

inline JPDIR com_findFirstFile(const TCHAR *pDirName, JFILEDATA *pFileData) { JPDIR res = opendir(pDirName); if(res){pFileData = readdir(res);} return res;}
inline bool com_nextFile(JPDIR dir, JFILEDATA *pFileData) { return (pFileData = readdir(dir))?true:false; }
inline void com_getFileName(TCHAR **pFileName, JFILEDATA *pFileData) { *pFileName = pFileData->d_name;}

inline bool jISREG(struct stat *pst)		{return S_ISREG(pst->st_mode);} 
inline bool jISDIR(struct stat *pst)		{return S_ISDIR(pst->st_mode);} // if not directory return 0

#define com_ISREG(m)		jISREG(m) //regular file?
#define com_ISDIR(m)		jISDIR(m) //directory?
inline bool com_rmdir(TCHAR *pPath)	{ int res = rmdir(pPath); return (res==-1)?false:true; }	

/*
int access(const char *pathname, int mode);
mode : 
R_OK	Readable?
W_OK	Writeable?
X_OK	Executable?
F_OK	File Existence

result - 0 : success, -1 : error (errno)
?
*/

#endif 



#ifdef DX64
typedef __int64 p_int;
#elif _WIN32
typedef int p_int;
#else // unix & Linux
typedef int p_int;
#endif


#endif	// __COMFILETYPES_H__
