/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/
#pragma once

#include "comLogger.h"

#define MINIMUM_EVENT		100
#define MAX_ADDRESS_LEN		256
#define MAX_IP_LEN			30
//#define RECV_BUFFER_SIZE    65536	// 16384 : 16KB
//#define RECV_BUFFER_SIZE    24	// 16384 : 16KB

extern int g_nRecvBufSize;
extern int g_nSendDataCount;


enum {
	OP_NONE		= 0,
	OP_ACCEPT	,
	OP_SEND		,
	OP_RECV		,
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
	{"OP_RECV"},
	{"OP_CLOSE"},
    {"OP_EVENT"},
	{"OP_ERROR"}
};

enum {
	CERROR_NONE		= 0	,
	CERROR_CREATE		,
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
	CERROR_MAX
};
#define RESULT_COMPLETE 50	// define num is must bigger than CERROR_MAX


static const char err_desc_socket[CERROR_MAX][40] = 
{
	{"CERROR_NONE"}			,
	{"CERROR_CREATE"}		,
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
	{"CERROR_MAX_IO_COUNT"},
	{"CERROR_DEL_MEMORY"},
	{"CERROR_NEW_MEMORY"},
	{"CERROR_INSUFFICIENT_BUFFER"},
	{"CERROR_ALREADY_ALLOC"},
};

enum {
	SOCKETTYPE_SERVER = 1,
	SOCKETTYPE_CLIENT
};

enum {
	DISCONNECT_NORMAL =1,
	DISCONNECT_TIMEOUT,
	DISCONNECT_ERROR
};

#pragma pack ( push, 1 )

struct STOVERLAPPED {
	WSAOVERLAPPED	sOverlapped; 
	int opCode;
};

/*
typedef struct _OVERLAPPED_EX {
	STOVERLAPPED stCommon;
	void *pObject;
} OVERLAPPED_EX, *LPOVERLAPPED_EX;
*/
//
//struct StMsgData{
//	int			nSIdx;
//	char		szData[MAX_PACKET_SIZE];
//};

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
} StDisconInfo;

/*
struct stPacketHeader
{
	unsigned int nDataSize;
};

struct stAlivePacket
{
	unsigned int nDataSize;
	unsigned int nMsgIndex;
};
*/
#pragma pack (pop)

extern unsigned int		g_nTick;
extern unsigned int     g_nTimeDifference;  // 전 시간과의 시간차

void UpdateTick();
void UpdateTick(unsigned int nTick);

