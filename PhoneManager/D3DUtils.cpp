#include "stdafx.h"
#include "D3DUtils.h"


D3DUtils::D3DUtils(void)
{
	g_pD3D=NULL;    //Direct3D����
	g_pd3dDevice=NULL;    //Direct3D�豸����
	g_pVB0=NULL;    //���㻺��������
	g_pVB1=NULL;    //���㻺��������
	g_pTexture=NULL;    //�������

	InitializeCriticalSection(&Critical);
}


D3DUtils::~D3DUtils(void)
{
}

//-----------------------------------------------------------------------------
// Desc: ���ñ任����
//-----------------------------------------------------------------------------
void D3DUtils::SetupMatrices()
{
	//�����������������
	D3DXMATRIXA16 matWorld;
	D3DXMatrixIdentity( &matWorld );
	g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
	//���������ù۲����
	D3DXVECTOR3 vEyePt( 0.0f, 0.0f, -10 );
	D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
	g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );
	//����������ͶӰ����
	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 100.0f );
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}
//-----------------------------------------------------------------------------
// Desc: ��ʼ��Direct3D
//-----------------------------------------------------------------------------
HRESULT D3DUtils::InitD3D( HWND hWnd )
{
	//����Direct3D����, �ö������ڴ���Direct3D�豸����
	if(NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION ))) return E_FAIL;
	//����D3DPRESENT_PARAMETERS�ṹ, ׼������Direct3D�豸����
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	//����Direct3D�豸����
	if(FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}
	//��������Ч��
	g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE ); 
	//���ñ任����
	SetupMatrices();

	return S_OK;
}
//-----------------------------------------------------------------------------
// Desc: ��������ͼ��
//-----------------------------------------------------------------------------
HRESULT D3DUtils::InitGriphics()
{
	//�����������
	if( FAILED( D3DXCreateTextureFromFile( g_pd3dDevice, "texture.jpg", &g_pTexture ) ) )
	{
		MessageBox(NULL, "��������ʧ��", "Texture.exe", MB_OK);
		return E_FAIL;
	}

	//��������
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

	//�������㻺����
	if( FAILED( g_pd3dDevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX),0, D3DFVF_CUSTOMVERTEX,D3DPOOL_MANAGED, &g_pVB0,NULL)))
	{
		return E_FAIL;
	}
	//��䶥�㻺����
	void* pVertices0;
	if(FAILED(g_pVB0->Lock(0, sizeof(g_Vertices0), (void**)&pVertices0, 0))) return E_FAIL;
	memcpy( pVertices0, g_Vertices0, sizeof(g_Vertices0) );
	g_pVB0->Unlock();


	//��������
	CUSTOMVERTEX g_Vertices1[] =
	{
		{ -4,   -4,  0.0f,  0.0f, 1.0f},   
		{ -4, -0.1,  0.0f,  0.0f, 0.0f},    
		{ -3,   -4,  0.0f,  1.0f, 1.0f},    
		{ -3, -0.1,  0.0f,  1.0f, 0.0f}			
	};

	//�������㻺����
	if( FAILED( g_pd3dDevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX),0, D3DFVF_CUSTOMVERTEX,D3DPOOL_MANAGED, &g_pVB1,NULL)))
	{
		return E_FAIL;
	}
	//��䶥�㻺����
	void* pVertices1;
	if(FAILED(g_pVB1->Lock(0, sizeof(g_Vertices1), (void**)&pVertices1, 0))) return E_FAIL;
	memcpy( pVertices1, g_Vertices1, sizeof(g_Vertices1) );
	g_pVB1->Unlock();
	return S_OK;
}
//-----------------------------------------------------------------------------
// Desc: �ͷŴ����Ķ���
//-----------------------------------------------------------------------------
void D3DUtils::Cleanup()
{
	//�ͷ��������
	if( g_pTexture != NULL )
		g_pTexture->Release();
	//�ͷŶ��㻺��������
	if( g_pVB0 != NULL )        
		g_pVB0->Release();
	if( g_pVB1 != NULL )        
		g_pVB1->Release();
	//�ͷ�Direct3D�豸����
	if( g_pd3dDevice != NULL ) 
		g_pd3dDevice->Release();
	//�ͷ�Direct3D����
	if( g_pD3D != NULL )       
		g_pD3D->Release();
}
//-----------------------------------------------------------------------------
// Desc: ��Ⱦͼ�� 
//-----------------------------------------------------------------------------
void D3DUtils::Render()
{
	EnterCriticalSection(&Critical);
	//��պ�̨������
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);

	//��ʼ�ں�̨����������ͼ��
	if(SUCCEEDED( g_pd3dDevice->BeginScene()))
	{
		g_pd3dDevice->SetTexture(0, g_pTexture); //��������(�ؽ���������������������ͼ)
		g_pd3dDevice->SetStreamSource( 0, g_pVB0, 0, sizeof(CUSTOMVERTEX) );
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);
		//

		//g_pd3dDevice->SetTexture( 0, g_pTexture ); //��������(�ؽ���������������������ͼ)
		//g_pd3dDevice->SetStreamSource( 0, g_pVB1, 0, sizeof(CUSTOMVERTEX) );
		//g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		//g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);

		g_pd3dDevice->EndScene();
	}
	//���ں�̨���������Ƶ�ͼ���ύ��ǰ̨��������ʾ
	g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	LeaveCriticalSection(&Critical);
}

void D3DUtils::PushYUV(uint8_t *yuv,int widht, int height)
{
	OutputDebugString("PushYUV\n");
}
