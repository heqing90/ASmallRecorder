#pragma once
#include <windows.h>
#include <map>
#include <string.h>

enum class LOG_LEVEL
{
	DEBUG,
	INFO,
	WARN,
	ERR
};


class Logger
{
public:

	static void setup(const char* logPath, LOG_LEVEL level, bool console)
	{
		strcpy_s(mPath, logPath);
		mLevel = level;
		mConsole = console;
	}

	static void destroy()
	{
		for (auto it : mInstances)
		{
			delete (it.second);
		}
		mInstances.clear();
	}

	static Logger* getLogger(const char* name) 
	{ 
		auto it = mInstances.find(name);
		if (it != mInstances.end())
		{
			return it->second;
		}
		Logger* instance = new Logger(name);
		mInstances[name] = instance;
		return mInstances[name];
	}

	void info(const char * format, ...) 
	{ 
		va_list	args;
		va_start(args, format);
		write(LOG_LEVEL::INFO, format, args); 
		va_end(args);
	}
	
	void debug(const char * format, ...) 
	{
		va_list	args;
		va_start(args, format);
		write(LOG_LEVEL::DEBUG, format, args);
		va_end(args);
	}

	void warn(const char * format, ...)
	{
		va_list	args;
		va_start(args, format);
		write(LOG_LEVEL::WARN, format, args);
		va_end(args);
	}
	
	void error(const char * format, ...) 
	{
		va_list	args;
		va_start(args, format);
		write(LOG_LEVEL::ERR, format, args);
		va_end(args);
	}

private:

	void write(LOG_LEVEL level, const char * _format, ...);

	Logger(const char* name) 
	{
		mName = name;
	}
	~Logger() {}

	std::string mName;

	static std::map<std::string, Logger*> mInstances;
	static char mPath[MAX_PATH];
	static LOG_LEVEL mLevel;
	static bool mConsole;
};
