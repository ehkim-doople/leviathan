/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                   */
/*   Copyright 2012 by coms All Right Reserved                               */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/* **********************************************************

 *	FILE : fileutil.cpp

 * ********************************************************** */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <malloc.h>
#include <errno.h>
#include "comFileTypes.h"
#include "comFileUtil.h"
#include "comLogger.h"

#define MAX_PATH_LEN 256
#define UNIT_FILE_LIST 64

bool filter_stringCompare(TCHAR *pFile, void *pData, void *pRes)
{
	if (!pData) return true;
	CStringList *filterList = NULL;
	if (!pRes) {
		TCHAR *pFilter = (TCHAR *)pData;
		filterList = new CStringList();
		filterList->alloc(4);
		CFileUtil::getFieldList((char *)pFilter, CH_DELIMITER, filterList);
	}
	else {
		filterList = (CStringList *)pRes;
	}
	int i = 0;
	CmString *filter = filterList->getNext(&i);
	while (filter)
	{
		if (filter->compare(pFile)) return true;
		filter = filterList->getNext(&i);
	}
	return false;
}

bool filter_dateTimeCompare(TCHAR *pFile, void *pData, void *pRes)
{
	CFileTime *pComperand = (CFileTime *)pData;
	CFileTime *pComFileTime = (CFileTime *)pRes;
	if (pComFileTime->isCompare(pComperand, eOperator_LE)) // LE : less or Equal
	{
		return true;
	}
	return false;
}

void CheckDirName(TCHAR *dirName)
{
#ifndef WIN32
	TCHAR	fromChar = '\\';
	TCHAR	toChar = '/';
#else
	TCHAR	fromChar = '/';
	TCHAR	toChar = '\\';
#endif

	while (dirName && *dirName)
	{
		if (*dirName == fromChar)
			*dirName = toChar;
		dirName++;
	}
}

bool CFileUtil::IsExistDirectory(TCHAR *dirName)
{
	bool bRes = false;
	if (com_isDirectory(dirName))
#ifndef WIN32 // -- only unix
	{
		bRes = true;
	}
#else	
	{
		bRes = true;
		_stprintf(dirName, _T("%s*.*"), dirName);
	}
#endif
	return bRes;
}

