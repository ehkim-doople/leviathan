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
#include <errno.h>
#include "comFileTypes.h"
#include "comFileUtil.h"
#include "comLogger.h"

#define MAX_PATH_LEN 256
#define UNIT_FILE_LIST 64

bool filter_stringCompare(TCHAR *pFile, void *pData, void *pRes, E_OPERATOR eOP)
{
	CStringList *filterList = NULL;

	if (pRes) {
		filterList = (CStringList *)pRes;
	}
	else {
		if (!pData) return true; // nothing to compare
		TCHAR *pFilter = (TCHAR *)pData;
		filterList = new CStringList();
		filterList->alloc(4);
		CFileUtil::getFieldList((char *)pFilter, CH_DELIMITER, filterList);
	}

	int i = 0;
	CmString *filter = filterList->getNext(&i);
	while (filter)
	{
		if (filter->compare(pFile, eOP)) return true;  // string compare
		i++;
		filter = filterList->getNext(&i);
	}
	return false;
}

bool filter_dateTimeCompare(TCHAR *pFile, void *pData, void *pRes, E_OPERATOR eOP)
{
	CFileTime *pComperand = (CFileTime *)pData;
	CFileTime *pComFileTime = (CFileTime *)pRes;
	if (pComFileTime->isCompare(pComperand, eOP)) // LE : less or Equal
	{
		return true;
	}
	return false;
}

void CFileUtil::CheckDirName(TCHAR *dirName)
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

