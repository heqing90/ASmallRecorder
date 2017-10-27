#include "H264Encoder.h"
#include "Debug.h"
#include "Utils.h"
#include <thread>


H264Encoder::H264Encoder()
{
	mEncoder = NULL;
	mMediaEventGenerator = NULL;
	mStop = false;
	mRunning = false;
	mMaxBufferSize = 0;
	mFixedInputDataLen = 0;
}

H264Encoder::~H264Encoder()
{
	
}

void H264Encoder::workingThread(void* data)
{
	H264Encoder& encoder = *reinterpret_cast<H264Encoder*>(data);
	BYTE* pInputBuffer = NULL;
	BYTE* pOutputBuffer = NULL;
	DWORD dataLen = 0;

	encoder.mRunning = true;
	while (!encoder.mStop)
	{
		IMFMediaEvent* pEvent = NULL;
		MediaEventType eventType;
		HRESULT hr;

		hr = encoder.mMediaEventGenerator->GetEvent(MF_EVENT_FLAG_NO_WAIT, &pEvent);
		if (hr == MF_E_NO_EVENTS_AVAILABLE)
		{
			Sleep(5);
			continue;
		}
		if (FAILED(hr))
		{
			encoder.mLogger->error("Get media event failed. [error=%08x]", hr);
			break;
		}
		assert(NULL != pEvent);
		hr = pEvent->GetType(&eventType);
		pEvent->Release();
		if (FAILED(hr))
		{
			encoder.mLogger->error("Get event type failed.[error=%08x]", hr);
			break;
		}
		if (eventType == METransformNeedInput)
		{
			LONGLONG timestamp = 0;
			IMFSample* pInputSample = NULL;
			IMFMediaBuffer* pInputMediaBuffer = NULL;

			MFCreateMemoryBuffer(encoder.mFixedInputDataLen, &pInputMediaBuffer);
			assert(NULL != pInputMediaBuffer);
			if (NULL == pInputMediaBuffer)
			{
				encoder.mLogger->error("Create memory buffer failed. [error=%08x]", hr);
				break;
			}

			hr = MFCreateSample(&pInputSample);
			if (FAILED(hr))
			{
				pInputMediaBuffer->Release();
				encoder.mLogger->error("Create input sample failed. [error=%08x]", hr);
				break;
			}
			pInputSample->AddBuffer(pInputMediaBuffer);

			pInputMediaBuffer->Lock(&pInputBuffer, NULL, NULL);
			ZeroMemory(pInputBuffer, encoder.mFixedInputDataLen);
			dataLen = encoder.mProcessIn(pInputBuffer, encoder.mFixedInputDataLen, timestamp);
			assert(dataLen != 0);
			pInputMediaBuffer->Unlock();
			pInputMediaBuffer->SetCurrentLength(encoder.mFixedInputDataLen);
			pInputMediaBuffer->Release();
		
			hr = encoder.mEncoder->ProcessInput(0, pInputSample, 0);
			pInputSample->Release();
			if (FAILED(hr) && hr != MF_E_NOTACCEPTING)
			{
				encoder.mLogger->error("Process input data failed.");
				break;
			}
			if (hr == MF_E_NOTACCEPTING)
			{
				encoder.mLogger->warn("Cannot accept more data.");
				Sleep(50);
				continue;
			}
		}
		else if (eventType == METransformHaveOutput)
		{
			HRESULT hr = S_OK;
			MFT_OUTPUT_STREAM_INFO outputStreamInfo;

			hr = encoder.mEncoder->GetOutputStreamInfo(0, &outputStreamInfo);
			if (FAILED(hr))
			{
				encoder.mLogger->error("Cannot get output stream info. [error=%08x]", hr);
				break;
			}
			if (outputStreamInfo.dwFlags != MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)
			{
				encoder.mLogger->warn("Do not allocate the output samples.");
			}
			DWORD status = 0;
			MFT_OUTPUT_DATA_BUFFER outputDataBuffer = {};
			hr = encoder.mEncoder->ProcessOutput(0, 1, &outputDataBuffer, &status);
			if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
			{
				encoder.mLogger->warn("Need more input data.");
				continue;
			}
			if (FAILED(hr)) 
			{
				encoder.mLogger->error("Process output failed. [error=%08x]", hr);
			}
			if (outputDataBuffer.pEvents)
			{
				outputDataBuffer.pEvents->Release();
			}

			IMFMediaBuffer *pOutputMediaBuffer = NULL;
			hr = outputDataBuffer.pSample->GetBufferByIndex(0, &pOutputMediaBuffer);
			if (FAILED(hr))
			{
				encoder.mLogger->error("Failed to get buffer from output sample. [error=%d]", hr);
				break;
			}
			hr = pOutputMediaBuffer->GetCurrentLength(&dataLen);
			if (FAILED(hr))
			{
				encoder.mLogger->error("Failed to get length of output media buffer [error=%d]", hr);
				break;
			}

			pOutputMediaBuffer->Lock(&pOutputBuffer, NULL, NULL);
			encoder.mProcessOut(pOutputBuffer, dataLen);
			pOutputMediaBuffer->Unlock();
			pOutputMediaBuffer->SetCurrentLength(0);
			pOutputMediaBuffer->Release();
			outputDataBuffer.pSample->Release();
		}
		else if (eventType == METransformDrainComplete)
		{
			encoder.mLogger->info("Happy Ending.");
			break;
		}
		else if (eventType == MEError)
		{
			encoder.mLogger->error("fatal error when get event from encoder.");
			break;
		}
	}
	encoder.mRunning = false;
}

