#include "haikea_log.h"
#include <stdarg.h>           // va_start  va_end
#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <stdint.h>           // uint32_t
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <io.h>
#include <stdbool.h>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")


#define RET_SUCCESS (0)
#define RET_NG1 (1)
#define RET_NG2 (2)
#define RET_NG3 (3)

//��־������
LOG_CONFIG_T logMng;

//��ǰ��־�ļ����
FILE* currentFilePtr;

//��ǰ��־�ļ����
int32_t currentFileNum;

//��ǰ��־�ļ���С
int32_t currentFileSize;

//�ļ���
pthread_mutex_t fileLock;

//��ʼ���ɹ���־λ
bool curLogInitOK = false;

/*
 *  @���ܸ�Ҫ: ��¼��־
 *  @�����б�: 
 *  level   ��in��   ��ǰ��־����
 *  fileName    (in)     �ļ�����
 *  function     ��in)      ������
 *  line            ��in��    ��־����
 *  fmt             ��in��   �ɱ������ʽ
 *  ...                 ��in��   �ɱ����
 *  @ִ�н��: 
 *             RET_SUCCESS .... ִ�гɹ�
 *             RET_NG1 .... ִ��ʧ��
 * */
int32_t Writelog( LOG_LEVEL  level, const char* fileName, const char* function , int32_t lineNum , const char* fmt, ...) {
	//�Ƚ���־�����ϵͳ����
	if (level<logMng.logLevel)
	{
		return RET_SUCCESS;
	}

	//��ȡ��־����
	char currentLevel[5];
	if (level==DEBUG_LOG)
	{
		snprintf(currentLevel, sizeof(currentLevel), "%s","DBG");
	}
	else if (level==INFO_LOG)
	{
		snprintf(currentLevel, sizeof(currentLevel), "%s", "INFO");
	}
	else if (level==ERROR_LOG)
	{
		snprintf(currentLevel, sizeof(currentLevel),  "%s", "ERR");
	}
	else
	{
		snprintf(currentLevel, sizeof(currentLevel), "%s", "NONE");
	}

	//��ȡʱ��
	char currentTime[58];
	SYSTEMTIME curTime;
	GetLocalTime(&curTime);
	snprintf(currentTime, sizeof(currentTime), "%04d/%02d/%02d %02d:%02d:%02d.%03d",
		curTime.wYear, curTime.wMonth, curTime.wDay, curTime.wHour ,
		curTime.wMinute, curTime.wSecond, curTime.wMilliseconds);
	
	//��ȡ�ɱ������Ϣ
	char currentMsg[256];
	va_list ap;
	va_start(ap,fmt);
	vsprintf(currentMsg, fmt, ap);
	va_end(ap);

	//����
	pthread_mutex_lock(&fileLock);

	if (!curLogInitOK)
	{
		printf("[%s] [%s] [%s-%s-%d] %s\n", currentTime, currentLevel, fileName, function, lineNum, currentMsg);
		return RET_SUCCESS;
	}

	//��װ��Ϣ
	char currentLog[512];
	sprintf(currentLog, "[%s] [%s] [%s-%s-%d] %s", currentTime, currentLevel, fileName, function, lineNum, currentMsg);

	//���ļ���¼����
	fprintf(currentFilePtr, "%s\n", currentLog);

	//�ж��ļ���С������������ֵ���л��ļ�
	int len = ftell(currentFilePtr);
	if (len>=logMng.fileSize)
	{
		int ret = OpenCurrentLog();
		if (ret!=RET_SUCCESS)
		{
			LOG_ERROR("%s","open log file failed");
			pthread_mutex_unlock(&fileLock);
			return RET_NG1;
		}
	}
	pthread_mutex_unlock(&fileLock);
	return RET_SUCCESS;
}

/*
 *  @���ܸ�Ҫ: ��ʼ����־
 *  @�����б�:
 *  level   ��in��   ��ǰ��־����
 *  @ִ�н��:
 *             RET_SUCCESS .... ִ�гɹ�
 *             RET_NG1 .... ִ��ʧ��
 * */
