#include "stdafx.h"
#include "D3DUtils.h"


D3DUtils::D3DUtils(void)
{
	g_pD3D=NULL;    //Direct3D对象
	g_pd3dDevice=NULL;    //Direct3D设备对象
	g_pVB0=NULL;    //顶点缓冲区对象
	g_pVB1=NULL;    //顶点缓冲区对象
	g_pTexture=NULL;    //纹理对象

	InitializeCriticalSection(&Critical);
}


D3DUtils::~D3DUtils(void)
{
}

//-----------------------------------------------------------------------------
// Desc: 设置变换矩阵
//-----------------------------------------------------------------------------
void D3DUtils::SetupMatrices()
{
	//创建并设置世界矩阵
	D3DXMATRIXA16 matWorld;
	D3DXMatrixIdentity( &matWorld );
	g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
	//创建并设置观察矩阵
	D3DXVECTOR3 vEyePt( 0.0f, 0.0f, -10 );
	D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
	g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );
	//创建并设置投影矩阵
	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 100.0f );
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}
//-----------------------------------------------------------------------------
// Desc: 初始化Direct3D
//-----------------------------------------------------------------------------
HRESULT D3DUtils::InitD3D( HWND hWnd )
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
	//设置变换矩阵
	SetupMatrices();

	return S_OK;
}
//-----------------------------------------------------------------------------
// Desc: 创建场景图形
//-----------------------------------------------------------------------------
HRESULT D3DUtils::InitGriphics()
{
	//创建纹理对象
	if( FAILED( D3DXCreateTextureFromFile( g_pd3dDevice, "texture.jpg", &g_pTexture ) ) )
	{
		MessageBox(NULL, "创建纹理失败", "Texture.exe", MB_OK);
		return E_FAIL;
	}

	//顶点数据
	CUSTOMVERTEX g_Vertices0[] =
	{
		{ -4,   -4,  0.0f,  0.0f, 1.0f},   
		{ -4,    4,  0.0f,  0.0f, 0.0f},    
		{  4,   -4,  0.0f,  1.0f, 1.0f},    
		{  4,    4,  0.0f,  1.0f, 0.0f}	
	};
	//CUSTOMVERTEX g_Vertices[] =
	//{
	//	{ -4,  0.1,  0.0f,  0.0f, 1.0f},   
	//	{ -4,    4,  0.0f,  0.0f, 0.0f},    
	//	{ -3,  0.1,  0.0f,  1.0f, 1.0f},    
	//	{ -3,    4,  0.0f,  1.0f, 0.0f}	
	//};

	//创建顶点缓冲区
	if( FAILED( g_pd3dDevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX),0, D3DFVF_CUSTOMVERTEX,D3DPOOL_MANAGED, &g_pVB0,NULL)))
	{
		return E_FAIL;
	}
	//填充顶点缓冲区
	void* pVertices0;
	if(FAILED(g_pVB0->Lock(0, sizeof(g_Vertices0), (void**)&pVertices0, 0))) return E_FAIL;
	memcpy( pVertices0, g_Vertices0, sizeof(g_Vertices0) );
	g_pVB0->Unlock();


	//顶点数据
	CUSTOMVERTEX g_Vertices1[] =
	{
		{ -4,   -4,  0.0f,  0.0f, 1.0f},   
		{ -4, -0.1,  0.0f,  0.0f, 0.0f},    
		{ -3,   -4,  0.0f,  1.0f, 1.0f},    
		{ -3, -0.1,  0.0f,  1.0f, 0.0f}			
	};

	//创建顶点缓冲区
	if( FAILED( g_pd3dDevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX),0, D3DFVF_CUSTOMVERTEX,D3DPOOL_MANAGED, &g_pVB1,NULL)))
	{
		return E_FAIL;
	}
	//填充顶点缓冲区
	void* pVertices1;
	if(FAILED(g_pVB1->Lock(0, sizeof(g_Vertices1), (void**)&pVertices1, 0))) return E_FAIL;
	memcpy( pVertices1, g_Vertices1, sizeof(g_Vertices1) );
	g_pVB1->Unlock();
	return S_OK;
}
//-----------------------------------------------------------------------------
// Desc: 释放创建的对象
//-----------------------------------------------------------------------------
void D3DUtils::Cleanup()
{
	//释放纹理对象
	if( g_pTexture != NULL )
		g_pTexture->Release();
	//释放顶点缓冲区对象
	if( g_pVB0 != NULL )        
		g_pVB0->Release();
	if( g_pVB1 != NULL )        
		g_pVB1->Release();
	//释放Direct3D设备对象
	if( g_pd3dDevice != NULL ) 
		g_pd3dDevice->Release();
	//释放Direct3D对象
	if( g_pD3D != NULL )       
		g_pD3D->Release();
}
//-----------------------------------------------------------------------------
// Desc: 渲染图形 
//-----------------------------------------------------------------------------
void D3DUtils::Render()
{
	EnterCriticalSection(&Critical);
	//清空后台缓冲区
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);

	//开始在后台缓冲区绘制图形
	if(SUCCEEDED( g_pd3dDevice->BeginScene()))
	{
		g_pd3dDevice->SetTexture(0, g_pTexture); //设置纹理(重剑：在俩三角形上贴了张图)
		g_pd3dDevice->SetStreamSource( 0, g_pVB0, 0, sizeof(CUSTOMVERTEX) );
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);
		//

		//g_pd3dDevice->SetTexture( 0, g_pTexture ); //设置纹理(重剑：在俩三角形上贴了张图)
		//g_pd3dDevice->SetStreamSource( 0, g_pVB1, 0, sizeof(CUSTOMVERTEX) );
		//g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		//g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);

		g_pd3dDevice->EndScene();
	}
	//将在后台缓冲区绘制的图形提交到前台缓冲区显示
	g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	LeaveCriticalSection(&Critical);
}

void D3DUtils::PushYUV(uint8_t *yuv,int widht, int height)
{
	OutputDebugString("PushYUV\n");
}
