#include	<windows.h>
#include	<stdio.h>
#include	<string.h>
#include	"../../jinIPC.h"

#define	MAX_QUEUE_COUNT	20
#define	MAX_LIMIT		65000
#define	MAX_ITEM	500


#pragma pack(1)
typedef	struct {
	int	nQKey;
	HANDLE	hFile;
	LPCTSTR	pBuf;
} WinMsgQueueInfo;

typedef	struct {
	int	nUse;
	int	nFront;
	int	nRear;
} WinMsgQueueHead;

typedef	struct {
	int nMsgType;
	int	nDataLen;
	int	nOffset;
} WinMsgDataHead;



#define MQ_HEADER_LEN	( sizeof(WinMsgQueueHead) + (sizeof(WinMsgDataHead) * MAX_ITEM))

#define	DATA_BUF_LEN(pPull, nPos)	*(int *)(pPull + nPos + sizeof(int))
#define	NEXT_DATA_POS(nBase, nBLen)	(nBase + nBLen + sizeof(int) * 2)

static	WinMsgQueueInfo	SimMQList[MAX_QUEUE_COUNT];
static	int		SimMQCount = 0;

static	int	NewMsgDataPos(char *pPull, int nLen, char *pData)
{
	int	nPos = 0, nNext = 0;
	int	nBLen;

	while (nPos < (MAX_LIMIT - MQ_HEADER_LEN)) {
		nBLen = DATA_BUF_LEN(pPull, nPos);

		if (*(int *)(pPull + nPos) == 0) {		// Empty
			nNext = NEXT_DATA_POS(nPos, nBLen);
			if (nNext < (MAX_LIMIT - MQ_HEADER_LEN) && *(int *)(pPull + nNext) == 0) {
				nBLen = DATA_BUF_LEN(pPull, nNext);
				DATA_BUF_LEN(pPull, nPos) += nBLen + sizeof(int) * 2;
			}

			nBLen = DATA_BUF_LEN(pPull, nPos);

			if (nBLen > nLen) {
				*(int *)(pPull + nPos) = 1;		// In Use
				memcpy(pPull + nPos + sizeof(int) * 2, pData, nLen);
				// offset과 다음 길이 정보가 들어갈 헤더까지 포함
				//if (nBLen > nLen * 2) {
				if (nBLen > nLen + sizeof(int) * 2) {
					DATA_BUF_LEN(pPull, nPos) = nLen;

					nNext = nPos + nLen + sizeof(int) * 2;
					*(int *)(pPull + nNext) = 0;
					*(int *)(pPull + nNext + sizeof(int)) = nBLen - nLen - sizeof(int) * 2;
				}
				return nPos;
			}
		}

		nPos += nBLen + sizeof(int) * 2;
	}

	return -1;
}


static	int	GetMsgDataPos(char *pPull, int nOffset, int nBLen, char *pData)
{
	int	nLen;
	int	nNext;

	if (nOffset > MAX_LIMIT - MQ_HEADER_LEN)
		return 0;

	nLen = DATA_BUF_LEN(pPull, nOffset);

	if (*(int *)(pPull + nOffset) == 0)
		return 0;

	nNext = NEXT_DATA_POS(nOffset, nLen);
	//printf("Offset :%d, nLen : %d, nNext : %d\n", nOffset, nLen, nNext);

	if (nBLen < nLen)
		nLen = nBLen;

	memcpy(pData, pPull + nOffset + sizeof(int) * 2, nLen);

	*(int *)(pPull + nOffset) = 0;
	if (*(int *)(pPull + nNext) == 0) {
		nBLen = DATA_BUF_LEN(pPull, nNext);
		DATA_BUF_LEN(pPull, nOffset) += nBLen + sizeof(int) * 2;
	}

	return nLen;
}

int	
CIPC::CreateMQ(int	key, int createFlag)
{
	int	*pDataBuf, nHeadLen;
	WinMsgQueueHead	*pQHead;

	if (SimMQCount >= MAX_QUEUE_COUNT) {
		printf("Can't create Message Queue : MaxCount : %d\n", SimMQCount);
		return -1;
	}
	SimMQList[SimMQCount].pBuf = (LPCTSTR)CreateSM(key, MAX_LIMIT, createFlag, (int *)&SimMQList[SimMQCount].hFile);
	if (SimMQList[SimMQCount].pBuf) {
		if (createFlag) {
			memset((char *)SimMQList[SimMQCount].pBuf, 0, MAX_LIMIT);
			pQHead = (WinMsgQueueHead *)SimMQList[SimMQCount].pBuf;
			pQHead->nFront = -1;
			nHeadLen = MQ_HEADER_LEN;
			pDataBuf = (int *)( SimMQList[SimMQCount].pBuf + nHeadLen);
			*pDataBuf = 0;
			pDataBuf ++;
			*pDataBuf = MAX_LIMIT - nHeadLen - sizeof(int);
		}
		SimMQList[SimMQCount].nQKey = key;
		SimMQCount ++;
		return (SimMQCount - 1) | 0x8000;
	}
	else {
		SimMQList[SimMQCount].hFile = 0;
		SimMQList[SimMQCount].nQKey = -1;
        return -1;
	}
}

