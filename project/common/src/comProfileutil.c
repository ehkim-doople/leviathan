/******************************************************************************/
/*                                                                            */
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.                   */
/*   Copyright 2012 by keh									  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/* *************************************************************
 * FILE : profileutil.c
 * ************************************************************* */

#include "comProfileutil.h"


#ifndef WIN32
#include <pthread.h>
#include "hash.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#define	HASH_SIZE	100
#define	SETTINGS_FNAME	"./default.ini"


pthread_mutex_t		G_CS;
bool			G_RUN = false;

struct _SectEntry {
	char	*name;
	struct hash *h;
};

struct ConfEntry
{
	char *filename;
	time_t	tmRead;
	int	curEntryIdx;
	int	maxEntry;
	struct _SectEntry *sections;
}; 

static	char	*projectHomeDir = NULL;

static bool
_parse_config_entry(ConfEntry *ce);

void	checkCSInfo()
{
	if (G_RUN == false) {
		G_RUN = true;
		if (pthread_mutex_init(&G_CS, NULL) != 0) {
			printf("pthread_mutex_init error\n");
		}
	}
}

char	*trimString(char *str)
{
	int nLen;

	/*
	while (isspace(*str)) str++;
	while (isspace(str[_tcslen(str)-1])) str[_tcslen(str)-1] = '\0'; 
	*/

	// 20101123 SHK
	while (!(*str & 0x80) && isspace(*str)) str++;

    nLen = _tcslen(str);
    nLen--;

    if (nLen < 0)
        return str;

    while (!(str[nLen] & 0x80) && isspace(str[nLen])) 
	{
		str[nLen] = '\0';
		nLen--;
    }

	return str;
}

bool
currentSectionSet(ConfEntry *ce, const char *secName)
{
	int	i;

	for (i = 0; i < ce->maxEntry; i++) {
		if (!strcmp(ce->sections[i].name, secName))
			break;
	}

	if (i >= ce->maxEntry) {
		ce->curEntryIdx = -1;
		return false;
	}

	ce->curEntryIdx = i;
	return true;
}



bool	entryNew(ConfEntry *ce, const char *secName)
{
	if (!ce || !secName || !*secName) {
		ce->curEntryIdx = -1;
		return false;
	}

	if (!currentSectionSet(ce, secName)) {
		if (ce->maxEntry == 0) 
			ce->sections = (struct _SectEntry *)malloc(sizeof(struct _SectEntry));
		else
			ce->sections = (struct _SectEntry *)realloc(ce->sections, sizeof(struct _SectEntry) * (ce->maxEntry+1));

		if (ce->sections == NULL) {
			return false;
		}

		ce->sections[ce->maxEntry].name = strdup(secName);
		ce->sections[ce->maxEntry].h = hash_new(HASH_SIZE);
		ce->curEntryIdx = ce->maxEntry;
		ce->maxEntry++;
	}

	return true;
}



static bool
_parse_config_entry(ConfEntry *ce)
{

	FILE *fp;
	char data[256];
	char *key = NULL, *val = NULL, *str = NULL;  

	if (!(fp = fopen(ce->filename, "r")))
		return false;

	while(fgets(data,256,fp) != NULL) {
		if (data[0] == '#')
			continue;

		if (data[0] == '[') {
			str = strchr(data+1, ']');
			if (str) {
				*str = 0;
				entryNew(ce, data+1);
			}
			else
				ce->curEntryIdx = -1;
		}

		else if (ce->curEntryIdx >= 0) {
			str = strdup(data);
			if ( (val = strchr(str, '=')) != NULL) {
				*val++ = '\0'; key = str;
				if (*val != '\0') {
					key = trimString(key);
					val = trimString(val); 

					if (val[_tcslen(val)-1] == '\n')
						val[_tcslen(val)-1] = '\0';

					if (*val != '\0')
						hash_add(ce->sections[ce->curEntryIdx].h, key, val);
				}
			}
			free(str);
		}
	}  

	fclose(fp);
	return true;
}



ConfEntry *
confEntryNew(const char *filename)
{
	ConfEntry *ce;

	ce = (ConfEntry *)malloc(sizeof(ConfEntry));
	memset(ce, 0, sizeof(ConfEntry));
	ce->curEntryIdx = -1;
	ce->filename = strdup(filename);
	ce->tmRead = time(NULL);
	_parse_config_entry(ce);
	return ce;
}

char *
confEntryGetFilename(ConfEntry *ce)
{
	if (ce)
		return ce->filename;
	else
		return NULL;
}



unsigned char *
confEntryGet(ConfEntry *ce, const char *field)
{
	struct nlist *n;

	if (!ce || !field || ce->curEntryIdx < 0)
		return NULL;
	n = hash_lookup(ce->sections[ce->curEntryIdx].h, (char *)field);
	if (n)
		return (unsigned char *)n->value;

	else
		return NULL;
}

void
confEntryFree(ConfEntry *ce)
{
	int i;

    if(projectHomeDir) free(projectHomeDir);
	if(!ce) return;

	free(ce->filename);
	for (i = 0; i < ce->maxEntry; i++) {
		hash_destroy(ce->sections[i].h);
		free(ce->sections[i].name);
	}
	free(ce->sections);
	free(ce);
	ce = NULL;
}

