// Common.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

//#include "pch.h"
#include <iostream>
#include <stdarg.h>
#include <Windows.h>

#include "haikea_log.h"

int main()
{
	LOG_START();
	LOG_END();
	LOG_INFO("%s%02d", "info_s", 123);
	LOG_DEBUG("%s%02d%s", "de_s", 456, "Bug");
	LOG_ERROR("%06d", 9955);

	//haikea_config.config_path = "./logconfig";
	LogInit( "./logconfig" );
	for (size_t i = 0; i < 100000; i++)
	{
	     Writelog(DEBUG_LOG, __FILE__, __FUNCTION__, __LINE__, "%s%d%s", "class", i, "u07ig");
	}

	LOG_START();
	LOG_END();
	LOG_INFO("%s%02d","info",123);
	LOG_DEBUG("%s%02d%s","de",456,"Bug");
	LOG_ERROR("%06d",99);
    std::cout << "Hello World!\n"; 
}

