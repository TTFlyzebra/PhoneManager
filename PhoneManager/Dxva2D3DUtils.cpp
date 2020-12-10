#include "stdafx.h"
#include "Dxva2D3DUtils.h"

#include <d3d9.h>
#include <dxva2api.h>
#include "FlyTools.h"

extern "C"
{
#include "libyuv.h"
#include "libavcodec/dxva2.h"
#include "libavutil/avassert.h"
#include "libavutil/buffer.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/pixfmt.h"
}

typedef struct {
	const char   *name;
	D3DFORMAT    format;
	AVPixelFormat  codec;
} d3d_format_t;
/* XXX Prefered format must come first */
static const d3d_format_t d3d_formats[] = {
	{ "YV12", (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'), AV_PIX_FMT_YUV420P },
	{ "NV12", (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2'), AV_PIX_FMT_NV12 },

	{ NULL, (D3DFORMAT)0, AV_PIX_FMT_NONE }
};

/* define all the GUIDs used directly here,
to avoid problems with inconsistent dxva2api.h versions in mingw-w64 and different MSVC version */
#include <initguid.h>
DEFINE_GUID(IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf, 0x55, 0x00, 0x05, 0x4e, 0x43, 0xff, 0x02);

DEFINE_GUID(DXVA2_ModeMPEG2_VLD, 0xee27417f, 0x5e28, 0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9);
DEFINE_GUID(DXVA2_ModeMPEG2and1_VLD, 0x86695f12, 0x340e, 0x4f04, 0x9f, 0xd3, 0x92, 0x53, 0xdd, 0x32, 0x74, 0x60);
DEFINE_GUID(DXVA2_ModeH264_E, 0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeH264_F, 0x1b81be69, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E, 0x604F8E68, 0x4951, 0x4C54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVA2_ModeVC1_D, 0x1b81beA3, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeVC1_D2010, 0x1b81beA4, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeHEVC_VLD_Main, 0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0);
DEFINE_GUID(DXVA2_ModeHEVC_VLD_Main10, 0x107af0e0, 0xef1a, 0x4d19, 0xab, 0xa8, 0x67, 0xa1, 0x63, 0x07, 0x3d, 0x13);
DEFINE_GUID(DXVA2_ModeVP9_VLD_Profile0, 0x463707f8, 0xa1d0, 0x4585, 0x87, 0x6d, 0x83, 0xaa, 0x6d, 0x60, 0xb8, 0x9e);
DEFINE_GUID(DXVA2_NoEncrypt, 0x1b81beD0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(GUID_NULL, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

typedef IDirect3D9* WINAPI pDirect3DCreate9(UINT);
typedef HRESULT WINAPI pCreateDeviceManager9(UINT *, IDirect3DDeviceManager9 **);


typedef struct dxva2_mode {
	const GUID     *guid;
	enum AVCodecID codec;
} dxva2_mode;

static const dxva2_mode dxva2_modes[] = {
	/* MPEG-2 */
	{ &DXVA2_ModeMPEG2_VLD, AV_CODEC_ID_MPEG2VIDEO },
	{ &DXVA2_ModeMPEG2and1_VLD, AV_CODEC_ID_MPEG2VIDEO },

	/* H.264 */
	{ &DXVA2_ModeH264_F, AV_CODEC_ID_H264 },
	{ &DXVA2_ModeH264_E, AV_CODEC_ID_H264 },
	/* Intel specific H.264 mode */
	{ &DXVADDI_Intel_ModeH264_E, AV_CODEC_ID_H264 },

	/* VC-1 / WMV3 */
	{ &DXVA2_ModeVC1_D2010, AV_CODEC_ID_VC1 },
	{ &DXVA2_ModeVC1_D2010, AV_CODEC_ID_WMV3 },
	{ &DXVA2_ModeVC1_D, AV_CODEC_ID_VC1 },
	{ &DXVA2_ModeVC1_D, AV_CODEC_ID_WMV3 },

	/* HEVC/H.265 */
	{ &DXVA2_ModeHEVC_VLD_Main, AV_CODEC_ID_HEVC },
	{ &DXVA2_ModeHEVC_VLD_Main10, AV_CODEC_ID_HEVC },

	/* VP8/9 */
	{ &DXVA2_ModeVP9_VLD_Profile0, AV_CODEC_ID_VP9 },

	{ NULL, AV_CODEC_ID_NONE },
};

typedef struct surface_info {
	int used;
	uint64_t age;
} surface_info;

typedef struct DXVA2Context {
	HMODULE d3dlib;
	HMODULE dxva2lib;

	HANDLE  deviceHandle;

	IDirect3D9                  *d3d9;
	IDirect3DDevice9            *d3d9device;
	IDirect3DDeviceManager9     *d3d9devmgr;
	IDirectXVideoDecoderService *decoder_service;
	IDirectXVideoDecoder        *decoder;

	GUID                        decoder_guid;
	DXVA2_ConfigPictureDecode   decoder_config;

	LPDIRECT3DSURFACE9          *surfaces;
	surface_info                *surface_infos;
	uint32_t                    num_surfaces;
	uint64_t                    surface_age;

	AVFrame                     *tmp_frame;
} DXVA2Context;

typedef struct DXVA2SurfaceWrapper {
	DXVA2Context         *ctx;
	LPDIRECT3DSURFACE9   surface;
	IDirectXVideoDecoder *decoder;
} DXVA2SurfaceWrapper;

void Dxva2D3DUtils::dxva2_destroy_decoder(AVCodecContext *s)
{
	InputStream  *ist = (InputStream *)s->opaque;
	DXVA2Context *ctx = (DXVA2Context *)ist->hwaccel_ctx;
	int i;

	if (ctx->surfaces) {
		for (i = 0; i < ctx->num_surfaces; i++) {
			if (ctx->surfaces[i])
				IDirect3DSurface9_Release(ctx->surfaces[i]);
		}
	}
	av_freep(&ctx->surfaces);
	av_freep(&ctx->surface_infos);
	ctx->num_surfaces = 0;
	ctx->surface_age = 0;

	if (ctx->decoder) {
		//IDirectXVideoDecoder_Release(ctx->decoder);
		ctx->decoder->Release();
		ctx->decoder = NULL;
	}
}

void Dxva2D3DUtils::dxva2_uninit(AVCodecContext *s)
{
	InputStream  *ist = (InputStream  *)s->opaque;
	DXVA2Context *ctx = (DXVA2Context *)ist->hwaccel_ctx;

	ist->hwaccel_uninit = NULL;
	ist->hwaccel_get_buffer = NULL;
	ist->hwaccel_retrieve_data = NULL;

	if (ctx->decoder)
		dxva2_destroy_decoder(s);

	if (ctx->decoder_service)
		//IDirectXVideoDecoderService_Release(ctx->decoder_service);
			ctx->decoder_service->Release();

	if (ctx->d3d9devmgr && ctx->deviceHandle != INVALID_HANDLE_VALUE)
		//IDirect3DDeviceManager9_CloseDeviceHandle(ctx->d3d9devmgr, ctx->deviceHandle);
			ctx->d3d9devmgr->CloseDeviceHandle(ctx->deviceHandle);

	if (ctx->d3d9devmgr)
		//IDirect3DDeviceManager9_Release(ctx->d3d9devmgr);
			ctx->d3d9devmgr->Release();

	if (ctx->d3d9device)
		IDirect3DDevice9_Release(ctx->d3d9device);

	if (ctx->d3d9)
		IDirect3D9_Release(ctx->d3d9);

	if (ctx->d3dlib)
		FreeLibrary(ctx->d3dlib);

	if (ctx->dxva2lib)
		FreeLibrary(ctx->dxva2lib);

	av_frame_free(&ctx->tmp_frame);

	av_freep(&ist->hwaccel_ctx);
	av_freep(&s->hwaccel_context);
}

void Dxva2D3DUtils::dxva2_release_buffer(void *opaque, uint8_t *data)
{
	DXVA2SurfaceWrapper *w = (DXVA2SurfaceWrapper *)opaque;
	DXVA2Context        *ctx = w->ctx;
	int i;

	for (i = 0; i < ctx->num_surfaces; i++) {
		if (ctx->surfaces[i] == w->surface) {
			ctx->surface_infos[i].used = 0;
			break;
		}
	}
	IDirect3DSurface9_Release(w->surface);
	//IDirectXVideoDecoder_Release(w->decoder);
	w->decoder->Release();
	av_free(w);
}

int Dxva2D3DUtils::dxva2_get_buffer(AVCodecContext *s, AVFrame *frame, int flags)
{
	InputStream  *ist = (InputStream  *)s->opaque;
	DXVA2Context *ctx = (DXVA2Context *)ist->hwaccel_ctx;
	int i, old_unused = -1;
	LPDIRECT3DSURFACE9 surface;
	DXVA2SurfaceWrapper *w = NULL;

	av_assert0(frame->format == AV_PIX_FMT_DXVA2_VLD);

	for (i = 0; i < ctx->num_surfaces; i++) {
		surface_info *info = &ctx->surface_infos[i];
		if (!info->used && (old_unused == -1 || info->age < ctx->surface_infos[old_unused].age))
			old_unused = i;
	}
	if (old_unused == -1) {
		TRACE("No free DXVA2 surface!\n");
		return AVERROR(ENOMEM);
	}
	i = old_unused;

	surface = ctx->surfaces[i];

	w = (DXVA2SurfaceWrapper *)av_mallocz(sizeof(*w));
	if (!w)
		return AVERROR(ENOMEM);

	frame->buf[0] = av_buffer_create((uint8_t*)surface, 0, dxva2_release_buffer, w, AV_BUFFER_FLAG_READONLY);
	if (!frame->buf[0]) {
		av_free(w);
		return AVERROR(ENOMEM);
	}

	w->ctx = ctx;
	w->surface = surface;
	IDirect3DSurface9_AddRef(w->surface);
	w->decoder = ctx->decoder;
	//IDirectXVideoDecoder_AddRef(w->decoder);
	w->decoder->AddRef();

	ctx->surface_infos[i].used = 1;
	ctx->surface_infos[i].age = ctx->surface_age++;

	frame->data[3] = (uint8_t *)surface;

	return 0;
}

int Dxva2D3DUtils::dxva2_alloc(AVCodecContext *s, HWND hwnd)
{
	InputStream *ist = (InputStream *)s->opaque;
	int loglevel = (ist->hwaccel_id == HWACCEL_AUTO) ? AV_LOG_VERBOSE : AV_LOG_ERROR;
	DXVA2Context *ctx;
	pDirect3DCreate9      *createD3D = NULL;
	pCreateDeviceManager9 *createDeviceManager = NULL;
	HRESULT hr;
	D3DDISPLAYMODE        d3ddm;
	unsigned resetToken = 0;
	UINT adapter = D3DADAPTER_DEFAULT;

	ctx = (DXVA2Context *)av_mallocz(sizeof(*ctx));
	if (!ctx)
		return AVERROR(ENOMEM);

	ctx->deviceHandle = INVALID_HANDLE_VALUE;

	ist->hwaccel_ctx = ctx;
	ist->hwaccel_uninit = dxva2_uninit;
	ist->hwaccel_get_buffer = dxva2_get_buffer;

	ctx->d3dlib = LoadLibrary("d3d9.dll");
	if (!ctx->d3dlib) {
		TRACE("Failed to load D3D9 library\n");
		goto fail;
	}
	ctx->dxva2lib = LoadLibrary("dxva2.dll");
	if (!ctx->dxva2lib) {
		TRACE("Failed to load DXVA2 library\n");
		goto fail;
	}

	createD3D = (pDirect3DCreate9 *)GetProcAddress(ctx->d3dlib, "Direct3DCreate9");
	if (!createD3D) {
		TRACE("Failed to locate Direct3DCreate9\n");
		goto fail;
	}
	createDeviceManager = (pCreateDeviceManager9 *)GetProcAddress(ctx->dxva2lib, "DXVA2CreateDirect3DDeviceManager9");
	if (!createDeviceManager) {
		TRACE("Failed to locate DXVA2CreateDirect3DDeviceManager9\n");
		goto fail;
	}

	ctx->d3d9 = createD3D(D3D_SDK_VERSION);
	if (!ctx->d3d9) {
		TRACE("Failed to create IDirect3D object\n");
		goto fail;
	}

	if (ist->hwaccel_device) {
		adapter = atoi(ist->hwaccel_device);
		TRACE("Using HWAccel device %d\n", adapter);
	}

	IDirect3D9_GetAdapterDisplayMode(ctx->d3d9, adapter, &d3ddm);
	d3dpp.Windowed = TRUE;
	d3dpp.BackBufferCount = 0;
	d3dpp.hDeviceWindow = hwnd;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = FALSE;
	d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	D3DCAPS9 caps;
	DWORD BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;
	hr = ctx->d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	if (SUCCEEDED(hr))
	{
		if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		{
			BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;
		}
		else
		{
			BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;
		}
	}

	if (ctx->d3d9device)
	{
		ctx->d3d9device->Release();
		ctx->d3d9device = NULL;
	}

	hr = IDirect3D9_CreateDevice(ctx->d3d9, adapter, D3DDEVTYPE_HAL, hwnd, BehaviorFlags, &d3dpp, &ctx->d3d9device);
	if (FAILED(hr)) {
		TRACE("Failed to create Direct3D device\n");
		goto fail;
	}

	hr = createDeviceManager(&resetToken, &ctx->d3d9devmgr);
	if (FAILED(hr)) {
		TRACE("Failed to create Direct3D device manager\n");
		goto fail;
	}

	hr = ctx->d3d9devmgr->ResetDevice(ctx->d3d9device, resetToken);
	if (FAILED(hr)) {
		TRACE("Failed to bind Direct3D device to device manager\n");
		goto fail;
	}

	hr = ctx->d3d9devmgr->OpenDeviceHandle(&ctx->deviceHandle);
	if (FAILED(hr)) {
		TRACE("Failed to open device handle\n");
		goto fail;
	}

	hr = ctx->d3d9devmgr->GetVideoService(ctx->deviceHandle, IID_IDirectXVideoDecoderService, (void **)&ctx->decoder_service);
	if (FAILED(hr)) {
		TRACE("Failed to create IDirectXVideoDecoderService\n");
		goto fail;
	}

	ctx->tmp_frame = av_frame_alloc();
	if (!ctx->tmp_frame)
		goto fail;

	s->hwaccel_context = av_mallocz(sizeof(struct dxva_context));
	if (!s->hwaccel_context)
		goto fail;

	return 0;
fail:
	dxva2_uninit(s);
	return AVERROR(EINVAL);
}


static int dxva2_get_decoder_configuration(AVCodecContext *s, const GUID *device_guid,
										   const DXVA2_VideoDesc *desc,
										   DXVA2_ConfigPictureDecode *config)
{
	InputStream  *ist = (InputStream *)s->opaque;
	int loglevel = (ist->hwaccel_id == HWACCEL_AUTO) ? AV_LOG_VERBOSE : AV_LOG_ERROR;
	DXVA2Context *ctx = (DXVA2Context *)ist->hwaccel_ctx;
	unsigned cfg_count = 0, best_score = 0;
	DXVA2_ConfigPictureDecode *cfg_list = NULL;
	DXVA2_ConfigPictureDecode best_cfg = { { 0 } };
	HRESULT hr;
	int i;

	hr = ctx->decoder_service->GetDecoderConfigurations(*device_guid, desc, NULL, &cfg_count, &cfg_list);
	if (FAILED(hr)) {
		TRACE("Unable to retrieve decoder configurations\n");
		return AVERROR(EINVAL);
	}

	for (i = 0; i < cfg_count; i++) {
		DXVA2_ConfigPictureDecode *cfg = &cfg_list[i];

		unsigned score;
		if (cfg->ConfigBitstreamRaw == 1)
			score = 1;
		else if (s->codec_id == AV_CODEC_ID_H264 && cfg->ConfigBitstreamRaw == 2)
			score = 2;
		else
			continue;
		if (IsEqualGUID(cfg->guidConfigBitstreamEncryption, DXVA2_NoEncrypt))
			score += 16;
		if (score > best_score) {
			best_score = score;
			best_cfg = *cfg;
		}
	}
	CoTaskMemFree(cfg_list);

	if (!best_score) {
		TRACE("No valid decoder configuration available\n");
		return AVERROR(EINVAL);
	}

	*config = best_cfg;
	return 0;
}

static const d3d_format_t *D3dFindFormat(D3DFORMAT format)
{
	for (unsigned i = 0; d3d_formats[i].name; i++) {
		if (d3d_formats[i].format == format)
			return &d3d_formats[i];
	}
	return NULL;
}


int Dxva2D3DUtils::dxva2_create_decoder(AVCodecContext *s)
{
	InputStream  *ist = (InputStream *)s->opaque;
	int loglevel = (ist->hwaccel_id == HWACCEL_AUTO) ? AV_LOG_VERBOSE : AV_LOG_ERROR;
	DXVA2Context *ctx = (DXVA2Context *)ist->hwaccel_ctx;
	struct dxva_context *dxva_ctx = (dxva_context *)s->hwaccel_context;
	GUID *guid_list = NULL;
	unsigned guid_count = 0, i, j;
	GUID device_guid = GUID_NULL;
	DXVA2_VideoDesc desc = { 0 };
	DXVA2_ConfigPictureDecode config;
	HRESULT hr;
	int surface_alignment;
	int ret;
	D3DFORMAT target_format = D3DFMT_X8R8G8B8;

	hr = ctx->decoder_service->GetDecoderDeviceGuids(&guid_count, &guid_list);
	if (FAILED(hr)) {
		TRACE("Failed to retrieve decoder device GUIDs\n");
		goto fail;
	}

	for (i = 0; dxva2_modes[i].guid; i++) {
		D3DFORMAT *target_list = NULL;
		unsigned target_count = 0;
		const dxva2_mode *mode = &dxva2_modes[i];
		if (mode->codec != s->codec_id)
			continue;

		for (j = 0; j < guid_count; j++) {
			if (IsEqualGUID(*mode->guid, guid_list[j]))
				break;
		}
		if (j == guid_count)
			continue;

		//hr = IDirectXVideoDecoderService_GetDecoderRenderTargets(ctx->decoder_service, mode->guid, &target_count, &target_list);
		hr = ctx->decoder_service->GetDecoderRenderTargets(*mode->guid, &target_count, &target_list);
		if (FAILED(hr)) {
			continue;
		}

		for (unsigned j = 0; j < target_count; j++)
		{
			const D3DFORMAT f = target_list[j];
			const d3d_format_t *format = D3dFindFormat(f);
			if (format) {
				TRACE("%s is supported for output\n", format->name);
			}
			else {
				TRACE("%d is supported for output (%4.4s)\n", f, (const char*)&f);
			}
		}

		for (j = 0; j < target_count; j++) {
			const D3DFORMAT format = target_list[j];
			if (format == MKTAG('N', 'V', '1', '2')) {
				target_format = format;
				break;
			}
		}
		CoTaskMemFree(target_list);
		if (target_format) {
			device_guid = *mode->guid;
			break;
		}
	}
	CoTaskMemFree(guid_list);

	if (IsEqualGUID(device_guid, GUID_NULL)) {
		TRACE("No decoder device for codec found\n");
		goto fail;
	}

	desc.SampleWidth = s->coded_width;
	desc.SampleHeight = s->coded_height;
	desc.Format = target_format;

	ret = dxva2_get_decoder_configuration(s, &device_guid, &desc, &config);
	if (ret < 0) {
		goto fail;
	}

	/* decoding MPEG-2 requires additional alignment on some Intel GPUs,
	but it causes issues for H.264 on certain AMD GPUs..... */
	if (s->codec_id == AV_CODEC_ID_MPEG2VIDEO)
		surface_alignment = 32;
	/* the HEVC DXVA2 spec asks for 128 pixel aligned surfaces to ensure
	all coding features have enough room to work with */
	else if (s->codec_id == AV_CODEC_ID_HEVC)
		surface_alignment = 128;
	else
		surface_alignment = 16;

	/* 4 base work surfaces */
	ctx->num_surfaces = 4;

	/* add surfaces based on number of possible refs */
	if (s->codec_id == AV_CODEC_ID_H264 || s->codec_id == AV_CODEC_ID_HEVC)
		ctx->num_surfaces += 16;
	else if (s->codec_id == AV_CODEC_ID_VP9)
		ctx->num_surfaces += 8;
	else
		ctx->num_surfaces += 2;

	/* add extra surfaces for frame threading */
	if (s->active_thread_type & FF_THREAD_FRAME)
		ctx->num_surfaces += s->thread_count;

	ctx->surfaces = (LPDIRECT3DSURFACE9 *)av_mallocz(ctx->num_surfaces * sizeof(*ctx->surfaces));
	ctx->surface_infos = (surface_info *)av_mallocz(ctx->num_surfaces * sizeof(*ctx->surface_infos));

	if (!ctx->surfaces || !ctx->surface_infos) {
		TRACE("Unable to allocate surface arrays\n");
		goto fail;
	}

	hr = ctx->decoder_service->CreateSurface(FFALIGN(s->coded_width, surface_alignment),
		FFALIGN(s->coded_height, surface_alignment),
		ctx->num_surfaces - 1,
		target_format, D3DPOOL_DEFAULT, 0,
		DXVA2_VideoDecoderRenderTarget,
		ctx->surfaces, NULL);

	if (FAILED(hr)) {
		TRACE("Failed to create %d video surfaces\n", ctx->num_surfaces);
		goto fail;
	}

	/*hr = IDirectXVideoDecoderService_CreateVideoDecoder(ctx->decoder_service, &device_guid,
	&desc, &config, ctx->surfaces,
	ctx->num_surfaces, &ctx->decoder);*/
	hr = ctx->decoder_service->CreateVideoDecoder(device_guid,
		&desc, &config, ctx->surfaces,
		ctx->num_surfaces, &ctx->decoder);
	if (FAILED(hr)) {
		TRACE("Failed to create DXVA2 video decoder\n");
		goto fail;
	}

	ctx->decoder_guid = device_guid;
	ctx->decoder_config = config;

	dxva_ctx->cfg = &ctx->decoder_config;
	dxva_ctx->decoder = ctx->decoder;
	dxva_ctx->surface = ctx->surfaces;
	dxva_ctx->surface_count = ctx->num_surfaces;

	if (IsEqualGUID(ctx->decoder_guid, DXVADDI_Intel_ModeH264_E))
		dxva_ctx->workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;

	return 0;
fail:
	dxva2_destroy_decoder(s);
	return AVERROR(EINVAL);
}

int Dxva2D3DUtils::dxva2_init(AVCodecContext *s, HWND hwnd)
{
	//InitializeCriticalSection(&cs);

	InputStream *ist = (InputStream *)s->opaque;
	int loglevel = (ist->hwaccel_id == HWACCEL_AUTO) ? AV_LOG_VERBOSE : AV_LOG_ERROR;
	DXVA2Context *ctx;
	int ret;

	if (!ist->hwaccel_ctx && hwnd != NULL) {
		ret = dxva2_alloc(s,hwnd);
		if (ret < 0){
			TRACE("dxva2_alloc Failed! ret=%d\n",ret);
			return ret;
		}
	}
	ctx = (DXVA2Context *)ist->hwaccel_ctx;

	if (s->codec_id == AV_CODEC_ID_H264 &&
		(s->profile & ~FF_PROFILE_H264_CONSTRAINED) > FF_PROFILE_H264_HIGH) {
			TRACE("Unsupported H.264 profile for DXVA2 HWAccel: %d\n", s->profile);
			return AVERROR(EINVAL);
	}

	if (ctx->decoder)
		dxva2_destroy_decoder(s);

	ret = dxva2_create_decoder(s);
	if (ret < 0) {
		TRACE("Error creating the DXVA2 decoder\n");
		return ret;
	}

	return 0;
}


Dxva2D3DUtils::Dxva2D3DUtils(void)
{
	g_pD3D=NULL;    //Direct3D对象
	g_pd3dDevice=NULL;    //Direct3D设备对象
	for(int i=0;i<MAX_NUM;i++){
		g_pTexture[i]=NULL;    //纹理对象
		g_pVB[i]=NULL;    //顶点缓冲区对象
		g_pVB[i]=NULL;    //顶点缓冲区对象
	}

	m_pDirect3DSurfaceRender = NULL;
	m_pBackBuffer = NULL;

	InitializeCriticalSection(&cs);
}


Dxva2D3DUtils::~Dxva2D3DUtils(void)
{
}

AVPixelFormat Dxva2D3DUtils::GetHwFormat(AVCodecContext *s, const AVPixelFormat *pix_fmts)
{
	InputStream* ist = (InputStream*)s->opaque;
	ist->active_hwaccel_id = HWACCEL_DXVA2;
	ist->hwaccel_pix_fmt = AV_PIX_FMT_DXVA2_VLD;
	return ist->hwaccel_pix_fmt;
}

BOOL Dxva2D3DUtils::HWAccelInit(AVCodec *codec, AVCodecContext *ctx, HWND hWnd)
{
	bool bRet = TRUE;
	switch (codec->id)
	{
	case AV_CODEC_ID_MPEG2VIDEO:
	case AV_CODEC_ID_H264:
	case AV_CODEC_ID_VC1:
	case AV_CODEC_ID_WMV3:
	case AV_CODEC_ID_HEVC:
	case AV_CODEC_ID_VP9:
		{
			// multi threading is apparently not compatible 
			// with hardware decoding
			ctx->thread_count = 1;  
			InputStream *ist = new InputStream();
			ist->hwaccel_id = HWACCEL_AUTO;
			ist->active_hwaccel_id = HWACCEL_AUTO;
			ist->hwaccel_device = "dxva2";
			ist->dec = codec;
			ist->dec_ctx = ctx;
			ctx->opaque = ist;
			if (dxva2_init(ctx, hWnd) == 0)
			{
				ctx->get_buffer2 = ist->hwaccel_get_buffer;
				ctx->get_format = GetHwFormat;
				ctx->thread_safe_callbacks = 1;
				bRet = TRUE;
			}
			else
			{
				bRet = FALSE;
			}

			break;
		}
	default:
		{
			bRet = FALSE;
			break;
		}
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// Desc: 初始化Direct3D
//-----------------------------------------------------------------------------
HRESULT Dxva2D3DUtils::InitD3D( HWND hWnd, int width, int height )
{
	//创建Direct3D对象, 该对象用于创建Direct3D设备对象
	if(NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION ))) return E_FAIL;
	//设置D3DPRESENT_PARAMETERS结构, 准备创建Direct3D设备对象
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	//创建Direct3D设备对象
	if(FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}
	//禁用照明效果
	g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE ); 

	//创建并设置世界矩阵
	D3DXMATRIXA16 matWorld;
	D3DXMatrixIdentity( &matWorld );
	g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
	//创建并设置观察矩阵
	D3DXVECTOR3 vEyePt( 0.0f, 0.0f, -200.f );
	D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
	g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );
	//创建并设置投影矩阵
	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/2, 1.0f, 1.0f, 1000.0f );
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

	//创建纹理对象
	if( FAILED( D3DXCreateTextureFromFile( g_pd3dDevice, "texture.jpg", &_dfTexture ) ) )
	{
		MessageBox(NULL, "创建纹理失败", "Texture", MB_OK);
		return E_FAIL;
	}
	//

	for(int i=0;i<MAX_NUM;i++){		
		//顶点数据
		float left = 1.0f+(i%7)*399.0f/(14/2.0f) - 200.0f;
		float right = left+(399.0f/(14/2.0f)-1.0f);
		float t_height = (right-left)*16.0f/9.0f*(float)width/(float)height;
		float top = 0;
		float bottom = 0;
		float start = 200.0f - (400.0f - 2.0f *t_height - 1.0f*width/height)/2.0f;
		if(i/(14/2)==0){			
			top = start;
			bottom = start - t_height;
		}else{			
			top = start - t_height - 1.0f*width/height;
			bottom = start - 2.0f *t_height - 1.0f*width/height;
		}
		//CUSTOMVERTEX g_Vertices[] =	{
		//	{ -200.0f, -200.0f,  0.0f,  0.0f, 1.0f},   
		//	{ -200.0f,  200.0f,  0.0f,  0.0f, 0.0f},    
		//	{  200.0f, -200.0f,  0.0f,  1.0f, 1.0f},    
		//	{  200.0f,  200.0f,  0.0f,  1.0f, 0.0f}	
		//};
		CUSTOMVERTEX g_Vertices[] =
		{
			{left,   bottom, 0.0f,  0.0f, 1.0f},   
			{left,   top,    0.0f,  0.0f, 0.0f},    
			{right,  bottom, 0.0f,  1.0f, 1.0f},    
			{right,  top,    0.0f,  1.0f, 0.0f}	
		};

		//创建顶点缓冲区
		if( FAILED( g_pd3dDevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX),0, D3DFVF_CUSTOMVERTEX,D3DPOOL_MANAGED, &g_pVB[i],NULL)))
		{
			return E_FAIL;
		}
		//填充顶点缓冲区
		void* pVertices;
		if(FAILED(g_pVB[i]->Lock(0, sizeof(g_Vertices), (void**)&pVertices, 0))) return E_FAIL;
		memcpy( pVertices, g_Vertices, sizeof(g_Vertices) );
		g_pVB[i]->Unlock();

	}
	return S_OK;
}

//-----------------------------------------------------------------------------
// Desc: 释放创建的对象
//-----------------------------------------------------------------------------
void Dxva2D3DUtils::Cleanup()
{	
	for(int i=0;i<MAX_NUM;i++){
		//释放纹理对象
		if( g_pTexture[i] != NULL )
			g_pTexture[i]->Release();
		//释放顶点缓冲区对象
		if( g_pVB[i] != NULL )        
			g_pVB[i]->Release();
	}
	if( _dfTexture != NULL )
		_dfTexture->Release();
	//释放Direct3D设备对象
	if( g_pd3dDevice != NULL ) 
		g_pd3dDevice->Release();
	//释放Direct3D对象
	if( g_pD3D != NULL )       
		g_pD3D->Release();
}
//-----------------------------------------------------------------------------
// Desc: 渲染RGB32图形 
//-----------------------------------------------------------------------------
static DWORD lastPlayTime = 0;
void Dxva2D3DUtils::RenderRGB32(uint8_t *rgb32,int width, int height, int size, int num)
{
	EnterCriticalSection(&cs);
	//新建一个纹理
	if(g_pTexture[num]==NULL){
		D3DXCreateTexture(g_pd3dDevice, width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &g_pTexture[num]);
	}
	D3DLOCKED_RECT LockedRect;
	g_pTexture[num]->LockRect(0, &LockedRect, NULL, 0);
	//LockedRect.pBits = yuv;
	memcpy(LockedRect.pBits,rgb32,size);
	g_pTexture[num]->UnlockRect(0);

	////开始在后台缓冲区绘制图形
	//DWORD currentTime = GetTickCount(); 
	//if((currentTime-lastPlayTime)>=40){
	//	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(45, 123, 255), 1.0f, 0);
	if(SUCCEEDED( g_pd3dDevice->BeginScene()))
	{
		//for(int i=0;i<MAX_NUM;i++){
		g_pd3dDevice->SetTexture(0, g_pTexture[num]); //设置纹理(重剑：在俩三角形上贴了张图)
		g_pd3dDevice->SetStreamSource( 0, g_pVB[num], 0, sizeof(CUSTOMVERTEX) );
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);
		//}
		g_pd3dDevice->EndScene();
	}
	//lastPlayTime = currentTime;
	//}	
	//将在后台缓冲区绘制的图形提交到前台缓冲区显示
	g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	LeaveCriticalSection(&cs);
}

