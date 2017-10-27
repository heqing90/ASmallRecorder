#pragma once
#include <windows.h>


class ScreenCapture
{
public:
	ScreenCapture();
	ScreenCapture(HWND hwnd);

	~ScreenCapture();

	bool get(int w, int h, char* pScreenBytes, unsigned int size);

protected:

private:

	HWND mWindow;
};
