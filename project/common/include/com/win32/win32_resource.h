
#ifndef _LIB_UNIX_RESOURCE_H
#define _LIB_UNIX_RESOURCE_H


namespace common {
/*#undef unix*/
namespace win32 {

	int get_cpu_usage_percent();
	int get_storage_usage_percent(char * dev_path);
	int get_mem_info(unsigned long *totMem, unsigned long *usedMem);

} // namespace unix

} // namespace common

#endif // _LIB_UNIX_RESOURCE_H

