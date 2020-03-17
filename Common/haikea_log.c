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

//日志管理器
LOG_CONFIG_T logMng;

//当前日志文件句柄
FILE* currentFilePtr;

//当前日志文件编号
int32_t currentFileNum;

//当前日志文件大小
int32_t currentFileSize;

//文件锁
pthread_mutex_t fileLock;

//初始化成功标志位
bool curLogInitOK = false;

/*
 *  @功能概要: 记录日志
 *  @参数列表: 
 *  level   （in）   当前日志级别
 *  fileName    (in)     文件名称
 *  function     （in)      函数名
 *  line            （in）    日志行数
 *  fmt             （in）   可变参数格式
 *  ...                 （in）   可变参数
 *  @执行结果: 
 *             RET_SUCCESS .... 执行成功
 *             RET_NG1 .... 执行失败
 * */
int32_t Writelog( LOG_LEVEL  level, const char* fileName, const char* function , int32_t lineNum , const char* fmt, ...) {
	//比较日志级别和系统级别
	if (level<logMng.logLevel)
	{
		return RET_SUCCESS;
	}

	//获取日志级别
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

	//获取时间
	char currentTime[58];
	SYSTEMTIME curTime;
	GetLocalTime(&curTime);
	snprintf(currentTime, sizeof(currentTime), "%04d/%02d/%02d %02d:%02d:%02d.%03d",
		curTime.wYear, curTime.wMonth, curTime.wDay, curTime.wHour ,
		curTime.wMinute, curTime.wSecond, curTime.wMilliseconds);
	
	//获取可变参数信息
	char currentMsg[256];
	va_list ap;
	va_start(ap,fmt);
	vsprintf(currentMsg, fmt, ap);
	va_end(ap);

	//加锁
	pthread_mutex_lock(&fileLock);

	if (!curLogInitOK)
	{
		printf("[%s] [%s] [%s-%s-%d] %s\n", currentTime, currentLevel, fileName, function, lineNum, currentMsg);
		return RET_SUCCESS;
	}

	//组装信息
	char currentLog[512];
	sprintf(currentLog, "[%s] [%s] [%s-%s-%d] %s", currentTime, currentLevel, fileName, function, lineNum, currentMsg);

	//打开文件记录内容
	fprintf(currentFilePtr, "%s\n", currentLog);

	//判断文件大小，如果大于最大值，切换文件
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
 *  @功能概要: 初始化日志
 *  @参数列表:
 *  level   （in）   当前日志级别
 *  @执行结果:
 *             RET_SUCCESS .... 执行成功
 *             RET_NG1 .... 执行失败
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

	//读取配置文件
	ret= ReadConfig(&haikea_config);
	if (ret!=RET_SUCCESS)
	{
		LOG_ERROR("%s", "read config failed");
		pthread_mutex_unlock(&fileLock);
		return ret;
	}

	//配置日志配置项及默认值
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
 *  @功能概要: 打开当前日志文件
 *  @参数列表:
 *  @执行结果:
 *             RET_SUCCESS .... 执行成功
 *             RET_NG1 .... 执行失败
 * */
int32_t OpenCurrentLog() {
	int ret = 0;
	//寻找文件路径
	if (access(logMng.logPath, F_OK) != 0)
	{
		//创建文件夹
		ret = _mkdir(logMng.logPath);
		if (ret!=RET_SUCCESS)
		{
			LOG_ERROR("%s", "mkdir failed");
			return RET_NG1;
		}
	}

	//判断文件句柄是否打开
	if (currentFilePtr!=NULL)
	{
		//判断文件是否达到最大值,如果没有达到，返回成功
		currentFileSize = ftell(currentFilePtr);
		if (currentFileSize< logMng.fileSize )
		{
			return RET_SUCCESS;
		}
		//如果达到，关闭原文件
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
	
	//打开新文件
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
 *  @功能概要: 设置日志级别
 *  @参数列表:
 *  loglevel    (in)         日志级别
 *  logMngPtr   (in)      日志管理器
 *  @执行结果:
 *             RET_SUCCESS .... 执行成功
 *             RET_NG1 .... 执行失败
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
 *  @功能概要: 设置日志管理器
 *  @参数列表:
 *  logMngPtr   (in)      日志管理器
 *  configPtr     (in)    读取配置文件结构体
 *  @执行结果:
 *             RET_SUCCESS .... 执行成功
 *             RET_NG1 .... 执行失败
 * */
int32_t SetLogMng(LOG_CONFIG_T* logMngPtr, CONFIG_T* configPtr) {
	int ret = 0;

	if (logMngPtr==NULL||configPtr==NULL)
	{
		LOG_ERROR("%s", "logMngPtr is NULL or configPtr is NULL");
		return RET_NG1;
	}

	int tempInt = 0;
	//配置日志管理器
	for (size_t i = 0; i < configPtr->keyCount ; i++)
	{
		if ( strcmp(configPtr->key[i],"LOG_LEVEL" )==0 )
		{
			//如果设置失败，按默认级别
			if (SetLogLevel(configPtr->value[i], logMngPtr) != RET_SUCCESS)
			{
				logMngPtr->logLevel = DEFAULT_LOG_LEVEL;
			} 
		}

		if ( strcmp(configPtr->key[i], "LOG_PATH")==0)
		{
			//设置日志存放路径
			snprintf(logMngPtr->logPath, sizeof(logMngPtr->logPath), "%s", configPtr->value[i]);
		}

		if (strcmp( configPtr->key[i] , "LOG_NAME")==0)
		{
			//设置日志文件名
			snprintf(logMngPtr->logName, sizeof(logMngPtr->logName) , "%s" , configPtr->value[i] );
		}

		if (strcmp(configPtr->key[i] , "LOG_FILE_SIZE")==0)
		{
			//设置日志文件size
			tempInt = atoi(configPtr->value[i]) ;
			if (1024<=tempInt<8192)
			{
				logMngPtr->fileSize = tempInt*1024;
			}
			//else 1设置的值不符合要求，2atoi转换字符串失败返回0，fileSize等于默认值
		}

		if (strcmp( configPtr->key[i] ,"LOG_FILE_MAX_NUM")==0)
		{
			//设置日志文件数量，超过数量将从第一个文件开始重写文件
			tempInt = atoi(configPtr->value[i]);
			if (5 <= tempInt <= 100)
			{
				logMngPtr->fileMaxNumber = tempInt;
			}
			//else 1设置的值不符合要求，2atoi转换字符串失败返回0 ，fileMaxNumber等于默认值 10
		}

	} //for (size_t i = 0; i < configPtr->keyCount ; i++)

	return RET_SUCCESS;

}

/*
 *  @功能概要: 读取日志配置文件
 *  @参数列表:
 *  configPtr     (in)    配置文件结构体
 *  @执行结果:
 *             RET_SUCCESS .... 执行成功
 *             RET_NG1 .... 执行失败
 * */
int32_t ReadConfig(CONFIG_T* configPtr) {

	if (configPtr==NULL || configPtr->config_path == NULL)
	{
		LOG_ERROR("%s", "configPtr is NULL or config_path is NULL");
		return RET_NG1;
	}

	//读取配置文件
	FILE* configFile = fopen(configPtr->config_path , "r");
	if (configFile == NULL) {
		LOG_ERROR("%s", "configFile is NULL");
		return RET_NG2;
	}
	LOG_INFO("%S","open configFile success");
	char lineString[512];
	int32_t config_count = 0;
	int tempKeyCount = 0;   //记录配置数
	//循环读取每行内容
	while (true)
	{
		//文件打开之后，循环读取每行的内容
		if (fgets(lineString, sizeof(lineString), configFile) == NULL) {
			break;
		}
		//处理开头结尾的空格，制表符，换行回车
		deleteSpaceTabEnter(lineString);

		//判断如果是#开头，则为注释
		if (*lineString == '#' || *lineString=='\0' ) {
			continue;
		}

		//解析key和value
		char tempKey[256];
		char tempValue[256];
		bool iskey = true;
		int i = 0;
		int j = 0;
		//获取键值对
		while ( lineString[i] != '\0' )
		{
			 //读取key值
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
			//读取value值
			else
			{
				if (lineString[i] != ' '&& lineString[i] != '=' )
				{
					tempValue[j++] = lineString[i];
				}
				i++;
			}
		}  //while ( lineString[i] != '\0' )  //获取键值对
		tempValue[j++] = '\0';
		
		//将读取的key和value放入字典
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
	} //循环读取每行内容
}


/*
 *  @功能概要: 删除字符串首部的空格和tab，尾部的空格/tab/换行/回车
 *  @参数列表: str    ....(in/out) ...   配置文件中的行数据
 *  @执行结果: RET_SUCCESS .... 执行成功
 *             RET_NG1 .... 执行失败
 * */
int32_t deleteSpaceTabEnter(char *str) {
	if (str==NULL)
	{
		LOG_ERROR("%s", "str is NULL");
		return RET_NG1;
	}

	int32_t lenTemp = strlen(str);

	//如果str结尾有空格，回车，换行，制表
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

	//去掉str开头的空格和制表
	while (*str==' '|| *str == '\t')
	{
		++str;
	}

	// 判断字符串是否需要前移 
	if (pstr == str)
	{
		return RET_SUCCESS;
	}

	//如果需要，因为形参指针值不能改变，只好一位一位改变地址上的内容
	while (*str!='\0')
	{
		*pstr++ = *str++;
	}
	*pstr = '\0';

	return RET_SUCCESS;
}