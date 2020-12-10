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
		0, 0, 640, 360, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}
	ShowWindow(hWnd, nCmdShow);
	//ShowWindow(hWnd, SW_MAXIMIZE);
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
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TEXTMETRIC tm;
	int width;
	int height;
	SDL_MouseButtonEvent button;
	bool isMouse;
	isMouse = false;
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
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		button.type = message==WM_MBUTTONDOWN?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
		button.button = 2;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		button.type = message==WM_RBUTTONDOWN?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
		button.button = 3;
		break;
	case WM_MOUSEHWHEEL:
		SDL_MouseWheelEvent wEvent;
		TRACE("WM_MOUSEHWHEEL\n");
		//wEvent.x=pMsg->pt.x-lRect.left;
		//wEvent.y=pMsg->pt.y-lRect.top;
		//mController->sendMouseWheelEvent(&wEvent);
		break; 
	case WM_MOUSEMOVE:
		SDL_MouseMotionEvent mMotionEvent;
		//mMotionEvent.x=pMsg->pt.x-lRect.left;
		//mMotionEvent.y=pMsg->pt.y-lRect.top;
		//mController->sendMouseMotionEvent(&mMotionEvent);
		break;
	//if(button.button>0){
	//	button.x=pMsg->pt.x-lRect.left;
	//	button.y=pMsg->pt.y-lRect.top;
	//	mController->sendMouseButtonEvent(&button);
	//}		
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

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