bool CFileUtil::GetFileList(STFileList *pResult, STFilterData *pFilter)
{
	JPDIR dir;
	JFILEDATA  file;

	CStringList *filterList = NULL;
	TCHAR	*fname= NULL;
	CTList<fileInfo> *fList = NULL;
	int nCount = 0;
	TCHAR dirName[128];

	if (!pResult) return false;
	_tcscpy(dirName, pResult->root);

#ifndef WIN32
	struct  stat st;
#endif

	if (!IsExistDirectory(dirName)) return false;

	dir = com_findFirstFile(dirName, &file);
	if (0 >= dir) return false;

	do  {
		com_getFileName(&fname, &file);

		if (_tcscmp(fname, _T(".")) == 0
			|| _tcscmp(fname, _T("..")) == 0)
			continue;

#ifndef WIN32 // -- only unix
		_stprintf(filePath, _T("%s%s"), szDir, fname);
		stat(filePath, &st);
		if(com_ISDIR(&st)) { appendFileList(fList, fname, 'D');}
		else if(com_ISREG(&st)) {
#else		
		if(com_ISDIR(&file)) { appendFileList(fList, fname, 'D'); }
		else if(com_ISREG(&file)) {
#endif
			if (pFilter) {
				if (!pFilter->pFunc(fname, pFilter->pData, filterList)) continue;
			}
			appendFileList(fList, fname, 'F');
		}
	} while (com_nextFile(dir, &file));

	com_dirclose(dir);
	pResult->pList = fList;
	delete filterList;
	return true;
}


int CFileUtil::delDirectory(STDelInfo *pData, STFilterData *filter)
{
	JPDIR dir;
	JFILEDATA  file;
	TCHAR	*fname = NULL, filePath[256], szDir[128], szSubDir[128];
	int nCount = 0, nResult;
	bool bDirectory = false;
	CFileTime cFTime;
	STDelInfo stSubDir;
	void *pRes = NULL;
	CStringList *filterList = NULL;

#ifndef WIN32
	struct  stat st;
#endif

	if (!pData->root) {
		// TODO LOG
		return 0;
	}
	_tcscpy(szDir, pData->root);

	if (!IsExistDirectory(szDir)) return 0;


	dir = com_findFirstFile(szDir, &file);
	if (0 >= dir) return 0;
	while (com_nextFile(dir, &file))
	{
		com_getFileName(&fname, &file);
		if (_tcscmp(fname, _T(".")) == 0
			|| _tcscmp(fname, _T("..")) == 0)
			continue;

		_stprintf(filePath, _T("%s%s"), pData->root, fname);
#ifndef WIN32 // -- only unix
		stat(filePath, &st);
		if (com_ISDIR(&st)) bDirectory = true;
		if (filter) {
			if (filter->pFunc == filter_dateTimeCompare) { cFTime.setFileTime(st.st_mtime); pRes = &cFTime; }
			else if (filter->pFunc == filter_stringCompare) { pRes = filterList; }
		}
#else
		cFTime.setFileTime(&file.ftLastWriteTime);
		// windows
		if (com_ISDIR(&file)) bDirectory = true;
		if (filter) {
			if (filter->pFunc == filter_dateTimeCompare) { 
				//cFTime.setFileTime(filePath); 
				pRes = &cFTime; 
			}
			else if (filter->pFunc == filter_stringCompare) { pRes = filterList; }
		}
#endif

		if (bDirectory) {
			if (pData->nFlag & DIR_RECURSIVE) {
				_stprintf(szSubDir, _T("%s%s%c"), pData->root, fname, g_s);
				memcpy(&stSubDir, pData, sizeof(STDelInfo));
				stSubDir.root = szSubDir;
				delDirectory(&stSubDir, filter);
				bDirectory = false;
			}
		}
		else {
			if (filter) {
				if (filter->pFunc(fname, filter->pData, pRes)) nResult = 1;
				else nResult = 0;
			}
			else nResult = 1;


			if (nResult) {
				nResult = _tremove(filePath);
				if (nResult < 0) {
					// TODO ERROR LOG
					gs_cLogger.DebugLog(LEVEL_WARN, _T("FAILED _tremove[%s] nResult[%d]"), filePath, nResult);
				}
				else {
					gs_cLogger.DebugLog(LEVEL_INFO, _T("SUCCESS _tremove[%s] nResult[%d]"), filePath, nResult);
				}
				// TODO TRACE LOG
				nCount++;
			}
		}
	}
	com_dirclose(dir);
	if (filterList) delete filterList;
	if (pData->nFlag & DEL_DIRECTORY) {
		if (com_rmdir(pData->root)) {
			gs_cLogger.DebugLog(LEVEL_INFO, _T("SUCCESS rmdir[%s]"), pData->root);
		}
		else {
			gs_cLogger.DebugLog(LEVEL_WARN, _T("FAILED rmdir[%s]"), pData->root);
		}
	}
	return nCount;
}




bool CFileUtil::setDirectory(char *pDir, int upCount)
{
	if (!pDir) {
		return false;
	}
	size_t	nLen = (int)_tcslen(pDir);
	char *pRear = pDir;

	int i;
	for (i = 0; i < upCount; i++) {
		pRear = _tcsrchr((TCHAR *)pDir, g_s);
		*pRear = 0;
	}

	pRear = _tcsrchr((TCHAR *)pDir, g_s);
	nLen = pRear - pDir + 1;

	_tcsncpy(pDir, pDir, nLen); pDir[nLen] = 0;
	return true;
}

bool CFileUtil::appendFileList(CTList<fileInfo> *flist, const TCHAR *fname, TCHAR ftype)
{
	if (!flist) {
		flist = new CTList<fileInfo>();
		if (!flist) return false;
		if (!flist->alloc(UNIT_FILE_LIST, eAlloc_Type_BufPool)) return false;
	}

	int nIdx, i = 0, nLen = sizeof(FileInfo);
	FileInfo *pInfo = (FileInfo *)gs_pMMgr->newBuf(nLen, &nIdx);

	pInfo->fname = gs_pMMgr->newBuf((int)_tcslen(fname) + 1);
	_tcscpy(pInfo->fname, fname);
	pInfo->stat = ftype;

	// add
	if (flist->push_back(pInfo)) {
		gs_cLogger.PutLogQueue(LEVEL_TRACE, _T("fname[%s] nCount[%d]"), pInfo->fname, flist->size());
		return true;
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T(" fname[%d] ftype[%d]"), fname, ftype);
		return false;
	}
}