int32_t LogInit(const char* configPtr) {
	if (curLogInitOK)
	{
		return RET_SUCCESS;
	}

	pthread_mutex_init(&fileLock, NULL);
	pthread_mutex_lock(&fileLock);
	currentFilePtr = NULL;
	currentFileNum = 0;
	currentFileSize = 0;

	int ret = RET_NG1;
	if (configPtr == NULL)
	{
		LOG_ERROR("%s", "configPtr is NULL");
		pthread_mutex_unlock(&fileLock);
		return RET_NG1;
	}

	CONFIG_T haikea_config;
	snprintf(haikea_config.config_path, sizeof(haikea_config.config_path), "%s", configPtr);

	//��ȡ�����ļ�
	ret= ReadConfig(&haikea_config);
	if (ret!=RET_SUCCESS)
	{
		LOG_ERROR("%s", "read config failed");
		pthread_mutex_unlock(&fileLock);
		return ret;
	}

	//������־�����Ĭ��ֵ
	snprintf(logMng.logPath,sizeof(logMng.logPath),"%s", DEFAULT_LOG_PATH );
	snprintf(logMng.logName, sizeof(logMng.logPath), "%s", DEFAULT_LOG_NAME );
	logMng.fileSize = DEFAULT_LOG_FILE_SIZE * 1024 ;
	logMng.fileMaxNumber = DEFAULT_LOG_FILE_MAX_NUM ;
	logMng.logLevel = DEFAULT_LOG_LEVEL ;
	ret = SetLogMng(&logMng,&haikea_config);
	if (ret != RET_SUCCESS)
	{
		LOG_ERROR("%s", "set logMng failed");
		pthread_mutex_unlock(&fileLock);
		return ret;
	}
	
	ret = OpenCurrentLog();
	if (ret != RET_SUCCESS)
	{
		LOG_ERROR("%s", "open log file failed");
		pthread_mutex_unlock(&fileLock);
		return ret;
	}
	curLogInitOK = true ;
	pthread_mutex_unlock(&fileLock);
	return RET_SUCCESS ;
}

/*
 *  @���ܸ�Ҫ: �򿪵�ǰ��־�ļ�
 *  @�����б�:
 *  @ִ�н��:
 *             RET_SUCCESS .... ִ�гɹ�
 *             RET_NG1 .... ִ��ʧ��
 * */
int32_t OpenCurrentLog() {
	int ret = 0;
	//Ѱ���ļ�·��
	if (access(logMng.logPath, F_OK) != 0)
	{
		//�����ļ���
		ret = _mkdir(logMng.logPath);
		if (ret!=RET_SUCCESS)
		{
			LOG_ERROR("%s", "mkdir failed");
			return RET_NG1;
		}
	}

	//�ж��ļ�����Ƿ��
	if (currentFilePtr!=NULL)
	{
		//�ж��ļ��Ƿ�ﵽ���ֵ,���û�дﵽ�����سɹ�
		currentFileSize = ftell(currentFilePtr);
		if (currentFileSize< logMng.fileSize )
		{
			return RET_SUCCESS;
		}
		//����ﵽ���ر�ԭ�ļ�
		else
		{
			//fflush(currentFilePtr);
			fclose(currentFilePtr);
			currentFileNum++;
			if (currentFileNum>=logMng.fileMaxNumber)
			{
				currentFileNum = 0;
			}
		}
	}
	
	//�����ļ�
	char currentFilePath[256];
	snprintf(currentFilePath ,sizeof(currentFilePath), "%s/%s%02d" , logMng.logPath , logMng.logName , currentFileNum );
	currentFilePtr = fopen( currentFilePath , "w" );
	if (currentFilePtr==NULL)
	{
		LOG_ERROR("%s", "open new file failed");
		return RET_NG1;
	}
	
	return RET_SUCCESS;
}

/*
 *  @���ܸ�Ҫ: ������־����
 *  @�����б�:
 *  loglevel    (in)         ��־����
 *  logMngPtr   (in)      ��־������
 *  @ִ�н��:
 *             RET_SUCCESS .... ִ�гɹ�
 *             RET_NG1 .... ִ��ʧ��
 * */
