///////////////////////////////////////////////////////////////////////////
// Copyright 2020 haikea
// License(GPL)  ���������ʹ�ã�����������Ϊ��ҵ���ӯ����
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

#define CONFIG_PATH_MAX_LENGTH   (512)         //�����ļ�·��
#define CONFIG_KEY_COUNT           (20)            //���������ļ������������
#define CONFIG_KEY_MAX_LENGTH       (50)      //����������key��󳤶�
#define CONFIG_VALUE_MAX_LENGTH    (128)  //����������value��󳤶�

#define DEFAULT_LOG_FILE_SIZE (8192)              //������־�ļ����size
#define DEFAULT_LOG_FILE_MAX_NUM 10         //��ౣ���ļ�����
#define DEFAULT_LOG_NAME "haikea_log"        //�ļ�Ĭ������
#define DEFAULT_LOG_PATH "./haikea"              //�ļ�Ĭ��·��
#define DEFAULT_LOG_LEVEL INFO_LOG            //�ļ�Ĭ�ϼ���

#ifndef F_OK
#define F_OK 0
#endif // !F_OK



//��־����ö��
typedef enum logLevel
{
	DEBUG_LOG = 0,
	INFO_LOG = 1,
	ERROR_LOG = 2,
	NONE_LOG = 3,
} LOG_LEVEL;

#define FILE_FUN_LINE  __FILE__, __FUNCTION__, __LINE__                   
#define LOG_START()  Writelog(INFO_LOG, FILE_FUN_LINE , "start")           // 1��ʼ
#define LOG_END()     Writelog(INFO_LOG, FILE_FUN_LINE , "end")            // 2����
#define LOG_INFO(...)  Writelog(INFO_LOG, FILE_FUN_LINE , ##__VA_ARGS__)              // 3�����Ϣ��־
#define LOG_ERROR(...)  Writelog(ERROR_LOG, FILE_FUN_LINE , ##__VA_ARGS__)           // 4���������־
#define LOG_DEBUG(...)  Writelog(DEBUG_LOG, FILE_FUN_LINE , ##__VA_ARGS__)           // 5���������־

//�����ļ��ṹ��
typedef struct config
{
	char config_path[CONFIG_PATH_MAX_LENGTH];                                                                                     //�����ļ�·��
	char key[CONFIG_KEY_COUNT][CONFIG_KEY_MAX_LENGTH];                      //����20��key��ÿ��key���50���ַ�
	char value[CONFIG_KEY_COUNT][CONFIG_VALUE_MAX_LENGTH];              //����20��value��ÿ��value���128���ַ�
	int32_t keyCount;                                                                                             //��¼��ֵ����
} CONFIG_T;

//������־�������ṹ��
typedef struct logConfig
{
	LOG_LEVEL logLevel;                                            //��־����(DEBUG_LOG,INFO_LOG,ERROR_LOG,NONE_LOG)
	char logPath[256];                                                //��־���·��
	char logName[256];                                             //��־����
	int32_t fileSize;                                                     //������־�ļ���С����λΪByte
	int32_t fileMaxNumber;                                       //��־�ļ������������ڴ���������ѭ��
} LOG_CONFIG_T;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	//��ʼ����־ϵͳ
	int32_t LogInit(const char* configPtr);

	//д��־
	int32_t Writelog(LOG_LEVEL  level, const char* fileName, const char* function, int32_t lineNum, const char* fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !COMMONUTILS_HAIKEA_LOG_H_