int
CIPC::DestroyMQ(int nMQId)
{
	nMQId &= 0x7fff;
	if (nMQId < 0 || nMQId > MAX_QUEUE_COUNT || SimMQList[nMQId].hFile == NULL)
			return 0;

	DestroySM((int)SimMQList[nMQId].hFile, false, (void *)SimMQList[nMQId].pBuf);
	SimMQList[nMQId].hFile = NULL;
	SimMQList[nMQId].pBuf = NULL;
	SimMQList[nMQId].nQKey = -1;
	if (nMQId+1 >= SimMQCount)
		SimMQCount = nMQId + 1;

	return 1;
}

int
CIPC::GetMQ(int nQID, long nMsgType, char *pBuf, int nBLen)
{
	WinMsgQueueHead	*pQHead;
	WinMsgDataHead	*pDataHead;
	char	*pDataBuf;
	int	i, nRLen = 0;
	bool	bGet = false;
	int		nPos, nLast;
	DWORD	dwWait = 0;
	int		nRet = 1;
//	char	szTmp[10];
	TCHAR	szQKey[64];
	HANDLE	hMutex;

	nQID &= 0x07fff;
	if (nQID < 0 || nQID > MAX_QUEUE_COUNT || SimMQList[nQID].pBuf == NULL)
			return 0;

	pQHead = (WinMsgQueueHead *)SimMQList[nQID].pBuf;
	_stprintf(szQKey, _T("Global\\MQ%d"), SimMQList[SimMQCount].nQKey);
	hMutex = CreateMutex(NULL, FALSE, szQKey);
	if (hMutex == NULL) {
		_tprintf(_T("Mutext Create Error! [Key : %s, Error : 0x%x\n"), szQKey, GetLastError());
		return 0;
	}
	else if (GetLastError() != 0) {
		_tprintf(_T("Mutext Already Exist! [Key : %s, Error : 0x%x : 0x%x\n"), szQKey, GetLastError(), ERROR_ALREADY_EXISTS);
		CloseHandle(hMutex);
		return 0;
	}

	pQHead->nUse = 1;
	nPos = pQHead->nFront + 1;
	if (nPos == MAX_ITEM)
		nPos = 0;

	if (nPos == pQHead->nRear) {	// Queue Empty
		pQHead->nUse = 0;
//		printf("Queue Empty [%d:%d]\n", nPos, pQHead->nRear);
		//return 0;
		nRet = 0;
	}
	else
	{
		if (nPos > pQHead->nRear)
			nLast = MAX_ITEM;
		else
			nLast = pQHead->nRear;

//		printf("======= Start : %d -> %d\n", nPos, pQHead->nRear);
		pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead) + (sizeof(WinMsgDataHead) * nPos));
		pDataBuf = (char *)(SimMQList[nQID].pBuf + MQ_HEADER_LEN);
		for (i = nPos; i < nLast; i++) {
			if (pDataHead->nDataLen > 0 && (nMsgType == 0 || pDataHead->nMsgType == nMsgType)) {
				bGet = true;
				break;
			}
			pDataHead ++;
		}

		if (!bGet) {
			if (nLast == MAX_ITEM) {
				pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead));
				for (i = 0; i < pQHead->nRear; i++) {
					if (pDataHead->nDataLen > 0 && (nMsgType == 0 || pDataHead->nMsgType == nMsgType)) {
						bGet = true;
						break;
					}
					pDataHead ++;
				}
			}
		}

		if (!bGet) {
			pQHead->nUse = 0;
			CloseHandle(hMutex);
			return 0;
		}

		nRLen = GetMsgDataPos(pDataBuf, pDataHead->nOffset, nBLen, pBuf);
		if (nRLen > pDataHead->nDataLen)
			nRLen = pDataHead->nDataLen;

		if ((pQHead->nFront + 1)%MAX_ITEM == i) {
			pQHead->nFront = i;

			if (i > pQHead->nRear)
				nLast = MAX_ITEM;
			else
				nLast = pQHead->nRear;

			pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead) + (sizeof(WinMsgDataHead) * i));
			for (; i < nLast; i++) {
				if (pDataHead->nDataLen > 0)
					break;
				pDataHead ++;
			}
			if (i == nLast && (i+1) == MAX_ITEM) {
				pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead));
				for (i = 0; i < pQHead->nRear; i++) {
					if (pDataHead->nDataLen > 0)
						break;
					pQHead->nFront = i;
					pDataHead ++;
				}
			}
		}
		else {
			pDataHead->nDataLen = 0;
		}
		pQHead->nUse = 0;
		//printf("GETMQ : %d[%d -> %d]\n", nRLen, pQHead->nFront, pQHead->nRear);
	}
	CloseHandle(hMutex);
	return nRLen;
}

