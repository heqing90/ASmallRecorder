#pragma once

class Utils
{
public:

	static int RGBA_SIZE(unsigned int w, unsigned int h) { return (w * h << 2); }

	template <class T> 
	static void safeRelease(T **ppT)
	{
		if (*ppT)
		{
			(*ppT)->Release();
			*ppT = NULL;
		}
	}
};

