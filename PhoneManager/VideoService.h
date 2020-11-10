extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
};
#include "D3DUtils.h"
#pragma once

static bool isRunning;
static bool isStop;

class VideoService
{
public:
	VideoService(HWND hwnd);
	~VideoService(void);
	static int VideoService::interrupt_cb(void *ctx);
	void start(D3DUtils *mD3DUtils);
	void stop();
private:    

    uint16_t out_sampleRateInHz;
    uint16_t out_channelConfig;
    uint16_t out_audioFormat;

	HANDLE pid_ffplay;  
	static DWORD CALLBACK ffplayThread(LPVOID);
	DWORD ffplay();

	D3DUtils *mD3DUtils;
	HWND mHwnd;
};

