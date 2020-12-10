// PhoneManager.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Windows.h>
#include <d3dx9.h>

#include "PhoneManager.h"
#include "Dxva2D3DUtils.h"
#include "VideoService.h"
#include "FlyTools.h"
#include "Controller.h"

#define MAX_LOADSTRING 100

// ȫ�ֱ���:
HINSTANCE hInst;								// ��ǰʵ��
TCHAR szTitle[MAX_LOADSTRING];					// �������ı�
TCHAR szWindowClass[MAX_LOADSTRING];			// ����������

Dxva2D3DUtils mDxva2D3DUtils;
Controller mController;
int WIDTH;
int HEIGHT;
RECT mClientRect[MAX_NUM];

// �˴���ģ���а����ĺ�����ǰ������:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

#define MAX_DBG_MSG_LEN (1024)
char out[MAX_DBG_MSG_LEN];


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
					   _In_opt_ HINSTANCE hPrevInstance,
					   _In_ LPTSTR    lpCmdLine,
					   _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: �ڴ˷��ô��롣
	MSG msg;
	HACCEL hAccelTable;

	// ��ʼ��ȫ���ַ���
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PHONEMANAGER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ִ��Ӧ�ó����ʼ��:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PHONEMANAGER));
	//����Ϣѭ��:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}		
	}

	return (int) msg.wParam;
}



//
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PHONEMANAGER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	=  CreateSolidBrush(RGB(45, 50, 170));;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��:
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

	//hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	//	CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		0, 0, 3840, 2160, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}
	//ShowWindow(hWnd, nCmdShow);
	ShowWindow(hWnd, SW_MAXIMIZE);
	UpdateWindow(hWnd);
	//��ʼ��Direct3D 

	//��ʼ��WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		TRACE("WSAStartup error !\n");
	}

	RECT rect;
	GetClientRect (hWnd, &rect) ;

	if(FAILED( mDxva2D3DUtils.InitD3D( hWnd, 1920,1000 ) ) )
	{
		MessageBox(NULL, "��������ʧ��", "InitD3D", MB_OK);
	}
	mController.start(hWnd, &mDxva2D3DUtils);
	return TRUE;
}

void initClientRect(HWND hWnd){
	RECT rect;
	GetClientRect(hWnd, &rect) ;
	WIDTH=rect.right;
	HEIGHT=rect.bottom;	
	TRACE("WIDTH=%d,HEIGHT=%d\n",WIDTH,HEIGHT);
	for(int i=0;i<MAX_NUM;i++){		
		mClientRect[i].left = WIDTH/400.0f+(i%7)*WIDTH*399.0f/400.0f/7.0f;
		mClientRect[i].right = mClientRect[i].left+(WIDTH*399.0f/400.0f/7.0f-WIDTH/400.0f);			
		int t_height = (mClientRect[i].right-mClientRect[i].left)*16.0f/9.0f;
		if(i/7==0){			
			mClientRect[i].top = HEIGHT/2-WIDTH/400.0f-t_height;
			mClientRect[i].bottom =  HEIGHT/2-WIDTH/400.0f;
		}else{			
			mClientRect[i].top = HEIGHT/2+WIDTH/400.0f;
			mClientRect[i].bottom = HEIGHT/2+WIDTH/400.0f+t_height;;
		}
	}
}