bool H264Encoder::initialize(unsigned int w, unsigned int h, unsigned int bitRate, unsigned int frameRate, processIn funcIn, processOut funcOut)
{
	bool bRet = false;

	assert(NULL != funcIn && NULL != funcOut);
	mProcessIn = funcIn;
	mProcessOut = funcOut;
	mSrcWidth = w;
	mSrcHeight = h;
	mDstWidth = (w + 15) & ~15;
	mDstHeight = (h + 15) & ~15;
	mMaxBufferSize = mDstWidth * mDstHeight * 4;
	mFixedInputDataLen = mDstHeight * mDstHeight * 3 / 2;
	mBitRate = bitRate;
	mFrameRate = frameRate;
	mQVsp = 34;
	mGop = 150;
	mQValue = 75;
	if (bitRate <= 1000000) 
		mQValue = 0;
	else if (bitRate <= 2000000) 
		mQValue = 25;
	else if (bitRate <= 4000000) 
		mQValue = 50;
	else if (bitRate <= 8000000) 
		mQValue = 100;
	
	mLogger = Logger::getLogger("H264Encoder");
	assert(NULL != mLogger);
	do
	{
		if (!createEncoder())
		{
			mLogger->error("Create encoder failed.");
			break;
		}
		if (!setAttributes())
		{
			mLogger->error("Set attributes failed.");
			break;
		}
		if (!setProperties())
		{
			mLogger->error("Set properties failed.");
			break;
		}
		if (!setupOutputType())
		{
			mLogger->error("setup output media type failed.");
			break;
		}
		if (!setupInputType())
		{
			mLogger->error("setup input media type failed.");
			break;
		}
		bRet = true;
	} while (0);
	if (!bRet)
	{
		destroy();
	}
	return bRet;
}


bool H264Encoder::start()
{
	HRESULT hr;
	
	assert(NULL != mEncoder);
	hr = mEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
	assert(hr == S_OK);
	hr = mEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);
	assert(hr == S_OK);
	hr = mEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);
	assert(hr == S_OK);

	std::thread worker(H264Encoder::workingThread, this);
	//worker.detach();
	return true;
}

void H264Encoder::stop()
{
	HRESULT hr;
	
	assert(NULL != mEncoder);
	hr = mEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, NULL);
	assert(hr == S_OK);
	hr = mEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, NULL);
	assert(hr == S_OK);
	while (mRunning)
	{
		Sleep(25);
	}
}

bool H264Encoder::destroy()
{
	destroyEncoder();
	return true;
}


bool H264Encoder::createEncoder()
{
	UINT32 nCount = 0;
	IMFActivate **ppActivate = NULL;
	HRESULT hr = S_OK;
	MFT_REGISTER_TYPE_INFO transformOutput = { MFMediaType_Video, MFVideoFormat_H264 };
	do 
	{
		hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, MFT_ENUM_FLAG_HARDWARE, NULL, &transformOutput, &ppActivate, &nCount);
		if (FAILED(hr))
		{
			mLogger->error("Enum hardware encoder failed [error=%08x]", hr);
			break;
		}
		if (nCount == 0)
		{
			mLogger->error("Cannot find hardware encoder device.");
			break;
		}
		assert(ppActivate != NULL);
		hr = ppActivate[0]->ActivateObject(IID_PPV_ARGS(&mEncoder));
		if (SUCCEEDED(hr))
		{
			mLogger->info("Succeeded to create an encoder.");
		}
		else
		{
			mLogger->error("failed to create an encoder. [error=%08x]", hr);
		}
		for (UINT32 i =0; i < nCount; i++)
		{
			ppActivate[i]->Release();
		}
		CoTaskMemFree(ppActivate);
	} while (0);

	return hr == S_OK;
}