bool CFileUtil::MakeDirectory(const TCHAR *dirName)
{
	TCHAR	tmpBuf[MAX_PATH_LEN];
	TCHAR	*ptr, *dptr = (TCHAR *)dirName;
	int	len;	
	CheckDirName((TCHAR*)dirName);
#ifndef WIN32
	TCHAR szTmp[2][5] = {_T("./"), _T("../")};
#else
	TCHAR szTmp[2][5] = {_T(".\\"), _T("..\\")};
#endif
	while (dptr && (ptr = _tcschr(dptr, g_s)))
	{
		len = (int)(ptr - dirName) + 1;
		_tcsncpy(tmpBuf, dirName, len);
		tmpBuf[len] = 0;

		if (strcmp(tmpBuf, szTmp[0]) && strcmp(tmpBuf, szTmp[1])) {
			if (com_isInvalid(tmpBuf)) {
				if (!com_mkdir(tmpBuf)) goto MDIR_ERROR;
			}
		}
		else goto MDIR_ERROR;
		dptr = ptr + 1;
	}
	return true;
MDIR_ERROR:
	gs_cLogger.DebugLog(LEVEL_ERROR, "com_mkdir has Failed! [%s]", tmpBuf);
	return false;
}

bool CFileUtil::IsAbsolutePath(const TCHAR *pPath)
{
	if (!pPath)
		return false;

	if (pPath[1] == ':' || pPath[0] == '\\' || pPath[0] == '/')
		return true;
	else
		return false;
}


bool CFileUtil::FileCopy(const TCHAR *szSrc, const TCHAR *szDest)
{

	TCHAR	szBuf[8192];
	size_t	nLen;
	FILE *inFp=NULL, *outFp=NULL;

	inFp = _tfopen(szSrc, _T("rb"));
	if(!inFp) {
		return false;
	}

	outFp = _tfopen(szDest, _T("wb+"));
	if(!outFp) {
		fclose(inFp);
		return false;
	}

	nLen = fread(szBuf, g_nChSize, sizeof(szBuf), inFp);
	while (0 < nLen) 
    {
		fwrite(szBuf, g_nChSize, nLen, outFp);
		if (nLen < sizeof(szBuf))
			break;

        nLen = fread(szBuf, g_nChSize, sizeof(szBuf), inFp);
	}

	fclose(inFp);
	fclose(outFp);
	return true;
}

bool CFileUtil::changeDirName(TCHAR *pOriDir, TCHAR *pNewDir)
{
	MakeDirectory(pNewDir);
	if ( _trename(pOriDir, pNewDir) != 0 ) {
		return false;
	}
	return true;
}


