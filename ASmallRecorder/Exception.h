#pragma once

#include <exception>
#include <string>

class OpenFileFailed : public std::exception
{
public:
	OpenFileFailed(std::string desc) : exception(desc.c_str())
	{

	}

	virtual const char *what() const {
		return exception::what();
	}
};