bool H264Encoder::setAttributes()
{
	UINT32 isAsync = 0;
	IMFAttributes *pAttributes = NULL;
	DWORD nInputStreamsCount = 0;
	DWORD nOutputStreamsCount = 0;
	HRESULT hr = S_OK;
	do 
	{
		hr = mEncoder->GetAttributes(&pAttributes);
		if (FAILED(hr))
		{
			mLogger->error("Get attributes of encoder failed [error=%08x]", hr);
			break;
		}
		hr = pAttributes->GetUINT32(MF_TRANSFORM_ASYNC, &isAsync);
		if (FAILED(hr))
		{
			mLogger->error("Get transform async prop failed. [error=%08x]", hr);
			break;
		}
		if (!isAsync)
		{
			mLogger->error("Not support async mode.");
			break;
		}
		hr = pAttributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE);
		if (FAILED(hr))
		{
			mLogger->error("Set uint32 prop failed [error=%08x]", hr);
			break;
		}
		hr = pAttributes->SetUINT32(CODECAPI_AVLowLatencyMode, TRUE);
		if (FAILED(hr))
		{
			mLogger->error("Set uint32 prop failed [error=%08x]", hr);
			break;
		}
		hr = mEncoder->GetStreamCount(&nInputStreamsCount, &nOutputStreamsCount);
		if (FAILED(hr))
		{
			mLogger->error("Cannot get stream count. [error=%08x]", hr);
			break;
		}
		if (nInputStreamsCount != 1 || nOutputStreamsCount != 1)
		{
			mLogger->error("Encoder must have one input stream one on output stream.");
			hr = E_FAIL;
			break;
		}
		hr = mEncoder->QueryInterface(IID_IMFMediaEventGenerator, reinterpret_cast<void**>(&mMediaEventGenerator));
		if (FAILED(hr))
		{
			mLogger->error("Cannot query the interface of media event generator. [error=%08x]", hr);
			break;
		}
	} while (0);
	if (pAttributes)
	{
		pAttributes->Release();
	}
	return hr == S_OK;
}

bool H264Encoder::setProperties()
{
	HRESULT hr = S_OK;
	ICodecAPI* pCodecApi = NULL;
	VARIANT var;

	do 
	{
		hr = mEncoder->QueryInterface<ICodecAPI>(&pCodecApi);
		if (FAILED(hr))
		{
			mLogger->error("Cannot query the interface of codec [error=%08x]", hr);
			break;
		}
		VariantInit(&var);
		var.vt = VT_UI4;
		var.ulVal = mQVsp;
		hr = pCodecApi->SetValue(&CODECAPI_AVEncCommonQualityVsSpeed, &var);
		if (SUCCEEDED(hr))
		{
			mLogger->info("Set common quality vs speed success: %d", mQValue);
		}
		VariantClear(&var);

		VariantInit(&var);
		var.vt = VT_UI4;
		var.ulVal = VARIANT_TRUE;
		hr = pCodecApi->SetValue(&CODECAPI_AVEncCommonRealTime, &var);
		if (SUCCEEDED(hr))
		{
			mLogger->info("Set common real time success: %d", var.ullVal);
		}
		VariantClear(&var);

		VariantInit(&var);
		pCodecApi->GetValue(&CODECAPI_AVEncMPVGOPSize, &var);
		var.vt = VT_UI4;
		var.ulVal = mGop;
		hr = pCodecApi->SetValue(&CODECAPI_AVEncMPVGOPSize, &var);
		if (SUCCEEDED(hr))
		{
			mLogger->info("Set gop size success.");
		}
		VariantClear(&var);
	} while (0);
	if (NULL != pCodecApi)
	{
		pCodecApi->Release();
	}
	return hr == S_OK;
}

