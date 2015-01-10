#ifndef __RENDERER_HEADER__
#define __RENDERER_HEADER__

#include <string>
#include <cstdio>
#include <vector>
#include "audio.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#define MP3_BUF_SIZE 4096

struct FFMpegAudioFile {
	AVCodec* codec;
	AVCodecContext* context;
	AVFormatContext* pFormatCtx;
	AVFrame* frame;
	SwrContext* resContext;
};



#endif // __RENDERER_HEADER__