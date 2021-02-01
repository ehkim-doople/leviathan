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
File Utility API
/******************************************************************************/

#ifndef	__CFileUtil_H__
#define	__CFileUtil_H__

#include "comTList.h"
#include "comMString.h"

#define DIR_RECURSIVE 0x00000001
#define DEL_DIRECTORY 0x00000002

typedef	struct fileInfo {
	TCHAR	*fname;
	TCHAR	stat;
} FileInfo;

struct STFileList {
	TCHAR	*root;
	CTList<fileInfo> *pList;
};

struct STDelInfo {
	TCHAR *root;
	int nFlag;
	void *pData;
	void init(TCHAR *dir, bool isRecursive, bool bDelDir, void *p=NULL)
	{
		pData = p;
		root = dir;
		nFlag = 0;
		if (isRecursive) nFlag = DIR_RECURSIVE;
		if (bDelDir) nFlag |= DEL_DIRECTORY;
	}
};


typedef bool (*fp_FileFilter)(TCHAR *pName, void *pData, void *pRes); 

struct STFilterData {
	fp_FileFilter pFunc;
	void *pData;
};

struct STMemInfo;

enum E_CH
{
	CH_TAB = 9,
	CH_LF = 10,		// linux line feed
	CH_CR = 13,		// carriage return
	CH_SPC = 32,	// space
	CH_DELIMITER = 59	// ;
};


struct STFileMem
{
	TCHAR *pBlock;
	int nKey;
};

bool filter_stringCompare(TCHAR *pFile, void *pData, void *pRes);
bool filter_dateTimeCompare(TCHAR *pFile, void *pData, void *pRes);

class CFileUtil {
public:
	static bool IsExistDirectory(TCHAR *dirName);
	static bool GetFileList(STFileList *pResult, STFilterData *pFilter=NULL);
	static int delDirectory(STDelInfo *pData, STFilterData *filter = NULL);
	static bool		MakeDirectory(const TCHAR *dirName);
	static bool		FileCopy(const TCHAR *szSrc, const TCHAR *szDest);
	static bool		IsAbsolutePath(const TCHAR *pPath);
	static bool		setDirectory(char *pDir, int upCount);
	static bool changeDirName(TCHAR *pOriDir, TCHAR *pNewDir);
	static int read(TCHAR *pFilePath, TCHAR *pBuf, int nFileSize);
	static int readNalloc(TCHAR *pFilePath, TCHAR **pBuf, int *aBufSize);
	static int getNextLine(STStrInfo *pInfo);
	static bool getFieldList(TCHAR *pRecord, TCHAR cSeparator, CStringList *pList);
	static bool fileToFieldRecord(TCHAR *pFilePath, CTList<CStringList> *pLineList, TCHAR ch);
	static bool add(TCHAR *pFilePath, TCHAR *pBuf);
	static bool getReFileName(TCHAR *pTarget, const TCHAR *pOriPath, const TCHAR *pChangeStr);
	static bool  renameFile(const TCHAR *pOldFileName, const TCHAR *pNewFileName);
	static int getFileSize(TCHAR *pFilePath);
	static FILE * tryopen(int tryCnt, TCHAR *pFilePath, const TCHAR *pMode, int *pSize);
	static bool modify(TCHAR *pFilePath, int nSize, TCHAR *pBuf);
	//static char* UTF8ToANSI(const char *pszCode);
	//static char* ANSIToUTF8(const char *pszCode);
	static int getFileSize2(TCHAR *pFilePath, int nMaxBuf=0, int *pLoopLen=NULL, int nPos=0);
	static int readLoop(TCHAR *pFilePath, int nLoopCnt, int nMaxBuf, STFileMem *pMem, int ntotSize);
	static int nextReadLoop(TCHAR *pFilePath, int nLoopCnt, int nMaxBuf, STFileMem *pMem, int nPos, int ntotSize);
	static int nextRead(TCHAR *pFilePath, TCHAR *pBuf, int nMaxBuf, int nPos);
	static bool modify(TCHAR *pFilePath, int nLoopCnt, STFileMem *pBuf);
	static bool add(TCHAR *pFilePath, int nLoopCnt, STFileMem *pBuf);
	static char * ehstrstr(const char *pTarget, const char *psubstr);

private:

	//static void			appendStringList(CStringList *slist, const TCHAR *str);
	static bool			appendFileList(CTList<fileInfo> *flist, const TCHAR *fname, TCHAR ftype);
	//static bool			checkFileFilter(const TCHAR *fname, CStringList *slist);

};



#endif	//	__CFileUtil_H__

