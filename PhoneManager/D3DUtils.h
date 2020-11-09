#include <d3dx9.h>

#pragma once

#pragma comment(lib,"d3d9.lib") 
#pragma comment(lib,"winmm.lib") 
#pragma comment(lib,"d3dx9.lib")

#define D3DFVF_CUSTOMVERTEX   (D3DFVF_XYZ|D3DFVF_TEX1)
typedef unsigned __int8 uint8_t;  

struct CUSTOMVERTEX
{
    FLOAT x, y, z;       //����λ��  
    FLOAT u, v ;         //������������
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
	LPDIRECT3D9             g_pD3D;    //Direct3D����
	LPDIRECT3DDEVICE9       g_pd3dDevice;    //Direct3D�豸����
	LPDIRECT3DVERTEXBUFFER9 g_pVB0;    //���㻺��������
	LPDIRECT3DVERTEXBUFFER9 g_pVB1;    //���㻺��������
	LPDIRECT3DTEXTURE9      g_pTexture;    //�������

	CRITICAL_SECTION Critical;
};