//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��: ���������ڵ���Ϣ��
//
//  WM_COMMAND	- ����Ӧ�ó���˵�
//  WM_PAINT	- ����������
//  WM_DESTROY	- �����˳���Ϣ������
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	SDL_MouseButtonEvent button;
	bool isMouse;
	isMouse = false;
	RECT rect;
	int x;
	int y;
	int id;
	switch (message)
	{
	case WM_COMMAND:		
		break;
	case WM_CREATE:	
	case WM_SIZE:
		initClientRect(hWnd);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: �ڴ���������ͼ����...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		mController.stop();
		mDxva2D3DUtils.Cleanup();
		WSACleanup();
		PostQuitMessage(0);		
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		button.type = message==WM_LBUTTONDOWN?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
		button.button = 1;
		x = LOWORD (lParam);
		y = HIWORD (lParam);
		for(int i=0;i<MAX_NUM;i++){
			if(x>=mClientRect[i].left&&x<=mClientRect[i].right&&y>=mClientRect[i].top&&y<=mClientRect[i].bottom){
				button.x=(x-mClientRect[i].left)*400/(mClientRect[i].right-mClientRect[i].left);
				button.y=(y-mClientRect[i].top)*712/(mClientRect[i].bottom-mClientRect[i].top);
				mController.sendMouseButtonEvent(&button,i);
				break;
			}
		}		
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		button.type = message==WM_MBUTTONDOWN?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
		button.button = 2;
		x = LOWORD (lParam);
		y = HIWORD (lParam);
		for(int i=0;i<MAX_NUM;i++){
			if(x>=mClientRect[i].left&&x<=mClientRect[i].right&&y>=mClientRect[i].top&&y<=mClientRect[i].bottom){
				button.x=(x-mClientRect[i].left)*400/(mClientRect[i].right-mClientRect[i].left);
				button.y=(y-mClientRect[i].top)*712/(mClientRect[i].bottom-mClientRect[i].top);
				mController.sendMouseButtonEvent(&button,i);
				break;
			}
		}			
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		button.type = message==WM_RBUTTONDOWN?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
		button.button = 3;
		x = LOWORD (lParam);
		y = HIWORD (lParam);
		for(int i=0;i<MAX_NUM;i++){
			if(x>=mClientRect[i].left&&x<=mClientRect[i].right&&y>=mClientRect[i].top&&y<=mClientRect[i].bottom){
				button.x=(x-mClientRect[i].left)*400/(mClientRect[i].right-mClientRect[i].left);
				button.y=(y-mClientRect[i].top)*712/(mClientRect[i].bottom-mClientRect[i].top);
				mController.sendMouseButtonEvent(&button,i);
				break;
			}
		}	
		break;
	case WM_MOUSEHWHEEL:
		SDL_MouseWheelEvent wEvent;
		x = LOWORD (lParam);
		y = HIWORD (lParam);
		for(int i=0;i<MAX_NUM;i++){
			if(x>=mClientRect[i].left&&x<=mClientRect[i].right&&y>=mClientRect[i].top&&y<=mClientRect[i].bottom){
				wEvent.x=(x-mClientRect[i].left)*400/(mClientRect[i].right-mClientRect[i].left);
				wEvent.y=(y-mClientRect[i].top)*712/(mClientRect[i].bottom-mClientRect[i].top);
				mController.sendMouseWheelEvent(&wEvent,i);
				break;
			}
		}
		break; 
	case WM_MOUSEMOVE:
		SDL_MouseMotionEvent mMotionEvent;
		x = LOWORD (lParam);
		y = HIWORD (lParam);
		for(int i=0;i<MAX_NUM;i++){
			if(x>=mClientRect[i].left&&x<=mClientRect[i].right&&y>=mClientRect[i].top&&y<=mClientRect[i].bottom){
				mMotionEvent.x=(x-mClientRect[i].left)*400/(mClientRect[i].right-mClientRect[i].left);
				mMotionEvent.y=(y-mClientRect[i].top)*712/(mClientRect[i].bottom-mClientRect[i].top);
				mController.sendMouseMotionEvent(&mMotionEvent,i);
				break;
			}
		}	
		break;		
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);		
	}
	return 0;
}

//float left = 1.0f+(i%7)*399.0f/(14/2.0f) - 200.0f;
//float right = left+(399.0f/(14/2.0f)-1.0f);
//float t_height = (right-left)*16.0f/9.0f*(float)width/(float)height;
//float top = 0;
//float bottom = 0;
//float start = 200.0f - (400.0f - 2.0f *t_height - 1.0f*width/height)/2.0f;
//if(i/(14/2)==0){			
//	top = start;
//	bottom = start - t_height;
//}else{			
//	top = start - t_height - 1.0f*width/height;
//	bottom = start - 2.0f *t_height - 1.0f*width/height;
//}
////CUSTOMVERTEX g_Vertices[] =	{
////	{ -200.0f, -200.0f,  0.0f,  0.0f, 1.0f},   
////	{ -200.0f,  200.0f,  0.0f,  0.0f, 0.0f},    
////	{  200.0f, -200.0f,  0.0f,  1.0f, 1.0f},    
////	{  200.0f,  200.0f,  0.0f,  1.0f, 0.0f}	
////};
//CUSTOMVERTEX g_Vertices[] =
//{
//	{left,   bottom, 0.0f,  0.0f, 1.0f},   
//	{left,   top,    0.0f,  0.0f, 0.0f},    
//	{right,  bottom, 0.0f,  1.0f, 1.0f},    
//	{right,  top,    0.0f,  1.0f, 0.0f}	
//};

// �����ڡ������Ϣ�������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
