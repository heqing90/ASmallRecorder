#include "ScreenCapture.h"

ScreenCapture::ScreenCapture()
{
	mWindow = ::GetDesktopWindow();
}

ScreenCapture::ScreenCapture(HWND hwnd)
{
	mWindow = hwnd;
}

ScreenCapture::~ScreenCapture()
{

}

bool ScreenCapture::get(int w, int h, char* pScreenBytes, unsigned int size)
{
	HDC hdcScreen;
	HDC hdcWindow;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;
	RECT rcClient;
	BITMAPINFOHEADER   bi;
	DWORD dwBmpSize;
	HANDLE hDIB;
	CHAR *lpbitmap;
	bool isSuccess = false;

	hdcScreen = GetDC(NULL);
	hdcWindow = GetDC(mWindow);
	hdcMemDC = CreateCompatibleDC(hdcWindow);

	if (!hdcMemDC)
	{
		goto done;
	}

	GetClientRect(mWindow, &rcClient);

	SetStretchBltMode(hdcWindow, HALFTONE);

	if (!StretchBlt(hdcWindow,
		0, 0,
		rcClient.right, rcClient.bottom,
		hdcScreen,
		0, 0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		SRCCOPY))
	{
		goto done;
	}

	hbmScreen = CreateCompatibleBitmap(
		hdcWindow,
		w,
		h
	);
	if (!hbmScreen)
	{
		goto done;
	}
	SelectObject(hdcMemDC, hbmScreen);
	if (!BitBlt(
		hdcMemDC,
		0, 0,
		w, h,
		hdcWindow,
		0, 0,
		SRCCOPY))
	{
		goto done;
	}

	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpScreen.bmWidth;
	bi.biHeight = -bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
	hDIB = GlobalAlloc(GHND, dwBmpSize);
	lpbitmap = (char *)GlobalLock(hDIB);

	GetDIBits(
		hdcWindow,
		hbmScreen,
		0,
		(UINT)bmpScreen.bmHeight,
		lpbitmap,
		(BITMAPINFO *)&bi,
		DIB_RGB_COLORS
	);
	memcpy(pScreenBytes, lpbitmap, dwBmpSize);
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);
	isSuccess = true;
done:
	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(NULL, hdcScreen);
	ReleaseDC(mWindow, hdcWindow);

	return isSuccess;
}

