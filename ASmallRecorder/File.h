#pragma once
#include <cstdlib>
#include <cstdio>
#include "Exception.h"

class QFile
{
public:
	QFile(const char* path, const char* mode) 
	{
		mFd = fopen(path, mode);
		if (!mFd)
		{
			throw OpenFileFailed("Cannot open file");
		}
	}

	~QFile() 
	{
		if (mFd)
		{
			fflush(mFd);
			fclose(mFd);
			mFd = NULL;
		}
	}

	operator FILE* () const { return mFd; }

private:
	FILE*	mFd;
};