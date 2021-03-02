/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2021 by keh														*/
/*																				*/
/********************************************************************************/

#pragma once

#include <windows.h>
#include <mutex>
#include <iostream>

#pragma warning(push)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(pop)

#pragma comment(lib, "Dbghelp.lib")


class exception_handler {
private : 
	static exception_handler* volatile _instantce;
	static std::mutex _mutex;

	std::string _dump_file_name;
	MINIDUMP_TYPE _dump_type;
	LPTOP_LEVEL_EXCEPTION_FILTER _prev_filter;

	exception_handler() {}
	exception_handler(const exception_handler& other);
	~exception_handler() {}

public :
	static exception_handler* volatile instance() {
		if (_instantce == nullptr) {
			std::lock_guard<std::mutex> lock(_mutex);

			if (_instantce == nullptr) {
				_instantce = new exception_handler();
			}
		}
		return _instantce;
	}

	DWORD initialize(__in LPCTSTR dump_file_name,__in const MINIDUMP_TYPE dump_type = MINIDUMP_TYPE::MiniDumpNormal);
	DWORD run();
	static LONG WINAPI exception_callback(__in struct _EXCEPTION_POINTERS *exceptioninfo);
};