int CFileUtil::GetFileList(STFileList *pResult, STFilterData *pFilter)
{
	JPDIR dir;
	JFILEDATA  file;

	CStringList *filterList = NULL;
	TCHAR	*fname= NULL, *pFileName=NULL;
	int nCount = 0, nResult;
	TCHAR dirName[128], szFullPathName[256];
	unsigned int nSize;
	void *pRes = NULL;
	CFileTime cFileTime;
	TCHAR nStat;

	if (!pResult) return 0;
	_tcscpy(dirName, pResult->root);

	if (pResult->nFlag & SAVE_FULLPATH) pFileName = szFullPathName;

#ifndef WIN32
	struct  stat st;
#endif

	if (!IsExistDirectory(dirName)) return 0;

	// StringCompare initialize
	if (pFilter && pFilter->pFunc == filter_stringCompare) {
		filterList = new CStringList();
		filterList->alloc(4);
		CFileUtil::getFieldList((char *)pFilter->pData, CH_DELIMITER, filterList);
		pRes = filterList;
	}

	dir = com_findFirstFile(dirName, &file);
	if (0 >= dir) return 0;

	do  {
		com_getFileName(&fname, &file);

		if (_tcscmp(fname, _T(".")) == 0
			|| _tcscmp(fname, _T("..")) == 0)
			continue;

		if(!pFileName) pFileName = fname;

#ifndef WIN32 // -- only unix
		_stprintf(szFullPathName, _T("%s%s"), szDir, fname);
		stat(szFullPathName, &st);
		cFileTime.setFileTime(&st.st_ctime);
		nSize = st.st_size;

		if(com_ISDIR(&st)) { 
#else		
		cFileTime.setFileTime(&file.ftLastWriteTime);
		nSize = file.nFileSizeLow;

		if (com_ISDIR(&file)) {
#endif
			_stprintf(szFullPathName, _T("%s%s%c"), pResult->root, fname, g_s);
			if (pResult->nFlag & DIR_RECURSIVE) {
				STFileList stSubDir;
				memcpy(&stSubDir, pResult, sizeof(STFileList));
				stSubDir.root = szFullPathName;
				nCount += GetFileList(&stSubDir, pFilter);
				if (!pResult->pList && nCount) pResult->pList = stSubDir.pList;
			}
			
			if (pResult->nFlag & HAS_DIRNAME) nStat = _T('D');
			else continue;
		} // end directory
#ifndef WIN32
		else if (com_ISREG(&st)) {
#else
		else if (com_ISREG(&file)) {
#endif
			if (pResult->nFlag & SAVE_FULLPATH) _stprintf(szFullPathName, _T("%s%s"), pResult->root, fname);
			nStat = _T('F');
		}// end file

		if (pFilter) {
			if (pFilter->pFunc == filter_dateTimeCompare) { pRes = &cFileTime; }
			if (pFilter->pFunc(pFileName, pFilter->pData, pRes, pFilter->eOP)) nResult = 1;
			else nResult = 0;
		} else nResult = 1;

		if (nResult) {
			appendFileList(&pResult->pList, pFileName, nStat, &cFileTime, nSize); nCount++;
		}
	} while (com_nextFile(dir, &file));

	com_dirclose(dir);
	if(filterList) delete filterList;
	return nCount;
}


int CFileUtil::delDirectory(STFileList *pData, STFilterData *filter)
{
	int nCount = 0, nResult=1, nIdx=0;
	void *pRes = NULL;
	CStringList *filterList = NULL;
	STFileInfoEx *pFileInfo;

	if (!pData->root) {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T("pData->root is NULL"), pData->root);
		return 0;
	}

	if (!com_isDirectory(pData->root)) return 0;

	// StringCompare initialize
	if (filter && filter->pFunc == filter_stringCompare) {
		filterList = new CStringList();
		filterList->alloc(4);
		CFileUtil::getFieldList((char *)filter->pData, CH_DELIMITER, filterList);
		pRes = filterList;
	}

	pFileInfo = (STFileInfoEx *)pData->pList->getNext(&nIdx);
	while (pFileInfo)
	{			
		if (filter) {
			if (filter->pFunc == filter_dateTimeCompare) { pRes = &pFileInfo->stFileTime; }
			if (filter->pFunc(pFileInfo->fname, filter->pData, pRes, filter->eOP)) nResult = 1;
			else nResult = 0;
		}
		else nResult = 1;

		if (nResult) {
			nResult = 0;
			if (pFileInfo->stat == 'D') {
				if (pData->nFlag & HAS_DIRNAME) {
					if (com_rmdir(pFileInfo->fname)) {
						nResult = 1;
						gs_cLogger.DebugLog(LEVEL_TRACE, _T("SUCCESS rmdir[%s]"), pFileInfo->fname);
					}
					else {
						nResult = 0;
						gs_cLogger.DebugLog(LEVEL_TRACE, _T("FAILED rmdir[%s]"), pFileInfo->fname);
					}
				}
			}
			else {//'F'
				nResult = _tremove(pFileInfo->fname);
				if (nResult < 0) {
					gs_cLogger.DebugLog(LEVEL_TRACE, _T("FAILED _tremove[%s] nResult[%d]"), pFileInfo->fname, nResult);
				}
				else {
					nResult = 1;
					gs_cLogger.DebugLog(LEVEL_TRACE, _T("SUCCESS _tremove[%s] nResult[%d]"), pFileInfo->fname, nResult);
				}
			}
			if (0 < nResult) {
				gs_pMMgr->delString(pFileInfo->fname);
				pData->pList->del(nIdx, true); // deep delete
				nCount++;
			}
		}
		nIdx++;
		pFileInfo = (STFileInfoEx *)pData->pList->getNext(&nIdx);
	} // end while

	if (!pData->pList->size()) {
		free(pData->pList);
		pData->pList = NULL;
		gs_cLogger.DebugLog(LEVEL_TRACE, _T("destroy list"));
	}

	if (filterList) delete filterList;
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

bool CFileUtil::appendFileList(CSList **flist, TCHAR *fname, TCHAR ftype, CFileTime *pFileTime, unsigned int nSize)
{
	CSList *fList = *flist;
	if (!fList) {
		fList = new CSList();
		if (!fList) return false;
		if (!fList->alloc(UNIT_FILE_LIST, eAlloc_Type_alloc)) return false;
		*flist = fList;
	}

	STFileInfoEx *pInfo = (STFileInfoEx *)calloc(1,sizeof(STFileInfoEx));
	
	memcpy(&pInfo->stFileTime, pFileTime, sizeof(CFileTime));
	pInfo->fname = gs_pMMgr->newString(fname);
	pInfo->stat = ftype;
	pInfo->nSize = nSize;

	// add
	if (fList->push_back(pInfo)) {
		gs_cLogger.PutLogQueue(LEVEL_TRACE, _T("fname[%s] nCount[%d]"), pInfo->fname, fList->size());
		return true;
	}
	else {
		gs_cLogger.DebugLog(LEVEL_ERROR, _T(" fname[%d] ftype[%d]"), fname, ftype);
		return false;
	}
}

void CFileUtil::destroyFileList(STFileList *pResult)
{
	int nIdx = 0;
	STFileInfoEx *pFileInfo = (STFileInfoEx *)pResult->pList->getNext(&nIdx);
	while (pFileInfo) {
		gs_pMMgr->delString(pFileInfo->fname);
		pResult->pList->del(nIdx); // deep delete
		nIdx++;
		pFileInfo = (STFileInfoEx *)pResult->pList->getNext(&nIdx);
	}		
	free(pResult->pList);
	pResult->pList = NULL;
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

bool CFileUtil::fileToFieldRecord(TCHAR *pFilePath, CSList *pLineList, TCHAR ch)
{
	// FILE READ
	//TCHAR *pFileBuf = NULL;
	int fileSize = 0;
	CStringList *pList;
	bool bres;
	STStrInfo stInfo;
	STBuf szFileBuf;

	memset(&stInfo, 0, sizeof(STStrInfo));
	memset(&szFileBuf, 0, sizeof(STBuf));
	fileSize = readNalloc(pFilePath, &szFileBuf);

	if (!fileSize) {
		bres = false;
		goto END_PROC;
	}

	stInfo.aBufSize = 4096;
	stInfo.pSource = szFileBuf.pValue;
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
		pList = (CStringList *)pLineList->getNext(&idx);
		while (pList) {
			delete pList;
			idx++;
			pList = (CStringList *)pLineList->getNext(&idx);
		}
	}
	bres = false;
END_PROC:
	if (szFileBuf.pValue) gs_pMMgr->delBuf(&szFileBuf);
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

//int CFileUtil::readNalloc(TCHAR *pFilePath, TCHAR **pBuf, int *aBufSize)
//{
//	FILE *pFile = _tfopen(pFilePath, _T("rb"));
//	int nSize = 0, nRead, nPos=0, nBufSize;
//
//	if (!pFile) {
//		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CDFSFile::read pFile open FAIL [%s]"), pFilePath);
//		return 0;
//	}
//	fseek(pFile, 0, SEEK_END);
//	nSize = ftell(pFile);
//	fseek(pFile, 0, SEEK_SET);
//
//	nBufSize = nSize + 1;
//	if (*aBufSize < nBufSize) {
//		if (*aBufSize) {
//			gs_pMMgr->delBuf(*pBuf, *aBufSize);
//		}
//		*pBuf = gs_pMMgr->newBuf(nBufSize);
//		if (!*pBuf) {
//			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CFileUtil::readNalloc malloc error! size[%ld]"), nBufSize);
//			return 0;
//		}
//		*aBufSize = nBufSize;
//	}
//
//	while ((nRead = (int)fread(*pBuf + nPos, 1, nSize - nPos, pFile)) > 0)
//	{
//		nPos += nRead;
//	}
//
//	if (!nPos) {
//		printf("fread errno[%d]\n", errno);
//	}
//	(*pBuf)[nPos] = 0;
//	fclose(pFile);
//	return nSize;
//}

#define MAX_FREAD_BYTES 65536
unsigned int CFileUtil::readNalloc(TCHAR *pFilePath, STBuf *pBuf, unsigned int nSize)
{
	FILE *pFile = _tfopen(pFilePath, _T("rb"));
	size_t nRead, nPos = 0;
	int nIdx;

	if (!pFile) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("_tfopen FAIL! [%s] error[%s]"), pFilePath, strerror(errno));
		return 0;
	}
	if (!nSize) {
		fseek(pFile, 0, SEEK_END);
		nSize = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);
	}
	nIdx = getIndex(nSize + 1);

	if (pBuf->nIdx < nIdx) {
		if (pBuf->pValue) {
			gs_pMMgr->delBuf(pBuf);
		}
		pBuf->pValue = gs_pMMgr->newBufByIndex(nIdx);
		pBuf->nIdx = nIdx;
	}

	while ((nRead = fread(pBuf->pValue + nPos, 1, nSize - nPos, pFile)) > 0)
	{
		nPos += nRead;
	}

	//while (nPos < nSize)
	//{
	//	nRemain = nSize - nPos;
	//	if (MAX_FREAD_BYTES < nRemain) nReadUnit = MAX_FREAD_BYTES;
	//	else nReadUnit = nRemain;
	//	nRead  = fread(pBuf->pValue + nPos, 1, nReadUnit, pFile);
	//	if (!nRead) {
	//		break;
	//	}
	//	nPos += nRead;
	//}
	if (nPos != nSize) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("[%u/%u] fread FAIL! [%s] error[%s]"), nPos, nSize, pFilePath, strerror(errno));
	}

	pBuf->pValue[nPos] = 0;
	fclose(pFile);
	return (int)nPos;
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
		if (!pNextLine) return 0;
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

