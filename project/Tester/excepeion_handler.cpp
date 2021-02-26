#include "exception_handler.h"

exception_handler *volatile exception_handler::_instantce = nullptr;
std::mutex exception_handler::_mutex;

DWORD exception_handler::initialize(__in LPCTSTR dump_file_name, __in const MINIDUMP_TYPE dump_type)
{
	DWORD error = ERROR_SUCCESS;

	do {
		if (nullptr == dump_file_name) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}

		_dump_file_name.assign(dump_file_name);
		_dump_type = dump_type;
	} while (false);
	return error;
}

DWORD exception_handler::run()
{
	_prev_filter = ::SetUnhandledExceptionFilter(exception_callback);
	return ERROR_SUCCESS;
}

LONG exception_handler::exception_callback(__in struct _EXCEPTION_POINTERS* exceptioninfo)
{
	do {
		if (nullptr == exceptioninfo) {
			break;
		}

		SYSTEMTIME st = { 0 };
		::GetLocalTime(&st);

		std::string dump_file_name;
		dump_file_name.assign(exception_handler::instance()->_dump_file_name);

		// create dump file
		HANDLE dump_file_handle = ::CreateFile(dump_file_name.c_str(), 
			GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (INVALID_HANDLE_VALUE == dump_file_handle) {
			break;
		}

		MINIDUMP_EXCEPTION_INFORMATION ex_info = { 0 };

		ex_info.ThreadId = ::GetCurrentThreadId();
		ex_info.ExceptionPointers = exceptioninfo;
		ex_info.ClientPointers = FALSE;

		//write dump file
		if (FALSE == ::MiniDumpWriteDump(
			::GetCurrentProcess(), ::GetCurrentProcessId(), dump_file_handle, exception_handler::instance()->_dump_type,
			&ex_info, nullptr, nullptr)
			) 
		{
			break;
		}
	} while (false);

	return(exception_handler::instance()->_prev_filter) ? exception_handler::instance()->_prev_filter(exceptioninfo) : EXCEPTION_EXECUTE_HANDLER;
}