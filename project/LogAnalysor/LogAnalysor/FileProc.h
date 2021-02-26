#pragma once

#include "Handler.h"

class CFileProc
{
public : 
	CFileProc();
	~CFileProc();

	bool init(char *pReadFilePath, unsigned int nSize);
	void FileProc();
	void FileProc2();

	inline int getFileSize() { return m_nFileSize; }
private : 
	unsigned int m_nFileSize;
	STBuf m_szFileBuf;
	STLine m_szLine;
	char *m_pFilePathName;
};