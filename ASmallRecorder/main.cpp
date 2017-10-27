#include "H264Encoder.h"
#include "Log.h"
#include "Debug.h"
#include <csignal>

bool gIsRunning = false;

DWORD processInput(BYTE* inputBuffer, UINT32 size, LONGLONG& timestamp)
{
	assert(NULL != inputBuffer && size != 0);
	return 0;
}

DWORD processOutput(BYTE* outputBuffer, UINT32 size)
{
	assert(NULL != outputBuffer && size != 0);
	return 0;
}

BOOL WINAPI handlerRoutine(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		gIsRunning = false;
		break;
	default:
		break;
	}
	return 1;
}


int main(int argc, const char** argv)
{
	SetConsoleCtrlHandler(handlerRoutine, true);
	Logger::setup(".\\MySmallRecorder.log", LOG_LEVEL::DEBUG, true);

	H264Encoder h264Encoder;
	int w = 1920;
	int h = 1080;
	int bitRate = 8000000;
	int frameRate = 25;

	HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	assert(SUCCEEDED(hr));
	hr = ::MFStartup(MF_VERSION);
	assert(SUCCEEDED(hr));

	bool bRet = h264Encoder.initialize(w, h, bitRate, frameRate, processInput, processOutput);
	if (bRet)
	{
		if (h264Encoder.start())
		{
			gIsRunning = true;
			while (gIsRunning)
			{
				Sleep(25);
			}
			h264Encoder.stop();
		}
		h264Encoder.destroy();
	}

	::MFShutdown();
	::CoUninitialize();

	Logger::destroy();
	return 0;
}
