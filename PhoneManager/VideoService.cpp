#include "stdafx.h"
#include "VideoService.h"
extern "C" {
#include "libavutil/imgutils.h"
}

#define MAX_DBG_MSG_LEN (1024)
char pformat[MAX_DBG_MSG_LEN];

#define PLAY_URL "rtmp://192.168.8.244/live/screen"
//#define PLAY_URL "d:\\temp\\test.mp4"


VideoService::VideoService(void)
{
	isStop = true;
	isRunning = false;
	OutputDebugString("VideoService\n");	
}


VideoService::~VideoService(void)
{
	OutputDebugString("~VideoService\n");
}


void VideoService::start(D3DUtils *mD3DUtils)
{
	this->mD3DUtils = mD3DUtils;
	isStop = false;	
	pid_ffplay = CreateThread(NULL, 0, &VideoService::ffplayThread, this, CREATE_SUSPENDED, NULL);
	if (NULL!= pid_ffplay) {  
		ResumeThread(pid_ffplay);  
	}
}


DWORD CALLBACK VideoService::ffplayThread(LPVOID lp)
{
	OutputDebugString("VideoService ffplayThread start \n");	
	VideoService *mPtr = (VideoService *)lp;	
	isRunning = true;
	while(isStop==false)
	{
	    OutputDebugString("loop read next frame.\n");
		mPtr->ffplay();
		Sleep(40);		
	}
	isRunning = false;
	OutputDebugString("VideoService ffplayThread exit \n");	
	return 0;
}

int VideoService:: interrupt_cb(void *ctx)
{
	if(isStop){
		OutputDebugString("VideoService interrupt_cb \n");	
		return 1;
	}
    return 0;
}

