#include "stdafx.h"
#include "D3DUtils.h"


D3DUtils::D3DUtils(void)
{
	g_pD3D=NULL;    //Direct3D对象
	g_pd3dDevice=NULL;    //Direct3D设备对象
	for(int i=0;i<MAX_NUM;i++){
		g_pTexture[i]=NULL;    //纹理对象
		g_pVB[i]=NULL;    //顶点缓冲区对象
		g_pVB[i]=NULL;    //顶点缓冲区对象
	}
	InitializeCriticalSection(&cs);
}


D3DUtils::~D3DUtils(void)
{
}

//-----------------------------------------------------------------------------
// Desc: 初始化Direct3D
//-----------------------------------------------------------------------------
HRESULT D3DUtils::InitD3D( HWND hWnd, int width, int height )
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
	D3DXVECTOR3 vEyePt( 0.0f, 0.0f, -50.f );
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
		float left = 1.0f+(i%7)*99.0f/(14/2.0f) - 50.0f;
		float right = left+(99.0f/(14/2.0f)-1.0f);
		float t_height = (right-left)*16.0f/9.0f*(float)width/(float)height;
		float top = 0;
		float bottom = 0;
		float start = 50.0f - (100.0f - 2.0f *t_height - 1.0f*width/height)/2.0f;
		if(i/(14/2)==0){			
			top = start;
			bottom = start - t_height;
		}else{			
			top = start - t_height - 1.0f*width/height;
			bottom = start - 2.0f *t_height - 1.0f*width/height;
		}
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
void D3DUtils::Cleanup()
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
void D3DUtils::RenderRGB32(uint8_t *rgb32,int width, int height, int size, int num)
{
	EnterCriticalSection(&cs);
	//新建一个纹理
	if(g_pTexture[num]==NULL){
		D3DXCreateTexture(g_pd3dDevice, width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_R8G8B8, D3DPOOL_DEFAULT, &g_pTexture[num]);
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