bool
confEntryPut(ConfEntry *ce, const char *field, char *value)
{
	if (field)
		hash_add(ce->sections[ce->curEntryIdx].h, (char *)field, value);
	return true;
}

bool
confEntrySave(ConfEntry *ce)
{
	int i;
	FILE *fp;
	char *buf = NULL;

	if (!ce || !ce->filename) 
		return false;

	fp = fopen(ce->filename, "w");
	if (fp == NULL)
		return false;

	for (i = 0; i < ce->maxEntry; i++) {
		fprintf(fp, "[%s]\n", ce->sections[i].name);		

		buf = hash_string(ce->sections[i].h);
		if (buf) {
			fprintf(fp, "%s", buf);
			free(buf);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	return true;
}

const char *
settingsFilename(char *fname)
{
	static char _fullpath[256];

	if (projectHomeDir == NULL) {
		char	*tmpdir = getenv(_T("PROJECT_HOME"));

		if (tmpdir == NULL)
			tmpdir = PROJECT_HOME_DIR;

		projectHomeDir = (char *)malloc(_tcslen(tmpdir) + 6);
		if (projectHomeDir) {
			strcpy(projectHomeDir, tmpdir);
			if (projectHomeDir[_tcslen(tmpdir)-1] != '/')
				strcat(projectHomeDir, "/bin/");
			else
				strcat(projectHomeDir, "bin/");
		}
	}

	memset(_fullpath, 0, sizeof(_fullpath));
	strncpy(_fullpath, projectHomeDir, 256);

	if (!fname)
		fname = SETTINGS_FNAME;

	if (_tcslen(_fullpath) + _tcslen(fname) < 256) 
		strcat(_fullpath, fname);

	return _fullpath;
}



/* Mini Icon theme spec implementation */
static int 
_file_exists(char *filename)
{
	struct stat st;
	if (stat(filename, &st)) return 0;
	return 1;
}



static char	_section[128];
static ConfEntry	*_confWork = NULL;
static bool	_bWrite = false;

void confEntryDestroy()
{
	confEntryFree(_confWork);
}

static
int	setProfileSection(const char *section, const char *fName)
{
	struct stat	fileStat;
	int	newB = 0;

	if (_confWork) {
		if (strcmp(_confWork->filename, fName)) 
			newB = 1;
		else {
			if (stat(_confWork->filename, &fileStat) != 0)
				newB = 1;
			else if (fileStat.st_mtime > _confWork->tmRead)
				newB = 1;
		}
	}

	if (newB) {
		if (_bWrite) {
			confEntrySave(_confWork);
			_bWrite = false;
		}
		confEntryFree(_confWork);
		_confWork = NULL;
		memset(_section, 0, sizeof(_section));
	}

	if (_confWork == NULL) {
		_confWork = confEntryNew(fName);
		_bWrite = false;
		memset(_section, 0, sizeof(_section));
		if (_confWork == NULL) {
			return 0;
		}
	}

	if (strcmp(_section, section)) {
		if (currentSectionSet(_confWork, section)) 
			strcpy(_section, section);
		else {
			memset(_section, 0, sizeof(_section));
			return 0;
		}
	}
	return 1;
}



void delspace (char *szdata)
{

//        printf ("===========> [%s]\n", szdata);
        ////////////////////////// space 없애기
        int     kk = 0;
        int ss = 0;
        char    val[256];

        memset(val, 0x00, sizeof(val));
        for (ss=0; ss<_tcslen(szdata); ss++)
        {
                if (szdata[ss] == 0x00)
                        break;
                //else if (szdata[ss] == ' ')
                else if (isspace(szdata[ss]))
                        continue;
                val[kk] = szdata[ss];
                kk++;
        }

        //printf ("===========> [%s]\n", val);
        strcpy (szdata, val);
}

int	GetPrivateProfileInt(const char *section, const char *key, int initVal, const char *fName)
{
	char	*pRes;
	int	retVal;

	checkCSInfo();
	pthread_mutex_lock(&G_CS);
	if (setProfileSection(section, fName)) {
		pRes = (char *)confEntryGet(_confWork, key);
		if (pRes == NULL)
			retVal = initVal;
		else
			retVal = atoi(pRes);
	}
	else
		retVal = initVal;

	pthread_mutex_unlock(&G_CS);
	return retVal;
}

int	GetPrivateProfileIntEX(const char *section, const char *key, int initVal, const char *fName)
{
	char pRes[12], cInitVal[12];

	memset(pRes, 0x00, sizeof pRes);
	sprintf(cInitVal, "%d", initVal);
	GetPrivateProfileStringEX(section, key, cInitVal, pRes, 12, fName);
	return atoi(pRes);
}



int	GetPrivateProfileString(const char *section, const char *key, const char *initVal, char *pRet, int len, const char *fName)
{
	char	*pRes;
	checkCSInfo();
	pthread_mutex_lock(&G_CS);

	if (setProfileSection(section, fName)) {
		pRes = (char *)confEntryGet(_confWork, key);
		if (pRes == NULL) 
			pRes = (char *)initVal;
		
		//printf("confEntryGet : %s\n", pRes);

	}
	else {
		pRes = (char *)initVal;
		//printf("initVal : %s\n", pRes);
	}

	if (pRes) {
		len = strlen(pRes);
		strncpy(pRet, pRes, len);
		pRet[len] = 0;
		//printf("pRes : %s , pRet : %s\n", pRes, pRet);

	}
	else
		*pRet = 0;
	pthread_mutex_unlock(&G_CS);
	//printf("GetPrivateProfileString : %s\n", pRet);
	return len;
}

int	GetPrivateProfileStringEX(const char *section, const char *key, const char *initVal, char *pRet, int len, const char *fName)
{
	FILE *fp;
    char data[256];
    char str_sec[128], str_key[128], val[2048], *sztmp = NULL;
    bool    bchk=0;

    //printf("GetPrivateProfileStingEX section[%s], key[%s], filename[%s]\n", section, key, fName);

    if (!(fp = fopen(fName, "r")))
	{
		pRet = (char *)initVal;
		return;
	}

    while(fgets(data,255,fp) != NULL) 
	{
		if (data[0] == '#' || data[0] == ';')
			continue;

		//printf("DATA[%s]\n",data);

		if (data[0] == '[') 
		{
			sztmp = strchr(data+1,']');
            memset(str_sec, 0x00, sizeof(str_sec));

			//앞뒤의 []값을 더한값 본래 +1에서 역산이기 때문에 -2를 더함
			c_memcpy(str_sec, data+1, (sztmp - data-1));

			str_sec[sztmp - data+2] = '\0';
			if(!strcmp(str_sec, section)){
				//printf("SET bchk str_sec[%s] section[%s]\n", str_sec, section);
				bchk = true;
			}
		}
		else if(bchk) {
			delspace(data);		// space 없애기
			char* pParse[2];
			char* pTok = strtok(data, "=");

			if(pTok == NULL) continue;

			int	nLen = _tcslen(pTok) + 1;
			memset(pParse, 0x00, sizeof pParse);
			pParse[0] = pTok;
			pTok = strtok(data + nLen, "\n");
			pParse[1] = pTok;			

			//printf("[%s]  [%s]\n", pParse[0], pParse[1]);			

			if(!strcmp(pParse[0], key)){
				if(pParse[1] == NULL) pRet = (char *)initVal;
				else strcpy(pRet, pParse[1]);				           

				fclose(fp);
				return strlen(pRet);
			}
		}
	}

	//pRet = (char *)initVal;
	strcpy(pRet, (char *)initVal);
	fclose(fp);
	return strlen(pRet);
}



void	SavePrivateProfile()
{
	checkCSInfo();

	pthread_mutex_lock(&G_CS);
	if (_bWrite && _confWork) {
		confEntrySave(_confWork);
		_bWrite = false;
	}
	pthread_mutex_unlock(&G_CS);
}

void	PutPrivateProfileInt(const char *section, const char *key, int val, const char *fName)
{
	char	szVal[32];
	checkCSInfo();

	pthread_mutex_lock(&G_CS);
	sprintf(szVal, "%d", val);
	if (!setProfileSection(section, fName))
	{
		if (!entryNew(_confWork, section)) {
			memset(_section, 0, sizeof(_section));
			pthread_mutex_unlock(&G_CS);
			return;
		}
		strcpy(_section, section);
	}

	confEntryPut(_confWork, key, szVal);
	_bWrite = true;
	pthread_mutex_unlock(&G_CS);
	return;
}



void	PutPrivateProfileString(const char *section, const char *key, const char *val, const char *fName)
{
	checkCSInfo();
	pthread_mutex_lock(&G_CS);
	if (!setProfileSection(section, fName)) {
		if (!entryNew(_confWork, section)) {
			memset(_section, 0, sizeof(_section));
			pthread_mutex_unlock(&G_CS);
			return;
		}
		strcpy(_section, section);
	}

	confEntryPut(_confWork, key, (char *)val);
	_bWrite = true;
	pthread_mutex_unlock(&G_CS);
}



#else
#include <stdio.h>
#include <windows.h>

void	PutPrivateProfileInt(const TCHAR *section, const TCHAR *key, int val, const TCHAR *fName)
{
	TCHAR	szVal[32];

	memset(szVal, 0x00, sizeof(szVal));
	_stprintf(szVal, _T("%d"), val);
#ifndef UNICODE
	WritePrivateProfileStringA(section, key, szVal, fName);
#else
	WritePrivateProfileString(section, key, szVal, fName);
#endif

}



void	PutPrivateProfileString(const TCHAR *section, const TCHAR *key, const TCHAR *val, const TCHAR *fName)
{
#ifndef UNICODE
	WritePrivateProfileStringA(section, key, val, fName);
#else
	WritePrivateProfileString(section, key, val, fName);
#endif
}

void  SavePrivateProfile() {}
void  confEntryDestroy() {}




#endif



/* ************************************************************
 *		END of profileutil.c
 * ************************************************************ */