void NV12_RGB32_SSE(const BYTE *yBuf, const BYTE *uvBuf, const int width, const int height, int lineSize, BYTE *rgbBuf) { 	
	int dstIndex = 0;
	int uvIndex = 0;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			BYTE y = yBuf[i * lineSize + j];
			uvIndex = i/2*lineSize+j-j%2;
			BYTE u = uvBuf[uvIndex];
			BYTE v = uvBuf[uvIndex+1];

			dstIndex = (width*i+j) * 4;
			int data = (int)(y + 1.772 * (u - 128));
			rgbBuf[dstIndex] = ((data < 0) ? 0 : (data > 255 ? 255 : data));

			data = (int)(y - 0.34414 * (u - 128) - 0.71414 * (v - 128));
			rgbBuf[dstIndex + 1] = ((data < 0) ? 0 : (data > 255 ? 255 : data));

			data = (int)(y + 1.402 * (v - 128));
			rgbBuf[dstIndex + 2] = ((data < 0) ? 0 : (data > 255 ? 255 : data));
		}
	}
	//__asm{
	//	mov eax,lineSize;     
	//	add eax,0;  
	//	mov lineSize,eax; 
	//}
}

int Dxva2D3DUtils::dxva2_retrieve_data_call(AVCodecContext *s, AVFrame *frame, int num)
{
	if(num<0||num>=MAX_NUM) {
		TRACE("id[%d] error!\n",num);
		return 0;
	}
	LPDIRECT3DSURFACE9 surface = (LPDIRECT3DSURFACE9)frame->data[3];
	InputStream  *ist = (InputStream  *)s->opaque;
	DXVA2Context *ctx = (DXVA2Context *)ist->hwaccel_ctx;
	D3DSURFACE_DESC    surfaceDesc;
	D3DLOCKED_RECT     srcLockedRect;
	D3DLOCKED_RECT     objLockedRect;
	HRESULT            hr;
	int                ret;
	

	EnterCriticalSection(&cs);	
	if(g_pTexture[num]==NULL){
		D3DXCreateTexture(g_pd3dDevice, frame->width, frame->height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_pTexture[num]);
	}
	surface->GetDesc(&surfaceDesc);
    surface->LockRect(&srcLockedRect, NULL, D3DLOCK_READONLY);	
	g_pTexture[num]->LockRect(0, &objLockedRect, NULL, 0);
	libyuv::ConvertToARGB(
		(const uint8_t*)srcLockedRect.pBits,
		srcLockedRect.Pitch*surfaceDesc.Height * 3 /2,
		(uint8_t*) objLockedRect.pBits, 
		objLockedRect.Pitch,
		0, 
		0,
		srcLockedRect.Pitch, 
		surfaceDesc.Height,
		objLockedRect.Pitch/4, 
		surfaceDesc.Height,
		libyuv::kRotate0,
		libyuv::FOURCC_NV12);
	g_pTexture[num]->UnlockRect(0);
	surface->UnlockRect();
	if(num==0){
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(45, 50, 170), 1.0f, 0);
		g_pd3dDevice->BeginScene();
		for(int i=0;i<MAX_NUM;i++) {			
			if(g_pTexture[0]!=NULL){				
				g_pd3dDevice->SetTexture(0, g_pTexture[0]); //设置纹理(重剑：在俩三角形上贴了张图)
				g_pd3dDevice->SetStreamSource( 0, g_pVB[i], 0, sizeof(CUSTOMVERTEX) );
				g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
				g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);								
			}			
		}
		g_pd3dDevice->EndScene();
		g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	}
	LeaveCriticalSection(&cs);
	return 0;
}