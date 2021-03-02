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

#include "comList.h"
#include "comMString.h"
#include "comDateTime.h"

#define SAVE_FULLPATH	0x00000001
#define HAS_DIRNAME		0x00000002
#define DIR_RECURSIVE	0x00000004

//typedef	struct STFileInfo {
//	TCHAR	*fname;
//	TCHAR	stat;
//};

struct STFileInfoEx {
	TCHAR	*fname;
	TCHAR	stat;
	CFileTime stFileTime;  //month~second
	unsigned int nSize;
};

struct STFileList {
	TCHAR  *root;
	int nFlag;
	CSList *pList;
	void init(TCHAR *dir, bool isRecursive, bool bHasDir, bool bSaveFullPath)
	{
		root = dir;
		nFlag = 0;
		pList = NULL;
		if (isRecursive)	nFlag = DIR_RECURSIVE;
		if (bHasDir)		nFlag |= HAS_DIRNAME;
		if (bSaveFullPath)	nFlag |= SAVE_FULLPATH;
	}
};


typedef bool (*fp_FileFilter)(TCHAR *pName, void *pData, void *pRes, E_OPERATOR eOP);

struct STFilterData {
	E_OPERATOR eOP;
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

bool filter_stringCompare(TCHAR *pFile, void *pData, void *pRes, E_OPERATOR eOP);
bool filter_dateTimeCompare(TCHAR *pFile, void *pData, void *pRes, E_OPERATOR eOP);

class CFileUtil {
public:
	static void CheckDirName(TCHAR *dirName);
	static bool IsExistDirectory(TCHAR *dirName);
	static int GetFileList(STFileList *pResult, STFilterData *pFilter=NULL);
	static void destroyFileList(STFileList *pResult);
	static int delDirectory(STFileList *pData, STFilterData *filter = NULL);
	static bool		MakeDirectory(const TCHAR *dirName);
	static bool		FileCopy(const TCHAR *szSrc, const TCHAR *szDest);
	static bool		IsAbsolutePath(const TCHAR *pPath);
	static bool		setDirectory(char *pDir, int upCount);
	static bool changeDirName(TCHAR *pOriDir, TCHAR *pNewDir);
	static int read(TCHAR *pFilePath, TCHAR *pBuf, int nFileSize);
	//static int readNalloc(TCHAR *pFilePath, TCHAR **pBuf, int *aBufSize);
	static unsigned int readNalloc(TCHAR *pFilePath, STBuf *pBuf, unsigned int nSize=0);
	static int getNextLine(STStrInfo *pInfo);
	static int getNextLine(STLine *pLine);
	static bool getFieldList(TCHAR *pRecord, TCHAR cSeparator, CStringList *pList);
	static bool fileToFieldRecord(TCHAR *pFilePath, CSList *pLineList, TCHAR ch);
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

private:
	//static bool	appendFileList(CSList *flist, const TCHAR *fname, TCHAR ftype);
	static bool	appendFileList(CSList **flist, TCHAR *fname, TCHAR ftype, CFileTime *pFileTime, unsigned int nSize);
};



#endif	//	__CFileUtil_H__