bool H264Encoder::setupInputType()
{
	HRESULT hr = S_OK;
	IMFMediaType* pInputMediaType = NULL;
	do 
	{
		hr = ::MFCreateMediaType(&pInputMediaType);
		if (FAILED(hr))
		{
			mLogger->error("Create media type failed [error=%08x]", hr);
			break;
		}
		hr = pInputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		if (FAILED(hr))
		{
			mLogger->error("Set GUID for input major type failed [error=%08x]", hr);
			break;
		}
		hr = pInputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
		if (FAILED(hr))
		{
			mLogger->error("Set GUID for input sub type failed [error=%08x]", hr);
			break;
		}
		hr = pInputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
		if (FAILED(hr))
		{
			mLogger->error("Set interlace mode failed [error=%08x]", hr);
			break;
		}
		hr = ::MFSetAttributeSize(pInputMediaType, MF_MT_FRAME_SIZE, mDstWidth, mDstHeight);
		if (FAILED(hr))
		{
			mLogger->error("Set frame size failed. [error=%08x]", hr);
			break;
		}
		hr = ::MFSetAttributeRatio(pInputMediaType, MF_MT_FRAME_RATE, mFrameRate, 1);
		if (FAILED(hr))
		{
			mLogger->error("Set frame rate failed [error=%08x]", hr);
			break;
		}
		hr = ::MFSetAttributeRatio(pInputMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
		if (FAILED(hr))
		{
			mLogger->error("Set pixel aspect ratio failed [error=%08x]", hr);
			break;
		}
		hr = mEncoder->SetInputType(0, pInputMediaType, 0);
		if (FAILED(hr))
		{
			mLogger->error("Set input type for encoder failed [error=%08x]", hr);
			break;
		}
	} while (0);
	if (pInputMediaType)
	{
		pInputMediaType->Release();
	}
	return hr == S_OK;
}

bool H264Encoder::setupOutputType()
{
	HRESULT hr = S_OK;
	IMFMediaType* pOutputMediaType = NULL;
	do 
	{
		hr = ::MFCreateMediaType(&pOutputMediaType);
		if (FAILED(hr))
		{
			mLogger->error("Create media type for output type failed.[error=%08x]", hr);
			break;
		}
		hr = pOutputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		if (FAILED(hr))
		{
			mLogger->error("Set GUID for input major type failed [error=%08x]", hr);
			break;
		}
		hr = pOutputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
		if (FAILED(hr))
		{
			mLogger->error("Set GUID for input sub type failed [error=%08x]", hr);
			break;
		}
		hr = pOutputMediaType->SetUINT32(MF_MT_AVG_BITRATE, mBitRate);
		if (FAILED(hr))
		{
			mLogger->error("Set AVG bit rate failed. [error=%08x]", hr);
			break;
		}
		hr = ::MFSetAttributeRatio(pOutputMediaType, MF_MT_FRAME_RATE, mFrameRate, 1);
		if (FAILED(hr))
		{
			mLogger->error("Set frame rate for output media type failed. [error=%08x]", hr);
			break;
		}
		hr = ::MFSetAttributeSize(pOutputMediaType, MF_MT_FRAME_SIZE, mDstWidth, mDstHeight);
		if (FAILED(hr))
		{
			mLogger->error("Set frame size for out put media type failed. [error=%08x]", hr);
		}
		hr = ::MFSetAttributeRatio(pOutputMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
		if (FAILED(hr))
		{
			mLogger->error("Set pixel aspect ratio for output media type failed [error=%08x]", hr);
			break;
		}
		hr = pOutputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
		if (FAILED(hr))
		{
			mLogger->error("Set interlace mode failed for output media type.[error=%08x]", hr);
			break;
		}
		hr = pOutputMediaType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base);
		if (FAILED(hr))
		{
			mLogger->error("Set MPEG2 profile failed for output media type.[error=%08x]", hr);
			break;
		}
		hr = mEncoder->SetOutputType(0, pOutputMediaType, 0);
		if (FAILED(hr))
		{
			mLogger->error("Set output type failed.[error=%08x]", hr);
			break;
		}
	} while (0);
	if (pOutputMediaType)
	{
		pOutputMediaType->Release();
	}
	return hr == S_OK;
}

void H264Encoder::destroyEncoder()
{
	Utils::safeRelease(&mMediaEventGenerator);
	if (NULL != mEncoder)
	{
		IMFShutdown* pIMFShutdown = NULL;
		mEncoder->QueryInterface(&pIMFShutdown);
		if (pIMFShutdown)
		{
			pIMFShutdown->Shutdown();
			pIMFShutdown->Release();
		}
	}
	Utils::safeRelease(&mEncoder);
}