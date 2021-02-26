
#ifndef _LIB_UNIX_RESOURCE_H
#define _LIB_UNIX_RESOURCE_H


namespace common {
/*#undef unix*/
namespace unix_na {

	int get_usage_percent(char * dev_path);
	int get_mem_info(unsigned long *totMem, unsigned long *usedMem);

	int initInterface();
	void set_network_info1();
	void set_network_info2();
	void get_network_info(int idx, char **pName, unsigned long long *pRXBytes, unsigned long long *pTXBytes);

	int getConnectionCount(int nPort);


	void initCPU_machine();
	int getUsageCPU_machine();
	void initCPU_process();
	int getUsageCPU_process();
} // namespace unix
} // namespace common

#endif // _LIB_UNIX_RESOURCE_H

