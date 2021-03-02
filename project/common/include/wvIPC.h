#ifndef	_WVIPC_H_
#define	_WVIPC_H_

#include "wvcom.h"

class WVCOM_API WVIPC {
public :
	static  int	CreateMQ(int key, int creatFlag);	// 1 : Create
	static	int	DestroyMQ(int nMQId);
	static	int	GetMQ(int nQID, long nMsType, char *pBuf, int nBLen); 
	static	int	GetMQWithType(int nQID, long *nMsType, char *pBuf, int nBLen); 
	static	int	PutMQ(int nQId, long nMsgType, char *pBuf, int SBLen);
	static	void	*CreateSM(int key, int nSize, int creatFlag, int *shID);	// 1 : Create
	static	int	DestroySM(int nSMId, int destroyFlag = 0, void *pBuf=NULL);
        static  int     CreateSEM(int key, int createFlag, int nCount = 1);
        static  int     DestroySEM(int nSEMId);
        static  int     GetSEMValue(int nSEMId, int nSemNo = 0);
        static  int     PutSEMValue(int nSEMId, int nValue, int nSemNo = 0);
	static	int	EnterSEM(int nSEMId, int nSemNo = 0);
	static	int	LeaveSEM(int nSEMId, int nSemNo = 0);
};

#endif	//_WVIPC_H_
