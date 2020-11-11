#include <d3dx9.h>

#pragma once

#pragma comment(lib,"d3d9.lib") 
#pragma comment(lib,"winmm.lib") 
#pragma comment(lib,"d3dx9.lib")

#define MAX_NUM 14
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
	HRESULT InitD3D( HWND hWnd, int width, int height );
	void Cleanup();
	void RenderRGB32(uint8_t *yuv,int widht, int height, int size, int num);
private:
	LPDIRECT3D9             g_pD3D;    //Direct3D����
	LPDIRECT3DDEVICE9       g_pd3dDevice;    //Direct3D�豸����

	LPDIRECT3DVERTEXBUFFER9 g_pVB[MAX_NUM];    //���㻺��������
	LPDIRECT3DTEXTURE9      g_pTexture[MAX_NUM];    //�������

	LPDIRECT3DTEXTURE9		_dfTexture;

	CRITICAL_SECTION cs;
};

