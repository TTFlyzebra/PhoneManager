#include "stdafx.h"
#include "VideoService.h"
#include "dxva.h"
#include "FlyTools.h"
extern "C" {
#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
};


#define PLAY_URL "rtmp://192.168.1.88/live/screen_%d"


VideoService::VideoService()
{
	
	isStop = true;
	isRunning = false;
	TRACE("VideoService\n");	
}


VideoService::~VideoService(void)
{
	TRACE("~VideoService\n");
}


void VideoService::start(HWND hwnd, Dxva2D3DUtils *mDxva2D3DUtils,int my_num)
{
	this->mHwnd = hwnd;
	this->mDxva2D3DUtils = mDxva2D3DUtils;
	this->myNUM = my_num;
	isStop = false;	
	pid_ffplay = CreateThread(NULL, 0, &VideoService::ffplayThread, this, CREATE_SUSPENDED, NULL);
	if (NULL!= pid_ffplay) 
	{  
		ResumeThread(pid_ffplay);  
	}
}


DWORD CALLBACK VideoService::ffplayThread(LPVOID lp)
{
	TRACE("VideoService ffplayThread start \n");	
	VideoService *mPtr = (VideoService *)lp;	
	mPtr->isRunning = true;
	while(mPtr->isStop==false)
	{
	    TRACE("loop read next frame.\n");
		mPtr->ffplay();
		Sleep(40);		
	}
	mPtr->isRunning = false;
	TRACE("VideoService ffplayThread exit \n");	
	return 0;
}

int VideoService:: interrupt_cb(void *ctx)
{
	VideoService *mPtr = (VideoService *)ctx;	
	if(mPtr->isStop)
	{
		TRACE("VideoService interrupt_cb \n");	
		return 1;
	}
    return 0;
}

