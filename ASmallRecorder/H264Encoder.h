#pragma once

#include <windows.h>
#include <mfapi.h>                                                              
#include <mferror.h>
#include <codecapi.h>
#include <strmif.h>
#include <winbase.h>
#include <mfidl.h>
#include "Log.h"

typedef DWORD (*processIn)(BYTE* inputBuffer, UINT32 size, LONGLONG& timestamp);
typedef DWORD (*processOut)(BYTE* outputBuffer, UINT32 size);

class H264Encoder
{
public:
	H264Encoder();
	
	~H264Encoder();

	bool initialize(unsigned int w, unsigned int h, unsigned int bitRate, unsigned int frameRate, processIn funcIn, processOut funcOut);

	bool start();

	void stop();

	bool destroy();

	static void workingThread(void* data);

protected:
	bool createEncoder();

	void destroyEncoder();

	bool setAttributes();

	bool setProperties();

	bool setupInputType();

	bool setupOutputType();

private:

	unsigned int mSrcWidth;
	unsigned int mSrcHeight;
	unsigned int mBitRate;
	unsigned int mFrameRate;

	unsigned int mDstWidth;
	unsigned int mDstHeight;
	unsigned char* mRGBBuffer;
	unsigned char* mYUVBuffer;
	unsigned int mMaxBufferSize;
	unsigned int mFixedInputDataLen;
	int mQValue;
	int mQVsp;
	int mGop;

	Logger*	mLogger;
	IMFTransform* mEncoder;
	IMFMediaEventGenerator	*mMediaEventGenerator;

	bool mStop;
	bool mRunning;
	processIn mProcessIn;
	processOut mProcessOut;
};