int CFileUtil::getNextLine(STLine *pLine)
{
	int nSize = 0, nIdx=0;
	TCHAR *pNextLine = pLine->pSource + pLine->nSkipLen;
	bool bEnd = false;

	if (pLine->pEnd) pNextLine = ehstrstr(pNextLine, pLine->pEnd);
	else pNextLine = _tcschr(pNextLine, g_rc);

	if (pNextLine) {
		nSize = (int)(pNextLine - pLine->pSource);
		nIdx = getIndex(nSize+1);
		pNextLine++;
	}
	else {
		pNextLine = _tcschr(pLine->pSource, 0);
		if (!pNextLine) return 0;
		nSize = (int)(pNextLine - pLine->pSource);
		if (!nSize) return 0;
		bEnd = true;
	}

	if (pLine->m_szLineBuf.nIdx < nIdx) {
		if (pLine->m_szLineBuf.nIdx) {
			gs_pMMgr->delBuf(&pLine->m_szLineBuf);
		}
		nIdx++; // Allocating enough space
		pLine->m_szLineBuf.pValue = gs_pMMgr->newBufByIndex(nIdx);
		if (!pLine->m_szLineBuf.pValue) {
			gs_cLogger.PutLogQueue(LEVEL_ERROR, _T("CFileUtil::getNextLine gs_pMMgr->newBuf error! nIdx[%d] size[%ld]"), nIdx, nSize+1);
			return 0;
		}
		pLine->m_szLineBuf.nIdx = nIdx;
	}

	memcpy(pLine->m_szLineBuf.pValue, pLine->pSource, nSize);
	pLine->m_szLineBuf.pValue[nSize] = 0;
	if (bEnd) return 0;
	if(!pNextLine[0]) return 0;
	pLine->pSource = pNextLine;
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