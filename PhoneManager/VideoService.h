#include "D3DUtils.h"
#pragma once

static bool isRunning;
static bool isStop;

class VideoService
{
public:
	VideoService();
	~VideoService(void);	
	void start(HWND hwnd,D3DUtils *mD3DUtils, int my_num);
	void stop();
private:    
	int myNUM;
    uint16_t out_sampleRateInHz;
    uint16_t out_channelConfig;
    uint16_t out_audioFormat;

	HANDLE pid_ffplay;  
	static int VideoService::interrupt_cb(void *ctx);
	static DWORD CALLBACK ffplayThread(LPVOID);
	DWORD ffplay();

	D3DUtils *mD3DUtils;
	HWND mHwnd;
};

