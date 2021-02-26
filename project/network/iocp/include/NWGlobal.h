/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

#pragma once

#include "comLogger.h"

#define MINIMUM_EVENT		100
#define MAX_ADDRESS_LEN		256
#define MAX_IP_LEN			30
#define ADD_CLEINT_COUNT	20
#define ADD_LIST_COUNT		10

extern int g_nRecvBufSize;
extern int g_nSendDataCount;


enum {
	OP_NONE		= 0,
	OP_ACCEPT	,
	OP_SEND		,
	//OP_SENDCOPLETE,
	OP_RECV,
	OP_CLOSE	,
	OP_EVENT	,
	OP_ERROR	,
	OP_MAX		
};

static const char opcode_desc[OP_MAX][40] = 
{
	{"OP_NONE"},
	{"OP_ACCEPT"},
	{"OP_SEND"},
	//{"OP_SENDCOPLETE" },
	{"OP_RECV"},
	{"OP_CLOSE"},
    {"OP_EVENT"},
	{"OP_ERROR"}
};

enum {
	CERROR_NONE		= 0	,
	CERROR_CREATE_SOCKET		,
	CERROR_BIND			,
	CERROR_LISTEN		,
	CERROR_BINDACCEPT	,
	CERROR_ACCEPT		,
	CERROR_SENDRECV		,	// send , recv result len is 0
	CERROR_PQCS			,
	CERROR_REGIOCP		,
	CERROR_DISCONNECTEX	,
	CERROR_NOEVENTMEM	,	// no exist event mem pool
	CERROR_LESSRECV		,	// less than specific len
	CERROR_RECVBUFLACK	,	// too a lot than buffer size
	CERROR_NOMEMORY		,	// not enough memory pool 
	CERROR_NOTENOUGH	,	// not enough to initalize memory pool 
    CERROR_MAX_IO_COUNT ,
	CERROR_DEL_MEMORY,
	CERROR_NEW_MEMORY,
	CERROR_INSUFFICIENT_BUFFER,
	CERROR_ALREADY_ALLOC,
	CERROR_ALREADY_SOCKET,
	CERROR_ALREADY_CLOSE,
	CERROR_MAX
};
#define RESULT_COMPLETE 50	// define num is must bigger than CERROR_MAX


static const char err_desc_socket[CERROR_MAX][36] {
	{"CERROR_NONE"}			,
	{"CERROR_CREATE_SOCKET"}		,
	{"CERROR_BIND" }		,
	{"CERROR_LISTEN" }		,
	{"CERROR_BINDACCEPT"}	,
	{"CERROR_ACCEPT"}		,
	{"CERROR_SENDRECV"}		,
	{"CERROR_PQCS"}			,
	{"CERROR_REGIOCP"}		,
	{"CERROR_DISCONNECTEX"}	,
	{"CERROR_NOEVENTMEM"}	,
	{"CERROR_LESSRECV"}		,
	{"CERROR_RECVBUFLACK"}	,
	{"CERROR_NOMEMORY"}		,
	{"CERROR_NOTENOUGH"}    ,
	{"CERROR_MAX_IO_COUNT"}	,
	{"CERROR_DEL_MEMORY"}	,
	{"CERROR_NEW_MEMORY"}	,
	{"CERROR_INSUFFICIENT_BUFFER"},
	{"CERROR_ALREADY_ALLOC"},
	{"CERROR_ALREADY_SOCKET" },
	{"CERROR_ALREADY_CLOSE" },
};
static const char g_sAnyIp[16]{ "INADDR_ANY" };

enum {
	SOCKETTYPE_SERVER = 1,
	SOCKETTYPE_CLIENT
};

enum {
	DISCONNECT_NORMAL =1,
	DISCONNECT_TIMEOUT,
	DISCONNECT_ERROR,
	DISCONNECT_MAX
};
static const char close_desc[DISCONNECT_MAX][36]{
	{ "DISCONNECT_NONE" }			,
	{ "DISCONNECT_NORMAL" }			,
	{ "DISCONNECT_TIMEOUT" }		,
	{ "DISCONNECT_ERROR" }		,
};

#pragma pack ( push, 1 )

struct STOVERLAPPED {
	WSAOVERLAPPED	sOverlapped; 
	int opCode;
};


struct StFuncData {
    int  isProcessing;
    long milliSecondsInterval; // 밀리세컨드 인터벌
	void *pData;
    void (*eventProcess) (void *);
};

typedef struct {
	int			nSIdx;
	int			nType;
	int			nDetail;
	int			nFlag;			// DISCONNECT_NORMAL : 종료 요청,  DISCONNECT_ERROR : 소켓 끊김 감지
	void *pObj;
} StDisconInfo;

#pragma pack (pop)


inline void errorLogWrite(int nErrorCode, const TCHAR *pHeader) {
	gs_cLogger.DebugLog(LEVEL_ERROR, _T("%s %s"), pHeader, err_desc_socket[nErrorCode]);
}