int CFileUtil::read(TCHAR *pFilePath, TCHAR *pBuf, int nFileSize)
{
	FILE *pFile = _tfopen(pFilePath, _T("rb"));
	int nSize = 0;

	if (!pFile) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::read pFile open FAIL [%s]"), pFilePath);
		return 0;
	}
	nSize = (int)fread(pBuf, g_nChSize, nFileSize, pFile);
	pBuf[nSize] = 0;
	fclose(pFile);
	return nSize;
}

bool CFileUtil::fileToFieldRecord(TCHAR *pFilePath, CTList<CStringList> *pLineList, TCHAR ch)
{
	// FILE READ
	TCHAR *pFileBuf = NULL;
	int fileSize = 0;
	CStringList *pList;
	bool bres;
	STStrInfo stInfo;

	memset(&stInfo, 0, sizeof(STStrInfo));
	fileSize = readNalloc(pFilePath, &pFileBuf, &fileSize);

	if (!fileSize) {
		bres = false;
		goto END_PROC;
	}

	stInfo.aBufSize = 4096;
	stInfo.pSource = pFileBuf;
	stInfo.pTarget = gs_pMMgr->newBuf(4096);
	if (!stInfo.pTarget) {
		bres = false;
		goto END_PROC;
	}

	while(getNextLine(&stInfo))
	{
		pList = new (std::nothrow) CStringList();
		if (!pList) { gs_cLogger.DebugLog(LEVEL_ERROR, _T("new pList")); goto ERROR_PROC; }
		if (!pList->alloc(10)) { gs_cLogger.DebugLog(LEVEL_ERROR, _T("alloc pList")); goto ERROR_PROC; }
		if (!getFieldList(stInfo.pTarget, ch, pList)) {
			goto ERROR_PROC;
		}
		if (!pLineList->push_back(pList)) { gs_cLogger.DebugLog(LEVEL_ERROR, _T("add pLineList")); goto ERROR_PROC; }
	}
	bres = true;
	goto END_PROC;
ERROR_PROC:
	if (pLineList) {
		int idx = 0;
		pList = pLineList->getNext(&idx);
		while (pList) {
			delete pList;
			idx++;
			pList = pLineList->getNext(&idx);
		}
	}
	bres = false;
END_PROC:
	if (pFileBuf) gs_pMMgr->delBuf(pFileBuf, fileSize);
	if(stInfo.pTarget) gs_pMMgr->delBuf(stInfo.pTarget, stInfo.aBufSize);
	return bres;
}

bool CFileUtil::getFieldList(TCHAR *pRecord, TCHAR cSeparator, CStringList *pList)
{
	int nLen; 
	TCHAR *pNext, word[512];
	bool bEnd=false;

	if (!cSeparator) {
		if (pList->push_back(pRecord)) {
			return true;
		}
		return false;
	}

	while (*pRecord)
	{
		while (*pRecord == CH_SPC || *pRecord == CH_TAB) pRecord++;
		pNext = strchr(pRecord, cSeparator);
		if (!pNext) {
			if (!pNext) pNext = strchr(pRecord, 0);
			if (!pNext) pNext = strchr(pRecord, g_rc);
			if (!pNext) return false;
			bEnd = true;
		}
		nLen = (int)(pNext - pRecord);
		_tcsncpy(word, pRecord, nLen); word[nLen] = 0;
		if (!pList->push_back(word)) {
			return false;
		}
		if (bEnd) return true;
		pRecord = pNext + 1;
	}
	return true;
}

