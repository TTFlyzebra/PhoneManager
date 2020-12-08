// PhoneManager.cpp : 定义应用程序的入口点。
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

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

Dxva2D3DUtils mDxva2D3DUtils;
VideoService mVideoService[MAX_NUM];

// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

#define MAX_DBG_MSG_LEN (1024)
char out[MAX_DBG_MSG_LEN];

Controller mController;



int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
					   _In_opt_ HINSTANCE hPrevInstance,
					   _In_ LPTSTR    lpCmdLine,
					   _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PHONEMANAGER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PHONEMANAGER));
	//主消息循环:
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
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
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
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // 将实例句柄存储在全局变量中

	//hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	//	CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		0, 0, 1920, 1000, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}
	//ShowWindow(hWnd, nCmdShow);
	ShowWindow(hWnd, SW_MAXIMIZE);
	UpdateWindow(hWnd);
	//初始化Direct3D 

	//初始化WSA
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
		MessageBox(NULL, "创建纹理失败", "InitD3D", MB_OK);
	}
	mController.start(9008);
	for(int i=0;i<MAX_NUM;i++){
		mVideoService[i].start(hWnd, &mDxva2D3DUtils, i);
	}
	return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TEXTMETRIC tm;
	int width;
	int height;

	switch (message)
	{
	case WM_COMMAND:		
		break;
	case WM_CREATE:		
		RECT rect;
		GetClientRect (hWnd, &rect) ;
		TRACE("WM_CREATE\n");		
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此添加任意绘图代码...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		mController.stop();
		for(int i=0;i<MAX_NUM;i++){
			mVideoService[i].stop();
		}
		mDxva2D3DUtils.Cleanup();
		WSACleanup();
		PostQuitMessage(0);		
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
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
