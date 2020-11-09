#include <d3dx9.h>

#pragma once

#pragma comment(lib,"d3d9.lib") 
#pragma comment(lib,"winmm.lib") 
#pragma comment(lib,"d3dx9.lib")

#define D3DFVF_CUSTOMVERTEX   (D3DFVF_XYZ|D3DFVF_TEX1)
typedef unsigned __int8 uint8_t;  

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
	void SetupMatrices();
	HRESULT InitD3D( HWND hWnd );
	HRESULT InitGriphics();
	void Cleanup();
	void Render();
	void PushYUV(uint8_t *yuv,int widht, int height);
private:
	LPDIRECT3D9             g_pD3D;    //Direct3D对象
	LPDIRECT3DDEVICE9       g_pd3dDevice;    //Direct3D设备对象
	LPDIRECT3DVERTEXBUFFER9 g_pVB0;    //顶点缓冲区对象
	LPDIRECT3DVERTEXBUFFER9 g_pVB1;    //顶点缓冲区对象
	LPDIRECT3DTEXTURE9      g_pTexture;    //纹理对象

	CRITICAL_SECTION Critical;
};