int	
CIPC::PutMQ(int nQID, long nMsgType, char *pBuf, int nBLen)
{
	WinMsgQueueHead	*pQHead;
	WinMsgDataHead	*pDataHead;
	char	*pDataBuf;
	int nPos;
	DWORD	dwWait = 0;
	TCHAR	szQKey[64];
	HANDLE	hMutex;
	int		nCount = 500;

	if (nBLen <= 0)
		return 0;

	nQID &= 0x07fff;
	if (nQID < 0 || nQID > MAX_QUEUE_COUNT || SimMQList[nQID].pBuf == NULL)
			return -1;

	pQHead = (WinMsgQueueHead *)SimMQList[nQID].pBuf;
	_stprintf(szQKey, _T("Global\\MQ%d"), SimMQList[SimMQCount].nQKey);
	
	while (nCount > 0) {
		hMutex = CreateMutex(NULL, FALSE, szQKey);
		if (hMutex == NULL) {
			_tprintf(_T("Mutext Create Error! [Key : %s, Error : 0x%x\n"), szQKey, GetLastError());
			return -1;
		}
		else if (GetLastError() != 0) {
			_tprintf(_T("Mutext Already Exist! [Key : %s, Error : 0x%x : 0x%x\n"), szQKey, GetLastError(), ERROR_ALREADY_EXISTS);
			CloseHandle(hMutex);
			//Sleep(100);
			nCount --;
            continue;
		}
		else
			break;
	}
	
	if (nCount <= 0) {
		printf("KEY [%d] PutMQ Timeout\n", SimMQList[SimMQCount].nQKey);
		return -1;
	}

	pQHead->nUse = 1;

	nPos = pQHead->nFront;

	if (nPos == -1 && (pQHead->nRear == MAX_ITEM - 1)) {
		pQHead->nUse = 0;
		CloseHandle(hMutex);
		return -1;
	}


	if (nPos >= 0 && nPos == ((pQHead->nRear+1)%MAX_ITEM)) {		// Queue Full
		pQHead->nUse = 0;
		printf("Message FULL : %x : %d [%d]\n", nQID & 0x8000, MAX_ITEM, pQHead->nFront);
		CloseHandle(hMutex);
		return -1;
	}
	
	pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead) + (sizeof(WinMsgDataHead) * pQHead->nRear));
	pDataBuf = (char *)(SimMQList[nQID].pBuf + MQ_HEADER_LEN);
	pDataHead->nMsgType = nMsgType;
	pDataHead->nOffset = NewMsgDataPos(pDataBuf, nBLen, pBuf);
	if (pDataHead->nOffset == -1) {
		printf("Data Buffer Full!!!\n");
		pQHead->nUse = 0;
		CloseHandle(hMutex);
		return -1;
	}
	pDataHead->nDataLen = nBLen;

	pQHead->nRear ++;
	if (pQHead->nRear >= MAX_ITEM) {
		pQHead->nRear = 0;
	}

	pQHead->nUse = 0;
	//printf("PUTMQ : [%d -> %d] DataOffset : %d, %d\n", pQHead->nFront, pQHead->nRear, pDataHead->nOffset, nBLen);
	CloseHandle(hMutex);

	return 1;
}

void * CIPC::CreateSM(int key, int nSize, int creatFlag, int *shID)
{

	HANDLE	hMapFile;
	TCHAR	szMapObjName[128];
	LPCTSTR	pBuf;

	_stprintf(szMapObjName, _T("FILEMAP_%x"), key);

	if (creatFlag) {
		hMapFile = CreateFileMapping(
				INVALID_HANDLE_VALUE,    // use paging file
				NULL,                    // default security 
				PAGE_READWRITE,          // read/write access
				0,                       // max. object size 
				nSize,						// buffer size  
				szMapObjName);           // name of mapping object
		
		if (hMapFile == NULL || hMapFile == INVALID_HANDLE_VALUE) { 
			_tprintf(_T("Could not create file mapping object (%s : %d).\n"), szMapObjName, GetLastError());
			return NULL;
		}
	}
	else {
		hMapFile = OpenFileMapping(
				FILE_MAP_ALL_ACCESS,   // read/write access
				FALSE,                 // do not inherit the name
				szMapObjName);               // name of mapping object 
 
		if (hMapFile == NULL) { 
			_tprintf(_T("Could not open file mapping object (%s : %d).\n"), szMapObjName, GetLastError());
			return NULL;
		} 
	}
 
	pBuf = (LPTSTR) MapViewOfFile(hMapFile, // handle to map object
				FILE_MAP_ALL_ACCESS,  // read/write permission
				0,                    
				0,                    
				nSize);                   
 
	if (pBuf == NULL) { 
		printf("Could not map view of file (%d).\n", GetLastError()); 
		return NULL;
	}
	*shID = (int)hMapFile;
	return (void *)pBuf;
}