int CFileUtil::readNalloc(TCHAR *pFilePath, TCHAR **pBuf, int *aBufSize)
{
	FILE *pFile = _tfopen(pFilePath, _T("rb"));
	int nSize = 0, nRead, nPos=0, nBufSize;

	if (!pFile) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::read pFile open FAIL [%s]"), pFilePath);
		return 0;
	}
	fseek(pFile, 0, SEEK_END);
	nSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	nBufSize = nSize + 1;
	if (*aBufSize < nBufSize) {
		if (*aBufSize) {
			gs_pMMgr->delBuf(*pBuf, *aBufSize);
		}
		*pBuf = gs_pMMgr->newBuf(nBufSize);
		if (!*pBuf) {
			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CFileUtil::readNalloc malloc error! size[%ld]"), nBufSize);
			return 0;
		}
		*aBufSize = nBufSize;
	}

	while ((nRead = (int)fread(*pBuf + nPos, 1, nSize - nPos, pFile)) > 0)
	{
		nPos += nRead;
	}

	if (!nPos) {
		printf("fread errno[%d]\n", errno);
	}
	(*pBuf)[nPos] = 0;
	fclose(pFile);
	return nSize;
}

int CFileUtil::getNextLine(STStrInfo *pInfo)
{
	int nSize = 0, nBufSize;  //g_nChSize
	TCHAR *pNextLine = _tcschr(pInfo->pSource, g_rc);
	bool bEnd = false;

	if (pNextLine) {
		nSize = (int)(pNextLine - pInfo->pSource) + 1;
	}
	else {
		pNextLine = _tcschr(pInfo->pSource, 0);
		if(!pNextLine) return 0;
		nSize = (int)(pNextLine - pInfo->pSource);
		bEnd = true;
	}

	*pNextLine = 0;
	nBufSize = nSize * g_nChSize;
	if (pInfo->aBufSize < nBufSize) {
		if (pInfo->aBufSize) {
			gs_pMMgr->delBuf(pInfo->pTarget, pInfo->aBufSize);
		}
		nBufSize += 1024; // Allocating enough space
		pInfo->pTarget = gs_pMMgr->newBuf(nBufSize);
		if (!pInfo->pTarget) {
			pInfo->aBufSize = 0;
			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CFileUtil::getNextLine gs_pMMgr->newBuf error! size[%ld]"), nBufSize);
			return 0;
		}
		pInfo->aBufSize = nBufSize;
	}

	_tcscpy(pInfo->pTarget, pInfo->pSource);
	if (bEnd) return 0;
	pInfo->pSource += nSize;
	return nSize;
}

bool CFileUtil::add(TCHAR *pFilePath, TCHAR *pBuf)
{
	// File copy
	FILE * pFile;
	pFile = _tfopen(pFilePath, _T("a+"));
	if (!pFile) {
		CFileUtil::MakeDirectory(pFilePath);
		pFile = _tfopen(pFilePath, _T("a+"));
		if (!pFile) {
			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::add pFile open FAIL [%s]"), pFilePath);
			return false;
		}
		return false;
	}
	fwrite(pBuf, sizeof(TCHAR), _tcslen(pBuf), pFile);
	fflush(pFile);
	fclose(pFile);
	return true;
}
bool CFileUtil::getReFileName(TCHAR *pTarget, const TCHAR *pOriPath, const TCHAR *pChangeStr)
{
	const TCHAR *pPos = _tcsrchr(pOriPath, _T('.'));
	size_t nLen = pPos - pOriPath;
	_tcsncpy(pTarget, pOriPath, nLen);
	_tcscpy(&pTarget[nLen], pChangeStr);
	return true;
}
bool  CFileUtil::renameFile(const TCHAR *pOldFileName, const TCHAR *pNewFileName)
{
	//TCHAR pNameBackup[255];
	//_stprintf(pNameBackup, _T("%s.BAK"), pFilePath);

	if (com_isFile(pOldFileName))
	{
		//if (com_isFile(pOldFileName)) {
		//	if (!DeleteFile(pOldFileName)) {
		//		gs_cLogger.DebugLog(LEVEL_ERROR, _T("CDFSFile::renameFile DeleteFile error code[%d]"), GetLastError());
		//		return false;
		//	}
		//}

		int nRes = FILEMOVE(pOldFileName, pNewFileName);
#ifdef WIN32
		if(!nRes)
#else
		if (nRes)
#endif

		{
			DWORD error = GetLastError();
			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("FILEMOVE error code[%d]"), error);
			return false;
		}
	}
	return true;
}

