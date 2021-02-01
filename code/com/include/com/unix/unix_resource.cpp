#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/vtimes.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>

#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <math.h>

#include <time.h>
#include <fcntl.h>
#include <ctype.h>



#include "unix_resource.h"

static inline char *
skip_ws(const char *p)
{
    while (isspace(*p)) p++;
    return (char *)p;
}
    
static inline char *
skip_token(const char *p)
{
    while (isspace(*p)) p++;
    while (*p && !isspace(*p)) p++;
    return (char *)p;
}

static void
xfrm_cmdline(char *p, int len)
{
    while (--len > 0)
    {
	if (*p == '\0')
	{
	    *p = ' ';
	}
	p++;
    }
}

int common::unix_na::get_usage_percent(char * dev_path)
{
	unsigned long long total=0, free = 0;
	double usage = 0;
	struct statvfs sfs;
	if ( statvfs ( dev_path, &sfs) != -1 ) {
		total = (unsigned long long)sfs.f_bsize * sfs.f_blocks;
		free = (unsigned long long)sfs.f_bsize * sfs.f_bfree;
	}
	usage = (total - free) * 100;
	usage = usage/total;
	int res = ceil(usage);
	return res;
}

int common::unix_na::get_mem_info(unsigned long *totMem, unsigned long *usedMem)
{
	char buffer[4096];
	int fd, len;
	char *p;
	unsigned long memtotal = 0;
	unsigned long memfree = 0;
	unsigned long cached = 0;
	unsigned long buffers = 0;
	int line = 0;
	long nRes;

	/* get system wide memory usage */
	if ((fd = open("/proc/meminfo", O_RDONLY)) != -1)
	{
		if ((len = read(fd, buffer, sizeof(buffer) - 1)) > 0)
		{
			buffer[len] = '\0';
			p = buffer - 1;

			/* iterate thru the lines */
			while (p != NULL)
			{
				p++;
				if (p[0] == ' ' || p[0] == '\t')
				{
					/* skip */
				}

				switch (line)
				{
					case 0: 
					{
						if (strncmp(p, "MemTotal:", 9) == 0) {
							p = skip_token(p);
							memtotal = strtoul(p, &p, 10);
						}
						break;
					}
					case 1:
					{
						if (strncmp(p, "MemFree:", 8) == 0) {
							p = skip_token(p);
							memfree = strtoul(p, &p, 10);
						}
						break;
					}
					case 2: break;
					case 3:
					{
						if (strncmp(p, "Buffers:", 8) == 0) {
							p = skip_token(p);
							buffers = strtoul(p, &p, 10);
						}
						break;
					}
					case 4:
					{
						if (strncmp(p, "Cached:", 7) == 0) {
							p = skip_token(p);
							cached = strtoul(p, &p, 10);
						}
						p = NULL; 
						break;
					}
				}

				/* move to the next line */
				if (p) {
					p = strchr(p, '\n');
					line++;
				}
			}
		}
		close(fd);
	}
	*totMem = memtotal;

	nRes = memtotal - (memfree + cached + buffers);
	*usedMem = nRes;

	return 1;
}


//######################################################################
// Network
//######################################################################
#define	 MAX_NET_INTERFACE 8
#define  BUFF_SIZE   1024
int g_nCount;
char g_szInterfaceName[160];
unsigned long long g_rx_bytes_before[MAX_NET_INTERFACE];
unsigned long long g_tx_bytes_before[MAX_NET_INTERFACE];
unsigned long long g_rx_bytes_current[MAX_NET_INTERFACE];
unsigned long long g_tx_bytes_current[MAX_NET_INTERFACE];

unsigned long long g_rx_bytes[MAX_NET_INTERFACE];
unsigned long long g_tx_bytes[MAX_NET_INTERFACE];

//char g_szBuf[BUFF_SIZE];

void printInfo(char *pCmd, char *pResult)
{
	FILE *fp;
	//printf("pCmd : %s\n", pCmd);

	fp = popen(pCmd, "r");
	if (NULL == fp)
	{
		perror("popen() FAIL");
		return;
	}

	//while (fgets(pResult, BUFF_SIZE, fp)) {
	//	printf("%s", pResult);
	//}
	fread(pResult, BUFF_SIZE, sizeof(char), fp);

	pclose(fp);
}

int common::unix_na::initInterface()
{
	char szCmd[512], szResult[1024];
	char *p1, *p2, *pNameList = g_szInterfaceName;
	int nCount = 0, i;
	strcpy(szCmd, "ls /sys/class/net");
	printInfo(szCmd, szResult);

	p2 = szResult;
	while (*p2) {
		p1 = p2;
		while (*p2 != '\n') p2++;

		if (!*p2) break;
		*p2 = 0; p2++;

		if (!strcmp(p1, "lo") || !strncmp(p1, "vir", 3)) continue;

		strcpy(pNameList, p1);
		//printf("name[%s]\n", pNameList);
		pNameList += strlen(pNameList) + 1;
		nCount++;
	}

	g_nCount = nCount;

	pNameList = g_szInterfaceName;
	printf("g_nCount : %d \n", g_nCount);
	for (i = 0; i < g_nCount; i++) {
		printf("name[%s]\n", pNameList);
		pNameList += strlen(pNameList) + 1;
	}

	return g_nCount;
}

