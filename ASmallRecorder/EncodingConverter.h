#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
#include "libavcodec/dxva2.h"
#ifdef __cplusplus
}
#endif

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")

class EncodingConverter
{
public:

	bool init(int srcW, int srcH);
	
	void destroy();

	bool rgb32ToNV12(uint8_t* src, uint8_t* dst, uint32_t size);

private:
	SwsContext*	mSwsContext;
	int mWidth;
	int mHeight;
	int mLineIndex;
	int mSrcLineSize[4];
	int mDstLineSize[4];
};