int
CIPC::DestroySM(int nSMId, int destroyFlag, void *pBuf)
{
	int	nRet = 1;

	if (pBuf)
		UnmapViewOfFile(pBuf);

	CloseHandle((HANDLE)nSMId);
	return 1;
}

int
CIPC::GetMQWithType(int nQID, long *nMsgType, char *pBuf, int nBLen)
{
	WinMsgQueueHead	*pQHead;
	WinMsgDataHead	*pDataHead;
	char	*pDataBuf;
	int	i, nRLen;
	bool	bGet = false;
	int		nPos, nLast;
	DWORD	dwWait = 0;
	TCHAR	szQKey[64];
	HANDLE	hMutex;

	nQID &= 0x07fff;
	if (nQID < 0 || nQID > MAX_QUEUE_COUNT || SimMQList[nQID].pBuf == NULL)
			return 0;

	pQHead = (WinMsgQueueHead *)SimMQList[nQID].pBuf;
	_stprintf(szQKey, _T("Global\\MQ%d"), SimMQList[SimMQCount].nQKey);
	hMutex = CreateMutex(NULL, FALSE, szQKey);
	if (hMutex == NULL) {
		_tprintf(_T("Mutext Create Error! [Key : %s, Error : 0x%x\n"), szQKey, GetLastError());
		return 0;
	}
	else if (GetLastError() != 0) {
		_tprintf(_T("Mutext Already Exist! [Key : %s, Error : 0x%x : 0x%x\n"), szQKey, GetLastError(), ERROR_ALREADY_EXISTS);
		CloseHandle(hMutex);
		return 0;
	}

	pQHead->nUse = 1;
	nPos = pQHead->nFront + 1;
	if (nPos == MAX_ITEM)
		nPos = 0;

	if (nPos == pQHead->nRear) {	// Queue Empty
		pQHead->nUse = 0;
		CloseHandle(hMutex);
		return 0;
	}

	if (nPos > pQHead->nRear)
		nLast = MAX_ITEM;
	else
		nLast = pQHead->nRear;

	pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead) + (sizeof(WinMsgDataHead) * nPos));
	pDataBuf = (char *)(SimMQList[nQID].pBuf + MQ_HEADER_LEN);
	for (i = nPos; i < nLast; i++) {
		if (pDataHead->nDataLen > 0 && (*nMsgType == 0 || pDataHead->nMsgType == *nMsgType)) {
			bGet = true;
			break;
		}
		pDataHead ++;
	}

	if (!bGet) {
		if (nLast == MAX_ITEM) {
			pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead));
			for (i = 0; i < pQHead->nRear; i++) {
				if (pDataHead->nDataLen > 0 && (*nMsgType == 0 || pDataHead->nMsgType == *nMsgType)) {
					bGet = true;
					break;
				}
				pDataHead ++;
			}
		}
	}

	if (!bGet) {
		pQHead->nUse = 0;
		CloseHandle(hMutex);
		return 0;
	}

	nRLen = GetMsgDataPos(pDataBuf, pDataHead->nOffset, nBLen, pBuf);
	if (nRLen > pDataHead->nDataLen)
		nRLen = pDataHead->nDataLen;

	*nMsgType = pDataHead->nMsgType;


	//if (pQHead->nFront + 1 == i) {
	if ((pQHead->nFront + 1)%MAX_ITEM == i) {
		pQHead->nFront = i;

		if (i > pQHead->nRear)
			nLast = MAX_ITEM;
		else
			nLast = pQHead->nRear;

		pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead) + (sizeof(WinMsgDataHead) * i));
		for (; i < nLast; i++) {
			if (pDataHead->nDataLen > 0)
				break;
			pDataHead ++;
		}
		if (i == nLast && i == MAX_ITEM) {
			pDataHead = (WinMsgDataHead *)(SimMQList[nQID].pBuf + sizeof(WinMsgQueueHead));
			for (i = 0; i < pQHead->nRear; i++) {
				if (pDataHead->nDataLen > 0)
					break;
				pQHead->nFront = i;
				pDataHead ++;
			}
		}
	}
	else {
		pDataHead->nDataLen = 0;
	}
	pQHead->nUse = 0;
	//printf("GETMQWITHTYPE : %d\n", nRLen);
	CloseHandle(hMutex);
	return nRLen;
}