void common::unix_na::set_network_info1()
{
	char szCmd[512], szResult[1024];
	int i;
	char *pNameList = g_szInterfaceName;
	for (i = 0; i < g_nCount; i++) {
		sprintf(szCmd, "cat /sys/class/net/%s/statistics/rx_bytes", pNameList);
		printInfo(szCmd, szResult);
		g_rx_bytes_before[i] = atoll(szResult);
		//printf("g_rx_bytes_before[%d]:%llu\n", i, g_rx_bytes_before[i]);
		sprintf(szCmd, "cat /sys/class/net/%s/statistics/tx_bytes", pNameList);
		printInfo(szCmd, szResult);
		g_tx_bytes_before[i] = atoll(szResult);
		//printf("g_tx_bytes_before[%d]:%llu\n", i, g_tx_bytes_before[i]);
		pNameList += strlen(pNameList) + 1;
	}
}

void common::unix_na::set_network_info2()
{
	char szCmd[512], szResult[1024];
	int i;
	char *pNameList = g_szInterfaceName;
	for (i = 0; i < g_nCount; i++) {
		sprintf(szCmd, "cat /sys/class/net/%s/statistics/rx_bytes", pNameList);
		printInfo(szCmd, szResult);
		g_rx_bytes_current[i] = atoll(szResult);
		//printf("g_rx_bytes_current[%d]:%llu\n", i, g_rx_bytes_current[i]);
		sprintf(szCmd, "cat /sys/class/net/%s/statistics/tx_bytes", pNameList);
		printInfo(szCmd, szResult);
		g_tx_bytes_current[i] = atoll(szResult);
		//printf("g_tx_bytes_current[%d]:%llu\n", i, g_tx_bytes_current[i]);
		pNameList += strlen(pNameList) + 1;

		g_rx_bytes[i] = g_rx_bytes_current[i] - g_rx_bytes_before[i];
		g_tx_bytes[i] = g_tx_bytes_current[i] - g_tx_bytes_before[i];
	}
}

void common::unix_na::get_network_info(int idx, char **pName, unsigned long long *pRXBytes, unsigned long long *pTXBytes)
{
	int i;
	char *pNameList = g_szInterfaceName;
	for (i = 0; i < idx; i++) {
		pNameList += strlen(pNameList) + 1;
	}

	*pName = pNameList;
	*pRXBytes = g_rx_bytes[idx];
	*pTXBytes = g_tx_bytes[idx];

	printf("*pRXBytes : %llu \n", *pRXBytes);
	printf("*pTXBytes : %llu \n", *pTXBytes);
}

int common::unix_na::getConnectionCount(int nPort)
{
	char szCmd[512], szResult[1024];
	// netstat -n | grep .80 | grep EST | wc -l
	int nCount = 0;
	sprintf(szCmd, "netstat -n | grep .%d | grep EST | wc -l", nPort);
	printInfo(szCmd, szResult);
	nCount = atoi(szResult);
	return nCount;
}




//######################################################################
// CPU
//######################################################################

static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

void common::unix_na::initCPU_machine() {
	FILE* file = fopen("/proc/stat", "r");
	fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
		&lastTotalSys, &lastTotalIdle);
	fclose(file);
}

int common::unix_na::getUsageCPU_machine() {
	double percent;
	FILE* file;
	unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

	file = fopen("/proc/stat", "r");
	fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
		&totalSys, &totalIdle);
	fclose(file);


	if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
		totalSys < lastTotalSys || totalIdle < lastTotalIdle) {
		//오버플로우 detection
		percent = -1.0;
	}
	else {
		total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
			(totalSys - lastTotalSys);
		percent = total;
		total += (totalIdle - lastTotalIdle);
		percent /= total;
		percent *= 100;
	}

	lastTotalUser = totalUser;
	lastTotalUserLow = totalUserLow;
	lastTotalSys = totalSys;
	lastTotalIdle = totalIdle;

	return (int)percent;
}




static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;

void common::unix_na::initCPU_process() {
	FILE* file;
	struct tms timeSample;
	char line[128];

	lastCPU = times(&timeSample);
	lastSysCPU = timeSample.tms_stime;
	lastUserCPU = timeSample.tms_utime;

	file = fopen("/proc/cpuinfo", "r");
	numProcessors = 0;
	while (fgets(line, 128, file) != NULL) {
		if (strncmp(line, "processor", 9) == 0) numProcessors++;
	}
	fclose(file);
}


int common::unix_na::getUsageCPU_process() {
	struct tms timeSample;
	clock_t now;
	double percent;

	now = times(&timeSample);
	if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
		timeSample.tms_utime < lastUserCPU) {
		//detect overflow
		percent = -1.0;
	}
	else {
		percent = (timeSample.tms_stime - lastSysCPU) +
			(timeSample.tms_utime - lastUserCPU);
		percent /= (now - lastCPU);
		percent /= numProcessors;
		percent *= 100;
	}
	lastCPU = now;
	lastSysCPU = timeSample.tms_stime;
	lastUserCPU = timeSample.tms_utime;

	return (int)percent;
}