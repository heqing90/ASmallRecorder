#include "Log.h"
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <iostream>
#include "File.h"

#define SIZE_1K 1024


std::map<std::string, Logger*> Logger::mInstances;
char Logger::mPath[MAX_PATH] = {};
LOG_LEVEL Logger::mLevel = LOG_LEVEL::DEBUG;
bool Logger::mConsole = false;


void Logger::write(LOG_LEVEL level, const char * format, ...)
{
	if (level >= mLevel)
	{
		int offset = 0;
		char buffer[SIZE_1K * 2] = { 0 };
		char timestamp[SIZE_1K] = { 0 };
		SYSTEMTIME time;

		::GetLocalTime(&time);
		sprintf(timestamp, "[%04u-%02u-%02u %02u:%02u:%02u.%03u",
			time.wYear, time.wMonth, time.wDay,
			time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
		DWORD processId = ::GetCurrentProcessId();
		DWORD threadId = ::GetCurrentThreadId();
		offset += sprintf(buffer, "%s %d %d", timestamp, processId, threadId);
		switch (level)
		{
		case LOG_LEVEL::DEBUG:
			offset += sprintf(buffer + offset, " DEBUG]");
			break;
		case LOG_LEVEL::INFO:
			offset += sprintf(buffer + offset, " INFO]");
			break;
		case LOG_LEVEL::WARN:
			offset += sprintf(buffer + offset, " WARNING]");
			break;
		case LOG_LEVEL::ERR:
			offset += sprintf(buffer + offset, " ERROR]");
			break;
		default:
			break;
		}

		offset += sprintf(buffer + offset, " %s: ", mName.c_str());

		va_list	_args;
		va_start(_args, format);
		vsprintf(buffer + offset, format, _args);
		va_end(_args);

		if (mConsole)
		{
			std::cout << buffer << std::endl;
		}
		try
		{
			QFile fd(mPath, "a+");
			fprintf_s(fd, "%s\n", buffer);
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
}