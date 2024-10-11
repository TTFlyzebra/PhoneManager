#pragma once

#include "Dxva2D3DUtils.h"

class VideoService
{
public:
	VideoService();
	~VideoService(void);	
	void start(HWND hwnd,Dxva2D3DUtils *mDxva2D3DUtils, int my_num);
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

	Dxva2D3DUtils *mDxva2D3DUtils;
	HWND mHwnd;

	bool isRunning;
	bool isStop;
};

