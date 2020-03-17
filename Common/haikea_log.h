///////////////////////////////////////////////////////////////////////////
// Copyright 2020 haikea
// License(GPL)  （可以免费使用，但不允许作为商业软件盈利）
// Author: haikea
// this is a log util
// Created on:2020/1/26
// Modified on: 2020/1/26
///////////////////////////////////////////////////////////////////////////

#ifndef COMMONUTILS_HAIKEA_LOG_H_
#define COMMONUTILS_HAIKEA_LOG_H_ 1

#include <stdint.h>
#include <io.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>

#define CONFIG_PATH_MAX_LENGTH   (512)         //配置文件路径
#define CONFIG_KEY_COUNT           (20)            //单个配置文件配置项最多数
#define CONFIG_KEY_MAX_LENGTH       (50)      //单个配置项key最大长度
#define CONFIG_VALUE_MAX_LENGTH    (128)  //单个配置项value最大长度

#define DEFAULT_LOG_FILE_SIZE (8192)              //单个日志文件最大size
#define DEFAULT_LOG_FILE_MAX_NUM 10         //最多保存文件个数
#define DEFAULT_LOG_NAME "haikea_log"        //文件默认名字
#define DEFAULT_LOG_PATH "./haikea"              //文件默认路径
#define DEFAULT_LOG_LEVEL INFO_LOG            //文件默认级别

#ifndef F_OK
#define F_OK 0
#endif // !F_OK



//日志级别枚举
typedef enum logLevel
{
	DEBUG_LOG = 0,
	INFO_LOG = 1,
	ERROR_LOG = 2,
	NONE_LOG = 3,
} LOG_LEVEL;

#define FILE_FUN_LINE  __FILE__, __FUNCTION__, __LINE__                   
#define LOG_START()  Writelog(INFO_LOG, FILE_FUN_LINE , "start")           // 1开始
#define LOG_END()     Writelog(INFO_LOG, FILE_FUN_LINE , "end")            // 2结束
#define LOG_INFO(...)  Writelog(INFO_LOG, FILE_FUN_LINE , ##__VA_ARGS__)              // 3输出信息日志
#define LOG_ERROR(...)  Writelog(ERROR_LOG, FILE_FUN_LINE , ##__VA_ARGS__)           // 4输出错误日志
#define LOG_DEBUG(...)  Writelog(DEBUG_LOG, FILE_FUN_LINE , ##__VA_ARGS__)           // 5输出调试日志

//配置文件结构体
typedef struct config
{
	char config_path[CONFIG_PATH_MAX_LENGTH];                                                                                     //配置文件路径
	char key[CONFIG_KEY_COUNT][CONFIG_KEY_MAX_LENGTH];                      //最多存20个key，每个key最多50个字符
	char value[CONFIG_KEY_COUNT][CONFIG_VALUE_MAX_LENGTH];              //最多存20个value，每个value最多128个字符
	int32_t keyCount;                                                                                             //记录键值对数
} CONFIG_T;

//定义日志管理器结构体
typedef struct logConfig
{
	LOG_LEVEL logLevel;                                            //日志级别(DEBUG_LOG,INFO_LOG,ERROR_LOG,NONE_LOG)
	char logPath[256];                                                //日志存放路径
	char logName[256];                                             //日志名字
	int32_t fileSize;                                                     //单个日志文件大小，单位为Byte
	int32_t fileMaxNumber;                                       //日志文件最多个数，大于次数将重新循环
} LOG_CONFIG_T;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	//初始化日志系统
	int32_t LogInit(const char* configPtr);

	//写日志
	int32_t Writelog(LOG_LEVEL  level, const char* fileName, const char* function, int32_t lineNum, const char* fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !COMMONUTILS_HAIKEA_LOG_H_