DWORD VideoService::ffplay()
{
	OutputDebugString("VideoService ffplay thread start\n");
	uint8_t *sps;
	uint8_t *pps;
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx_video;
	AVCodecContext *pCodecCtx_audio;
	AVPacket *packet;
	AVFrame *frame;
	AVFrame *fly_frame;
	struct SwrContext* swr_cxt;
	uint8_t *audio_buf;

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();	
	pFormatCtx->interrupt_callback.callback = interrupt_cb;
	pFormatCtx->interrupt_callback.opaque = pFormatCtx;
	AVDictionary* avdic = NULL;
	av_dict_set(&avdic, "probesize", "32", 0);
	av_dict_set(&avdic, "max_analyze_duration", "100000", 0);
	int ret = avformat_open_input(&pFormatCtx, PLAY_URL, nullptr, &avdic);	
	av_dict_free(&avdic);
	if (ret != 0) {
		wsprintf(pformat,"VideoService Couldn't open url=%s, (ret:%d)\n", PLAY_URL, ret);
		OutputDebugString(pformat);
		return -1;
	}
	int totalSec = static_cast<int>(pFormatCtx->duration / AV_TIME_BASE);
	wsprintf(pformat,"VideoService video time  %dmin:%dsec\n", totalSec / 60, totalSec % 60);
	OutputDebugString(pformat);
	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
		OutputDebugString("VideoService Could't find stream infomation\n");
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
		OutputDebugString("VideoService no find vedio_stream\n");
		return -1;
	}

	int vfps = (int) ((double) (pFormatCtx->streams[videoStream]->avg_frame_rate.num) /(double) (pFormatCtx->streams[videoStream]->avg_frame_rate.den));
	wsprintf(pformat,"VideoService fps = %d\n",vfps);
	OutputDebugString(pformat);

	AVCodecParameters *pCodecPar_video = pFormatCtx->streams[videoStream]->codecpar;
	AVCodec *pCodec_video = avcodec_find_decoder(pCodecPar_video->codec_id);
	if (pCodec_video == nullptr) {
		OutputDebugString("VideoService not found decodec.\n");
		return -1;
	}
	pCodecCtx_video = avcodec_alloc_context3(pCodec_video);
	ret = avcodec_parameters_to_context(pCodecCtx_video, pCodecPar_video);
	if (ret < 0) {
		wsprintf(pformat,"VideoService avcodec_parameters_to_context() failed %d\n", ret);
		OutputDebugString(pformat);
		return -1;
	}
	if (avcodec_open2(pCodecCtx_video, pCodec_video, nullptr) < 0) {
		OutputDebugString("VideoService Could not open decodec.\n");
		return -1;
	}

	pCodecCtx_video->flags |=CODEC_FLAG_LOW_DELAY;

	//��Ƶʹ�����⣬��ʼ����Ƶ���룬��Ƶ���Ǳ������
	for (int i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStream = i;
			AVCodecParameters *pCodecPar_audio = pFormatCtx->streams[i]->codecpar;
			AVCodec *pCodec_audio = avcodec_find_decoder(pCodecPar_audio->codec_id);
			if (pCodec_audio != nullptr) {
				pCodecCtx_audio = avcodec_alloc_context3(pCodec_audio);
				ret = avcodec_parameters_to_context(pCodecCtx_audio, pCodecPar_audio);
				if (ret >= 0) {
					if (avcodec_open2(pCodecCtx_audio, pCodec_audio, nullptr) >= 0) {
						wsprintf(pformat,"VideoService find audioStream = %d, sampleRateInHz = %d, channelConfig=%d, audioFormat=%d\n", 
							i, pCodecCtx_audio->sample_rate, pCodecCtx_audio->channels,pCodecCtx_audio->sample_fmt);
						OutputDebugString(pformat);
						swr_cxt = swr_alloc();
						swr_alloc_set_opts(
							swr_cxt,
							out_channelConfig,						
							(AVSampleFormat) out_audioFormat,
							out_sampleRateInHz,
							pCodecCtx_audio->channel_layout,
							pCodecCtx_audio->sample_fmt,
							pCodecCtx_audio->sample_rate,
							0,
							nullptr);
						swr_init(swr_cxt);
						audio_buf = (uint8_t *) av_malloc(out_sampleRateInHz * 8);
						//callBack->javaOnAudioInfo(out_sampleRateInHz, out_channelConfig, out_audioFormat);
					} else {
						avcodec_close(pCodecCtx_audio);
						OutputDebugString("VideoService init audio codec failed 2!\n");
					}
				} else {
					OutputDebugString("VideoService init audio codec failed 3!\n");
				}
			} else {
				OutputDebugString("VideoService init audio codec failed 4!\n");
			}

			break;
		}
	}
	frame = av_frame_alloc();	
	packet = (AVPacket *) av_malloc(sizeof(AVPacket)); //����һ��packet

	int buf_size;
	uint8_t *buffer;
	static struct SwsContext *sws_ctx;
	fly_frame = av_frame_alloc();
	buf_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,pCodecCtx_video->width,pCodecCtx_video->height,1);
	buffer = (uint8_t *)av_malloc(buf_size);
	av_image_fill_arrays(fly_frame->data,           // dst data[]
		fly_frame->linesize,       // dst linesize[]
		buffer,                    // src buffer
		AV_PIX_FMT_YUV420P,        // pixel format
		pCodecCtx_video->width,        // width
		pCodecCtx_video->height,       // height
		1                          // align
		);
	sws_ctx = sws_getContext(pCodecCtx_video->width,    // src width
		pCodecCtx_video->height,   // src height
		pCodecCtx_video->pix_fmt,  // src format
		pCodecCtx_video->width,    // dst width
		pCodecCtx_video->height,   // dst height
		AV_PIX_FMT_YUV420P,    // dst format
		SWS_BICUBIC,           // flags
		NULL,                  // src filter
		NULL,                  // dst filter
		NULL                   // param
		);          

	while (!isStop ) {	
		ret = av_read_frame(pFormatCtx, packet);
		if(ret<0){
			wsprintf(pformat,"VideoService av_read_frame read error, ret=%d\n",ret);
			OutputDebugString(pformat);
			break;		
		}
		if (packet->stream_index == videoStream) {
			//������Ƶ
			//TODO::ʱ��ͬ��
			ret = avcodec_send_packet(pCodecCtx_video, packet);
			while (ret >= 0) {
				ret = avcodec_receive_frame(pCodecCtx_video, frame);
				if (ret >= 0) {
					sws_scale(sws_ctx,                                  // sws context
						(const uint8_t *const *)frame->data,  // src slice
						frame->linesize,                      // src stride
						0,                                        // src slice y
						pCodecCtx_video->height,                      // src slice height
						fly_frame->data,                          // dst planes
						fly_frame->linesize                       // dst strides
						);
					fly_frame->width = frame->width;
					fly_frame->height = frame->height;
					uint8_t * video_buf = (uint8_t *) malloc((fly_frame->width*fly_frame->height* 3 / 2) * sizeof(uint8_t));
					int start = 0;
					memcpy(video_buf,fly_frame->data[0],fly_frame->width*fly_frame->height);
					start=start+fly_frame->width*fly_frame->height;
					memcpy(video_buf + start,fly_frame->data[1],fly_frame->width*fly_frame->height/4);
					start=start+fly_frame->width*fly_frame->height/4;
					memcpy(video_buf + start,fly_frame->data[2],fly_frame->width*fly_frame->height/4);
					mD3DUtils->PushYUV(video_buf,frame->width,frame->height);					
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

	//	av_free(audio_buf);	
	av_free(packet);
	av_frame_free(&frame);
	avcodec_close(pCodecCtx_video);
	//	avcodec_close(pCodecCtx_audio);
	avformat_close_input(&pFormatCtx);	
	OutputDebugString("VideoService ffplay thread exit\n");
	return 0;
}

void VideoService::stop()
{
	isStop = true;
	Sleep(1);
	while (isRunning){
		OutputDebugString("Can't stop VideoService, because is Running...\n");
		Sleep(1000);
	}
}