int CFileUtil::getFileSize(TCHAR *pFilePath)
{
	int nTotSize;
	FILE *pFile;

	pFile = _tfopen(pFilePath, _T("r"));
	if (!pFile) {
		return 0;
	}

	fseek(pFile, 0, SEEK_END);
	nTotSize = ftell(pFile);
	fclose(pFile);

	return nTotSize;
}
FILE * CFileUtil::tryopen(int tryCnt, TCHAR *pFilePath, const TCHAR *pMode, int *pSize)
{
	int i, res;
	FILE *pFile;
	for (i = 0; i < tryCnt; i++) {
		pFile = _tfopen(pFilePath, pMode);
		if (pFile) {
			res = fseek(pFile, 0, SEEK_END);
			*pSize = ftell(pFile);
			return pFile;
		}
		// 지연 밀리세컨드
	}
	gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CFileUtil::tryopen pFile open FAIL"));
	return NULL;
}
bool CFileUtil::modify(TCHAR *pFilePath, int nSize, TCHAR *pBuf)
{
	FILE *pFile = _tfopen(pFilePath, _T("w"));

	if (!pFile) {

		CFileUtil::MakeDirectory(pFilePath);
		pFile = _tfopen(pFilePath, _T("w"));

		if (!pFile) {
			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CFileUtil::modify pFile open FAIL [%s]"), pFilePath);
			return false;
		}
	}

	fwrite(pBuf, g_nChSize, nSize, pFile);
	fflush(pFile);
	fclose(pFile);
	return true;
}



int CFileUtil::getFileSize2(TCHAR *pFilePath, int nMaxBuf, int *pLoopLen, int nPos)
{
	int i = 0;
	int nTotSize;
	FILE *pFile;

	pFile = _tfopen(pFilePath, _T("r"));
	if (!pFile) {
		return 0;
	}

	fseek(pFile, 0, SEEK_END);
	nTotSize = ftell(pFile);
	fclose(pFile);

	//if(!nMaxBuf) return nTotSize; 

	nTotSize = nTotSize / g_nChSize - nPos;

	*pLoopLen = (int)(nTotSize / nMaxBuf);
	if (*pLoopLen * nMaxBuf < nTotSize) {
		*pLoopLen += 1;
	}
	return nTotSize;
}


int CFileUtil::readLoop(TCHAR *pFilePath, int nLoopCnt, int nMaxBuf, STFileMem *pMem, int ntotSize)
{
	int i;
	int nTot = 0, nEndSize;
	int nReadSize;

	FILE *pFile;
	pFile = _tfopen(pFilePath, _T("rb"));
	if (!pFile) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::readLoop pFile open FAIL [%s]"), pFilePath);
		return 0;
	}

	for (i = 0; i < nLoopCnt; i++) {
		if (i == nLoopCnt - 1) {
			nEndSize = ntotSize - nTot;
			nReadSize = (int)fread(pMem[i].pBlock, g_nChSize, nEndSize, pFile);
			nTot += nReadSize;
			pMem[i].pBlock[nReadSize] = 0;
		}
		else {
			nReadSize = (int)fread(pMem[i].pBlock, g_nChSize, nMaxBuf - 1, pFile);
			nTot += nReadSize;
			pMem[i].pBlock[nReadSize] = 0;
		}
	}
	fclose(pFile);
	return nTot;
}

