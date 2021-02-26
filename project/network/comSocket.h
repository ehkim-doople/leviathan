/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
2020.05.13 by KEH
-------------------------------------------------------------
Common Interface,
Not Yet finished!!

Function to separate only data by extracting only the data length
*********************************************************************/

#pragma once

#include "types.h"

typedef void(*fp_parsingProcess) (char *pData, int nSize);
extern char *g_pEndDelimeter;
enum E_HEADER_LENGTH_TYPE
{
	ePacket_none = 0,
	ePacket_numberLength = 1,
	ePacket_stringLength,
	ePacket_http, // header 의 시작 : HTTP method(GET, POST, HEAD, PUT, DELETE, TRACE, OPTION, CONNECT), header 의 끝 : 빈줄
};


class CComSocket {
public:
	static E_HEADER_LENGTH_TYPE initConfig_headerParsing(int nIdx = 0);
	static bool startComunicator(int nIdx, fp_parsingProcess function, E_HEADER_LENGTH_TYPE type);
};
