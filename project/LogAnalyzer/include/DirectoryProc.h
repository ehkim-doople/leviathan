#pragma once

struct STFileList;
struct STSortData;

#include "FileProc.h"


class CDirectoryProc
{
public:
	CDirectoryProc();
	~CDirectoryProc();
	bool init();
	bool proc(char *pDir);

private:
	STFileList *m_pFileList;
	void Dreport();
	int getTimeAscendingFileList(char *pDir, STSortData **pResult);
	bool verificationNSetFileProc();

	void (CFileProc::*m_fpFileProc)();
};

extern CDirectoryProc *g_pDProc;