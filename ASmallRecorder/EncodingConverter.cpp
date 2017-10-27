#include "EncodingConverter.h"

bool EncodingConverter::init(int srcW, int srcH)
{
	bool bRet = false;
	do 
	{
		mWidth = srcW;
		mHeight = srcH;
		mLineIndex = mWidth * mHeight;
		mSrcLineSize[0] = mWidth * 4;
	} while (0);
	return bRet;
}

void EncodingConverter::destroy()
{

}

bool EncodingConverter::rgb32ToNV12(uint8_t* src, uint8_t* dst, uint32_t size)
{
	bool bRet = false;

	do 
	{
	} while (0);
	return bRet;
}