int32_t SetLogLevel( char* logLevel, LOG_CONFIG_T* logMngPtr) {
	if (logLevel == NULL || logMngPtr == NULL)
	{
		LOG_ERROR("%s", "logLevel is NULL or logMngPtr is NULL");
		return RET_NG1;
	}
	if (strcmp(logLevel,"DBG")==0)
	{
		logMngPtr->logLevel = DEBUG_LOG;
	}

	if (strcmp(logLevel, "INF") == 0)
	{
		logMngPtr->logLevel = INFO_LOG;
	}

	if (strcmp(logLevel, "ERR") == 0)
	{
		logMngPtr->logLevel = ERROR_LOG;
	}

	if (strcmp(logLevel, "NONE") == 0)
	{
		logMngPtr->logLevel = NONE_LOG;
	}
	return RET_SUCCESS;
	
}


/*
 *  @���ܸ�Ҫ: ������־������
 *  @�����б�:
 *  logMngPtr   (in)      ��־������
 *  configPtr     (in)    ��ȡ�����ļ��ṹ��
 *  @ִ�н��:
 *             RET_SUCCESS .... ִ�гɹ�
 *             RET_NG1 .... ִ��ʧ��
 * */
int32_t SetLogMng(LOG_CONFIG_T* logMngPtr, CONFIG_T* configPtr) {
	int ret = 0;

	if (logMngPtr==NULL||configPtr==NULL)
	{
		LOG_ERROR("%s", "logMngPtr is NULL or configPtr is NULL");
		return RET_NG1;
	}

	int tempInt = 0;
	//������־������
	for (size_t i = 0; i < configPtr->keyCount ; i++)
	{
		if ( strcmp(configPtr->key[i],"LOG_LEVEL" )==0 )
		{
			//�������ʧ�ܣ���Ĭ�ϼ���
			if (SetLogLevel(configPtr->value[i], logMngPtr) != RET_SUCCESS)
			{
				logMngPtr->logLevel = DEFAULT_LOG_LEVEL;
			} 
		}

		if ( strcmp(configPtr->key[i], "LOG_PATH")==0)
		{
			//������־���·��
			snprintf(logMngPtr->logPath, sizeof(logMngPtr->logPath), "%s", configPtr->value[i]);
		}

		if (strcmp( configPtr->key[i] , "LOG_NAME")==0)
		{
			//������־�ļ���
			snprintf(logMngPtr->logName, sizeof(logMngPtr->logName) , "%s" , configPtr->value[i] );
		}

		if (strcmp(configPtr->key[i] , "LOG_FILE_SIZE")==0)
		{
			//������־�ļ�size
			tempInt = atoi(configPtr->value[i]) ;
			if (1024<=tempInt<8192)
			{
				logMngPtr->fileSize = tempInt*1024;
			}
			//else 1���õ�ֵ������Ҫ��2atoiת���ַ���ʧ�ܷ���0��fileSize����Ĭ��ֵ
		}

		if (strcmp( configPtr->key[i] ,"LOG_FILE_MAX_NUM")==0)
		{
			//������־�ļ������������������ӵ�һ���ļ���ʼ��д�ļ�
			tempInt = atoi(configPtr->value[i]);
			if (5 <= tempInt <= 100)
			{
				logMngPtr->fileMaxNumber = tempInt;
			}
			//else 1���õ�ֵ������Ҫ��2atoiת���ַ���ʧ�ܷ���0 ��fileMaxNumber����Ĭ��ֵ 10
		}

	} //for (size_t i = 0; i < configPtr->keyCount ; i++)

	return RET_SUCCESS;

}

/*
 *  @���ܸ�Ҫ: ��ȡ��־�����ļ�
 *  @�����б�:
 *  configPtr     (in)    �����ļ��ṹ��
 *  @ִ�н��:
 *             RET_SUCCESS .... ִ�гɹ�
 *             RET_NG1 .... ִ��ʧ��
 * */
