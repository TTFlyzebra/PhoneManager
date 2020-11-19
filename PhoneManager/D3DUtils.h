#include <d3dx9.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
};
#pragma once

#pragma comment(lib,"d3d9.lib") 
#pragma comment(lib,"winmm.lib") 
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"yuv.lib") 

#define MAX_NUM 14
#define D3DFVF_CUSTOMVERTEX   (D3DFVF_XYZ|D3DFVF_TEX1)
typedef unsigned __int8 uint8_t;  

enum HWAccelID {
    HWACCEL_NONE = 0,
    HWACCEL_AUTO,
    HWACCEL_VDPAU,
    HWACCEL_DXVA2,
    HWACCEL_VDA,
    HWACCEL_VIDEOTOOLBOX,
    HWACCEL_QSV,
};

typedef struct AVStream AVStream;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVCodec AVCodec;
typedef struct AVFrame AVFrame;
typedef struct AVDictionary AVDictionary;

typedef struct InputStream {
    int file_index;
    AVStream *st;
    int discard;             /* true if stream data should be discarded */
    int user_set_discard;
    int decoding_needed;     /* non zero if the packets must be decoded in 'raw_fifo', see DECODING_FOR_* */
#define DECODING_FOR_OST    1
#define DECODING_FOR_FILTER 2

    AVCodecContext *dec_ctx;
    AVCodec *dec;
    AVFrame *decoded_frame;
    AVFrame *filter_frame; /* a ref of decoded_frame, to be sent to filters */

    int64_t       start;     /* time when read started */
    /* predicted dts of the next packet read for this stream or (when there are
        * several frames in a packet) of the next frame in current packet (in AV_TIME_BASE units) */
    int64_t       next_dts;
    int64_t       dts;       ///< dts of the last packet read for this stream (in AV_TIME_BASE units)

    int64_t       next_pts;  ///< synthetic pts for the next decode frame (in AV_TIME_BASE units)
    int64_t       pts;       ///< current pts of the decoded frame  (in AV_TIME_BASE units)
    int           wrap_correction_done;

    int64_t filter_in_rescale_delta_last;

    int64_t min_pts; /* pts with the smallest value in a current stream */
    int64_t max_pts; /* pts with the higher value in a current stream */
    int64_t nb_samples; /* number of samples in the last decoded audio frame before looping */

    double ts_scale;
    int saw_first_ts;
    int showed_multi_packet_warning;
    AVDictionary *decoder_opts;
    AVRational framerate;               /* framerate forced with -r */
    int top_field_first;
    int guess_layout_max;

    int autorotate;
    int resample_height;
    int resample_width;
    int resample_pix_fmt;

    int      resample_sample_fmt;
    int      resample_sample_rate;
    int      resample_channels;
    uint64_t resample_channel_layout;

    int fix_sub_duration;
    struct { /* previous decoded subtitle and related variables */
        int got_output;
        int ret;
        AVSubtitle subtitle;
    } prev_sub;

    struct sub2video {
        int64_t last_pts;
        int64_t end_pts;
        AVFrame *frame;
        int w, h;
    } sub2video;

    int dr1;

    /* decoded data from this stream goes into all those filters
        * currently video and audio only */
    //InputFilter **filters;
    //int        nb_filters;

    //int reinit_filters;

    /* hwaccel options */
    enum HWAccelID hwaccel_id;
    char  *hwaccel_device;

    /* hwaccel context */
    enum HWAccelID active_hwaccel_id;
    void  *hwaccel_ctx;
    void(*hwaccel_uninit)(AVCodecContext *s);
    int(*hwaccel_get_buffer)(AVCodecContext *s, AVFrame *frame, int flags);
    int(*hwaccel_retrieve_data)(AVCodecContext *s, AVFrame *frame);
    enum AVPixelFormat hwaccel_pix_fmt;
    enum AVPixelFormat hwaccel_retrieved_pix_fmt;

    /* stats */
    // combined size of all the packets read
    uint64_t data_size;
    /* number of packets successfully read for this stream */
    uint64_t nb_packets;
    // number of frames/samples retrieved from the decoder
    uint64_t frames_decoded;
    uint64_t samples_decoded;
} InputStream;

struct CUSTOMVERTEX
{
    FLOAT x, y, z;       //顶点位置  
    FLOAT u, v ;         //顶点纹理坐标
};


class D3DUtils
{
public:
	D3DUtils(void);
	~D3DUtils(void);
	HRESULT InitD3D( HWND hWnd, int width, int height );
	void Cleanup();
	void RenderRGB32(uint8_t *yuv,int widht, int height, int size, int num);

	BOOL HWAccelInit(AVCodec *codec, AVCodecContext *ctx, HWND hWnd);
	int dxva2_init(AVCodecContext *s, HWND hwnd);
	int dxva2_retrieve_data_call(AVCodecContext *s, AVFrame *frame, int num);
	int dxva2_alloc(AVCodecContext *s, HWND hwnd);	
	
	static AVPixelFormat GetHwFormat(AVCodecContext *s, const AVPixelFormat *pix_fmts);
	static void dxva2_uninit(AVCodecContext *s);
    static int dxva2_get_buffer(AVCodecContext *s, AVFrame *frame, int flags);
	static void dxva2_release_buffer(void *opaque, uint8_t *data);
	

private:
	LPDIRECT3D9             g_pD3D;    //Direct3D对象
	LPDIRECT3DDEVICE9       g_pd3dDevice;    //Direct3D设备对象
	LPDIRECT3DVERTEXBUFFER9 g_pVB[MAX_NUM];    //顶点缓冲区对象
	LPDIRECT3DTEXTURE9      g_pTexture[MAX_NUM];    //纹理对象
	LPDIRECT3DTEXTURE9		_dfTexture;

	CRITICAL_SECTION		cs;
};