DWORD VideoService::ffplay()
{
	TRACE("VideoService ffplay thread start\n");
	uint8_t *sps;
	uint8_t *pps;
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx_video;

	AVPacket *packet;
	AVFrame *frame;
	AVFrame *fly_frame;


	av_register_all();
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();	
	pFormatCtx->interrupt_callback.callback = interrupt_cb;
	pFormatCtx->interrupt_callback.opaque = pFormatCtx;
	AVDictionary* avdic = NULL;
	//av_dict_set(&avdic, "probesize", "32", 0);
	//av_dict_set(&avdic, "max_analyze_duration", "100000", 0);
	char playurl[1024];
	memset(playurl,0,1024);
	sprintf(playurl,PLAY_URL,myNUM);
	TRACE("VideoService Couldn't open url=%s\n", playurl);
	int ret =  avformat_open_input(&pFormatCtx, playurl, nullptr, &avdic);	
	
	//av_dict_free(&avdic);
	if (ret != 0) {
		TRACE("VideoService Couldn't open url=%s, (ret:%d)\n", playurl, ret);
		return -1;
	}
	int totalSec = static_cast<int>(pFormatCtx->duration / AV_TIME_BASE);
	TRACE("VideoService video time  %dmin:%dsec\n", totalSec / 60, totalSec % 60);
	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
		TRACE("VideoService Could't find stream infomation\n");
		return -1;
	}

	int videoStream = -1;
	int audioStream = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}

	if (videoStream == -1) {
		TRACE("VideoService no find vedio_stream\n");
		return -1;
	}

	int vfps = (int) ((double) (pFormatCtx->streams[videoStream]->avg_frame_rate.num) /(double) (pFormatCtx->streams[videoStream]->avg_frame_rate.den));
	TRACE("VideoService fps = %d\n",vfps);
	AVCodecParameters *pCodecPar_video = pFormatCtx->streams[videoStream]->codecpar;
	AVCodec *pCodec_video = avcodec_find_decoder(pCodecPar_video->codec_id);
	if (pCodec_video == nullptr) {
		TRACE("VideoService not found decodec.\n");
		return -1;
	}

	pCodecCtx_video = avcodec_alloc_context3(pCodec_video);
	ret = avcodec_parameters_to_context(pCodecCtx_video, pCodecPar_video);
	if (ret < 0) {
		TRACE("VideoService avcodec_parameters_to_context() failed %d\n", ret);
		return -1;
	}
	if (avcodec_open2(pCodecCtx_video, pCodec_video, nullptr) < 0) {
		TRACE("VideoService Could not open decodec.\n");
		return -1;
	}

	pCodecCtx_video->flags |=CODEC_FLAG_LOW_DELAY;	

	//AVCodecContext *pCodecCtx_audio;
	//uint8_t *audio_buf;
	//struct SwrContext* swr_cxt;
	//音频使用软解，初始化音频解码，音频不是必须存在
	//for (int i = 0; i < pFormatCtx->nb_streams; i++) {
	//	if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
	//		audioStream = i;
	//		AVCodecParameters *pCodecPar_audio = pFormatCtx->streams[i]->codecpar;
	//		AVCodec *pCodec_audio = avcodec_find_decoder(pCodecPar_audio->codec_id);
	//		if (pCodec_audio != nullptr) {
	//			pCodecCtx_audio = avcodec_alloc_context3(pCodec_audio);
	//			ret = avcodec_parameters_to_context(pCodecCtx_audio, pCodecPar_audio);
	//			if (ret >= 0) {
	//				if (avcodec_open2(pCodecCtx_audio, pCodec_audio, nullptr) >= 0) {
	//					TRACE("VideoService find audioStream = %d, sampleRateInHz = %d, channelConfig=%d, audioFormat=%d\n", 
	//						i, pCodecCtx_audio->sample_rate, pCodecCtx_audio->channels,pCodecCtx_audio->sample_fmt);
	//					TRACE(pformat);
	//					swr_cxt = swr_alloc();
	//					swr_alloc_set_opts(
	//						swr_cxt,
	//						out_channelConfig,						
	//						(AVSampleFormat) out_audioFormat,
	//						out_sampleRateInHz,
	//						pCodecCtx_audio->channel_layout,
	//						pCodecCtx_audio->sample_fmt,
	//						pCodecCtx_audio->sample_rate,
	//						0,
	//						nullptr);
	//					swr_init(swr_cxt);
	//					audio_buf = (uint8_t *) av_malloc(out_sampleRateInHz * 8);
	//					//callBack->javaOnAudioInfo(out_sampleRateInHz, out_channelConfig, out_audioFormat);
	//				} else {
	//					avcodec_close(pCodecCtx_audio);
	//					TRACE("VideoService init audio codec failed 2!\n");
	//				}
	//			} else {
	//				TRACE("VideoService init audio codec failed 3!\n");
	//			}
	//		} else {
	//			TRACE("VideoService init audio codec failed 4!\n");
	//		}
	//
	//		break;
	//	}
	//}
	frame = av_frame_alloc();	
	packet = (AVPacket *) av_malloc(sizeof(AVPacket)); //分配一个packet

	//int buf_size;
	//uint8_t *buffer;
	//static struct SwsContext *sws_ctx;
	//fly_frame = av_frame_alloc();
	//int width = pCodecCtx_video->width;
	//int height = pCodecCtx_video->height;
	//width = width+(64-width%64);
	//buf_size = av_image_get_buffer_size(
	//	AV_PIX_FMT_YUV420P,
	//	width,
	//	height,
	//	1
	//	);
	//buffer = (uint8_t *)av_malloc(buf_size);
	//av_image_fill_arrays(
	//	fly_frame->data,           // dst data[]
	//	fly_frame->linesize,       // dst linesize[]
	//	buffer,                    // src buffer
	//	AV_PIX_FMT_YUV420P,        // pixel format
	//	width,    // width
	//	height,   // height
	//	1                          // align
	//	);
	//sws_ctx = sws_getContext(pCodecCtx_video->width,    // src width
	//	pCodecCtx_video->height,   // src height
	//	pCodecCtx_video->pix_fmt,  // src format
	//	width,    // dst width
	//	height,   // dst height
	//	AV_PIX_FMT_YUV420P,    // dst format
	//	SWS_BICUBIC,           // flags
	//	NULL,                  // src filter
	//	NULL,                  // dst filter
	//	NULL                   // param
	//	);  

	BOOL bRet = mDxva2D3DUtils->HWAccelInit(pCodec_video, pCodecCtx_video, mHwnd);
	if (!bRet){
		TRACE("VideoService HWAccelInit error.\n");
	}

	while (!isStop ) {	
		ret = av_read_frame(pFormatCtx, packet);
		if(ret<0){
			TRACE("VideoService av_read_frame read error, ret=%d\n",ret);
			break;		
		}
		if (packet->stream_index == videoStream) {
			//软解视频
			//TODO::时间同步
			ret = avcodec_send_packet(pCodecCtx_video, packet);
			while (ret >= 0) {
				ret = avcodec_receive_frame(pCodecCtx_video, frame);
				if (ret >= 0) {
					//硬解
					mDxva2D3DUtils->dxva2_retrieve_data_call(pCodecCtx_video, frame, myNUM);
					//sws_scale(sws_ctx,                                  // sws context
					//	(const uint8_t *const *)frame->data,  // src slice
					//	frame->linesize,                      // src stride
					//	0,                                        // src slice y
					//	pCodecCtx_video->height,                      // src slice height
					//	fly_frame->data,                          // dst planes
					//	fly_frame->linesize                       // dst strides
					//	);			
					//uint8_t * video_buf = (uint8_t *) malloc((width*height* 4) * sizeof(uint8_t));				
					//yuv420pToRGB32(frame->data[0],frame->data[1],frame->data[2],width,height,width,video_buf);
					//mDxva2D3DUtils->RenderRGB32(video_buf,width,height,width*height*4, myNUM);	
					//free(video_buf);
				}
			}
		} else if (packet->stream_index == audioStream) {
			//ret = avcodec_send_packet(pCodecCtx_audio, packet);
			//while (ret >= 0) {
			//	ret = avcodec_receive_frame(pCodecCtx_audio, frame);
			//	if (ret >= 0) {
			//		int len = swr_convert(
			//			swr_cxt,
			//			&audio_buf,
			//			frame->nb_samples,
			//			(const uint8_t **) frame->data,
			//			frame->nb_samples);
			//		int out_buf_size = av_samples_get_buffer_size(
			//			NULL,
			//			av_get_channel_layout_nb_channels(out_channelConfig),
			//			frame->nb_samples,
			//			(AVSampleFormat) out_audioFormat,
			//			0);
			//		int size = out_buf_size * out_sampleRateInHz / frame->sample_rate;
			//	}
			//}
		}
		av_packet_unref(packet);
	}

	//av_free(audio_buf);	
	//mDxva2D3DUtils->dxva2_uninit(pCodecCtx_video);
	av_free(packet);
	av_frame_free(&frame);
	avcodec_close(pCodecCtx_video);
	//avcodec_close(pCodecCtx_audio);
	avformat_close_input(&pFormatCtx);	
	TRACE("VideoService ffplay thread exit\n");
	return 0;
}

void VideoService::stop()
{
	isStop = true;
	while (isRunning){
		TRACE("Can't stop VideoService, because is Running...\n");
		Sleep(10);
	}
}

