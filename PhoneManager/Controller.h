#include <winsock2.h>
#include <windows.h>
#include "SDL.h"
#include "android\input.h"
#include "android\keycodes.h"
#include "util\buffer_util.h"
#include "screen.h"
#include "Dxva2D3DUtils.h"
#define CONTROL_MSG_MAX_SIZE (1 << 18) // 256k
#define CONTROL_MSG_INJECT_TEXT_MAX_LENGTH 300
#define CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH (CONTROL_MSG_MAX_SIZE - 6)

#pragma comment(lib,"ws2_32.lib")
#pragma once


enum control_msg_type {
	CONTROL_MSG_TYPE_INJECT_KEYCODE,
	CONTROL_MSG_TYPE_INJECT_TEXT,
	CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT,
	CONTROL_MSG_TYPE_INJECT_SCROLL_EVENT,
	CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON,
	CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL,
	CONTROL_MSG_TYPE_COLLAPSE_NOTIFICATION_PANEL,
	CONTROL_MSG_TYPE_GET_CLIPBOARD,
	CONTROL_MSG_TYPE_SET_CLIPBOARD,
	CONTROL_MSG_TYPE_SET_SCREEN_POWER_MODE,
	CONTROL_MSG_TYPE_ROTATE_DEVICE,
};

struct control_msg {
	enum control_msg_type type;
	union {
		struct {
			enum android_keyevent_action action;
			enum android_keycode keycode;
			uint32_t repeat;
			enum android_metastate metastate;
		} inject_keycode;
		struct {
			char *text; // owned, to be freed by SDL_free()
		} inject_text;
		struct {
			enum android_motionevent_action action;
			enum android_motionevent_buttons buttons;
			uint64_t pointer_id;
			struct position position;
			float pressure;
		} inject_touch_event;
		struct {
			struct position position;
			int32_t hscroll;
			int32_t vscroll;
		} inject_scroll_event;
		struct {
			char *text; // owned, to be freed by SDL_free()
			bool paste;
		} set_clipboard;
		struct {
			enum screen_power_mode mode;
		} set_screen_power_mode;
	};
};

class Controller
{
public:
	Controller(void);
	~Controller(void);
	void start(HWND hwnd,Dxva2D3DUtils *mDxva2D3DUtils);
	void stop();
	void sendMouseMotionEvent(SDL_MouseMotionEvent *event);
	void sendMouseWheelEvent(SDL_MouseWheelEvent *event);
	void sendMouseButtonEvent(SDL_MouseButtonEvent *event);
private:

	int mPort;

	SOCKET socket_lis;
	SOCKET socket_cli;

	bool isStop;
	bool isRunning;
	HANDLE m_socketThread;  
	static DWORD CALLBACK socketThread(LPVOID); 

	HANDLE m_sendThread;  
	static DWORD CALLBACK sendThread(LPVOID);

	SDL_Window *pWindow;

	Dxva2D3DUtils *mDxva2D3DUtils;
	HWND mHwnd;

};