int32_t ReadConfig(CONFIG_T* configPtr) {

	if (configPtr==NULL || configPtr->config_path == NULL)
	{
		LOG_ERROR("%s", "configPtr is NULL or config_path is NULL");
		return RET_NG1;
	}

	//��ȡ�����ļ�
	FILE* configFile = fopen(configPtr->config_path , "r");
	if (configFile == NULL) {
		LOG_ERROR("%s", "configFile is NULL");
		return RET_NG2;
	}
	LOG_INFO("%S","open configFile success");
	char lineString[512];
	int32_t config_count = 0;
	int tempKeyCount = 0;   //��¼������
	//ѭ����ȡÿ������
	while (true)
	{
		//�ļ���֮��ѭ����ȡÿ�е�����
		if (fgets(lineString, sizeof(lineString), configFile) == NULL) {
			break;
		}
		//����ͷ��β�Ŀո��Ʊ�������лس�
		deleteSpaceTabEnter(lineString);

		//�ж������#��ͷ����Ϊע��
		if (*lineString == '#' || *lineString=='\0' ) {
			continue;
		}

		//����key��value
		char tempKey[256];
		char tempValue[256];
		bool iskey = true;
		int i = 0;
		int j = 0;
		//��ȡ��ֵ��
		while ( lineString[i] != '\0' )
		{
			 //��ȡkeyֵ
			if (iskey)
			{
				if ( lineString[i] != ' ' && lineString[i] != '=' )
				{
					tempKey[j++] = lineString[i++];
				}
				else
				{
					tempKey[j] = '\0';
					iskey = false;
					j = 0;
				}	
			}
			//��ȡvalueֵ
			else
			{
				if (lineString[i] != ' '&& lineString[i] != '=' )
				{
					tempValue[j++] = lineString[i];
				}
				i++;
			}
		}  //while ( lineString[i] != '\0' )  //��ȡ��ֵ��
		tempValue[j++] = '\0';
		
		//����ȡ��key��value�����ֵ�
		if (tempKeyCount >= CONFIG_KEY_COUNT-1)
		{
			break;
		}
		else
		{
			snprintf(configPtr->key[tempKeyCount],sizeof(configPtr->key[tempKeyCount]),"%s",tempKey);
			snprintf(configPtr->value[tempKeyCount], sizeof(configPtr->key[tempKeyCount]), "%s", tempValue);
			tempKeyCount++;
		}

		configPtr->keyCount = tempKeyCount;
	} //ѭ����ȡÿ������
}


/*
 *  @���ܸ�Ҫ: ɾ���ַ����ײ��Ŀո��tab��β���Ŀո�/tab/����/�س�
 *  @�����б�: str    ....(in/out) ...   �����ļ��е�������
 *  @ִ�н��: RET_SUCCESS .... ִ�гɹ�
 *             RET_NG1 .... ִ��ʧ��
 * */
int32_t deleteSpaceTabEnter(char *str) {
	if (str==NULL)
	{
		LOG_ERROR("%s", "str is NULL");
		return RET_NG1;
	}

	int32_t lenTemp = strlen(str);

	//���str��β�пո񣬻س������У��Ʊ�
	while (str[lenTemp-1]==' '|| str[lenTemp - 1] == '\r'|| str[lenTemp - 1] == '\n'|| str[lenTemp - 1] == '\t')
	{
		str[lenTemp - 1] = '\0';
		lenTemp--;
		if (lenTemp==0)
		{
			break;
		}
	}

	char *pstr = str;

	//ȥ��str��ͷ�Ŀո���Ʊ�
	while (*str==' '|| *str == '\t')
	{
		++str;
	}

	// �ж��ַ����Ƿ���Ҫǰ�� 
	if (pstr == str)
	{
		return RET_SUCCESS;
	}

	//�����Ҫ����Ϊ�β�ָ��ֵ���ܸı䣬ֻ��һλһλ�ı��ַ�ϵ�����
	while (*str!='\0')
	{
		*pstr++ = *str++;
	}
	*pstr = '\0';

	return RET_SUCCESS;
}