int CFileUtil::nextReadLoop(TCHAR *pFilePath, int nLoopCnt, int nMaxBuf, STFileMem *pMem, int nPos, int ntotSize)
{
	int i;
	int nTot = 0;
	int nEndSize;
	int nReadSize;

	FILE *pFile;
	for (i = 0; i < 3; i++) {
		pFile = _tfopen(pFilePath, _T("rb"));
		if (pFile) break;
		// 지연 밀리세컨드
	}
	if (!pFile) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::nextReadLoop pFile open FAIL [%s]"), pFilePath);
		return 0;
	}

	fseek(pFile, nPos*g_nChSize, SEEK_SET);

	for (i = 0; i < nLoopCnt; i++) {
		if (i == nLoopCnt - 1) {
			nEndSize = ntotSize - nTot;
			nReadSize = (int)fread(pMem[i].pBlock, g_nChSize, nEndSize, pFile);
			nTot += nReadSize;
			pMem[i].pBlock[nReadSize] = 0;
		}
		else {
			nReadSize = (int)fread(pMem[i].pBlock, g_nChSize, nMaxBuf - 1, pFile);
			nTot += nReadSize;
			pMem[i].pBlock[nReadSize] = 0;
		}
	}
	fclose(pFile);

	return nTot;
}


int CFileUtil::nextRead(TCHAR *pFilePath, TCHAR *pBuf, int nMaxBuf, int nPos)
{
	FILE *pFile = _tfopen(pFilePath, _T("rb"));
	if (!pFile) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::nextRead pFile open FAIL [%s]"), pFilePath);
		return 0;
	}

	fseek(pFile, nPos*g_nChSize, SEEK_SET);

	int nSize = (int)fread(pBuf, g_nChSize, nMaxBuf, pFile);
	pBuf[nSize] = 0;
	fclose(pFile);
	return nSize;
}


bool CFileUtil::modify(TCHAR *pFilePath, int nLoopCnt, STFileMem *pBuf)
{
	int i;
	FILE *pFile = _tfopen(pFilePath, _T("w"));

	if (!pFile) {

		CFileUtil::MakeDirectory(pFilePath);
		pFile = _tfopen(pFilePath, _T("w"));

		if (!pFile) {
			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::modify pFile open FAIL [%s]"), pFilePath);
			return false;
		}
	}

	for (i = 0; i < nLoopCnt; i++) {
		fwrite(pBuf[i].pBlock, g_nChSize, _tcslen(pBuf[i].pBlock), pFile);
		//_ftprintf(pBackup, pBuf[i]);
	}
	fflush(pFile);
	fclose(pFile);
	return true;
}
bool CFileUtil::add(TCHAR *pFilePath, int nLoopCnt, STFileMem *pBuf)
{
	int i;
	FILE *pFile = _tfopen(pFilePath, _T("a+"));

	if (!pFile) {

		CFileUtil::MakeDirectory(pFilePath);
		pFile = _tfopen(pFilePath, _T("a+"));

		if (!pFile) {
			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::add2 pFile open FAIL [%s]"), pFilePath);
			return false;
		}
	}

	for (i = 0; i < nLoopCnt; i++) {
		fwrite(pBuf[i].pBlock, g_nChSize, _tcslen(pBuf[i].pBlock), pFile);
		//_ftprintf(pBackup, pBuf[i]);
	}
	fflush(pFile);
	fclose(pFile);
	return true;

}


char * CFileUtil::ehstrstr(const char *pTarget, const char *psubstr)
{
	char *pPos = (char *)pTarget;
	int nIdx=0, nLen= (int)strlen(psubstr);
	while (*pPos) {
		while (*pPos == psubstr[nIdx++]) {
			if (nIdx >= nLen) return pPos;
			pPos++; if (!*pPos) return NULL;			
		}
		nIdx = 0;
		pPos++;
	}
	return NULL;
}


/* ***********************************************************

 *		END of CFileUtil.cpp

 * *********************************************************** */

 //bool CFileUtil::checkFileFilter(const TCHAR *fname, CStringList *slist)
 //{
 //	size_t i = 0;
 //	CmString *filter = slist->getNext(&i);
 //	while(filter)
 //	{
 //		if (filter->compare(fname)) return true;
 //		filter = slist->getNext(&i);
 //	}
 //	return false;
 //}