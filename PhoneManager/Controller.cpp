#include "stdafx.h"
#include "Controller.h"
#include "FlyTools.h"
#include "VideoService.h"
#include <assert.h>
#define EVENT_STOP (SDL_USEREVENT + 1)

static const int ACTION_DOWN = 1;
static const int ACTION_UP = 1 << 1;
static bool vfinger_down = false;

bool IsSocketClosed(SOCKET clientSocket)  
{  
	bool ret = false;  
	HANDLE closeEvent = WSACreateEvent();  
	WSAEventSelect(clientSocket, closeEvent, FD_CLOSE);  
	DWORD dwRet = WaitForSingleObject(closeEvent, 0);
	if(dwRet == WSA_WAIT_EVENT_0)  
		ret = true;  
	else if(dwRet == WSA_WAIT_TIMEOUT)  
		ret = false; 
	WSACloseEvent(closeEvent);  
	return ret;  
} 

static void write_position(uint8_t *buf, const struct position *position) {
    buffer_write32be(&buf[0], position->point.x);
    buffer_write32be(&buf[4], position->point.y);
    buffer_write16be(&buf[8], position->screen_size.width);
    buffer_write16be(&buf[10], position->screen_size.height);
}

size_t utf8_truncation_index(const char *utf8, size_t max_len) {
    size_t len = strlen(utf8);
    if (len <= max_len) {
        return len;
    }
    len = max_len;
    while ((utf8[len] & 0x80) != 0 && (utf8[len] & 0xc0) != 0xc0) {
        len--;
    }
    return len;
}

static size_t write_string(const char *utf8, size_t max_len, unsigned char *buf) {
    size_t len = utf8_truncation_index(utf8, max_len);
    buffer_write32be(buf, len);
    memcpy(&buf[4], utf8, len);
    return 4 + len;
}

static uint16_t to_fixed_point_16(float f) {
    //assert(f >= 0.0f && f <= 1.0f);
    uint32_t u = f * (2^16); 
    if (u >= 0xffff) {
        u = 0xffff;
    }
    return (uint16_t) u;
}

size_t control_msg_serialize(struct control_msg *msg, unsigned char *buf) {
    buf[0] = msg->type;
	size_t ret = 0;
	uint16_t pressure = 0;
    switch (msg->type) {
        case CONTROL_MSG_TYPE_INJECT_KEYCODE:
            buf[1] = msg->inject_keycode.action;
            buffer_write32be(&buf[2], msg->inject_keycode.keycode);
            buffer_write32be(&buf[6], msg->inject_keycode.repeat);
            buffer_write32be(&buf[10], msg->inject_keycode.metastate);
            ret = 14;
			break;
        case CONTROL_MSG_TYPE_INJECT_TEXT: {
            size_t len = write_string(msg->inject_text.text, CONTROL_MSG_INJECT_TEXT_MAX_LENGTH, &buf[1]);
            ret =  1 + len;
			break;
        }
        case CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT:
            buf[1] = msg->inject_touch_event.action;
            buffer_write64be(&buf[2], msg->inject_touch_event.pointer_id);
            write_position(&buf[10], &msg->inject_touch_event.position);
            pressure = to_fixed_point_16(msg->inject_touch_event.pressure);
            buffer_write16be(&buf[22], pressure);
            buffer_write32be(&buf[24], msg->inject_touch_event.buttons);
            ret = 28;
			break;
        case CONTROL_MSG_TYPE_INJECT_SCROLL_EVENT:
            write_position(&buf[1], &msg->inject_scroll_event.position);
            buffer_write32be(&buf[13], (uint32_t) msg->inject_scroll_event.hscroll);
            buffer_write32be(&buf[17], (uint32_t) msg->inject_scroll_event.vscroll);
            ret = 21;
			break;
        case CONTROL_MSG_TYPE_SET_CLIPBOARD: {
            buf[1] = !!msg->set_clipboard.paste;
            size_t len = write_string(msg->set_clipboard.text, CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH, &buf[2]);
            ret = 2 + len;
			break;
        }
        case CONTROL_MSG_TYPE_SET_SCREEN_POWER_MODE:
            buf[1] = msg->set_screen_power_mode.mode;
            ret = 2;
			break;
        case CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON:
        case CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL:
        case CONTROL_MSG_TYPE_COLLAPSE_NOTIFICATION_PANEL:
        case CONTROL_MSG_TYPE_GET_CLIPBOARD:
        case CONTROL_MSG_TYPE_ROTATE_DEVICE:
            ret = 1;
			break;
        default:
            ret = 0;
			break;
    }
	return ret;
}

void sendMsg(SOCKET socket, struct control_msg msg){
	static unsigned char serialized_msg[CONTROL_MSG_MAX_SIZE];
	int length = control_msg_serialize(&msg, serialized_msg);
	if (!length) {
		return;
	}
	int w = send(socket,(const char *)serialized_msg,length,0);
}

static void send_keycode(struct control_msg *msg, enum android_keycode keycode, int actions, const char *name) {
    msg->type = CONTROL_MSG_TYPE_INJECT_KEYCODE;
    msg->inject_keycode.keycode = keycode;
    msg->inject_keycode.metastate = AMETA_NONE;
    msg->inject_keycode.repeat = 0;

    if (actions & ACTION_DOWN) {
        msg->inject_keycode.action = AKEY_EVENT_ACTION_DOWN;        
    }

    if (actions & ACTION_UP) {
        msg->inject_keycode.action = AKEY_EVENT_ACTION_UP;        
    }
}

#define MAP(FROM, TO) case FROM: *to = TO; return true
#define FAIL default: return false
bool convert_mouse_action(SDL_EventType from, enum android_motionevent_action *to) {
    switch (from) {
        MAP(SDL_MOUSEBUTTONDOWN, AMOTION_EVENT_ACTION_DOWN);
        MAP(SDL_MOUSEBUTTONUP,   AMOTION_EVENT_ACTION_UP);
        FAIL;
    }
}

enum android_motionevent_buttons convert_mouse_buttons(uint32_t state) {
    enum android_motionevent_buttons buttons = AMOTION_EVENT_BUTTON_PRIMARY;
    if (state & SDL_BUTTON_LMASK) {
        buttons = (android_motionevent_buttons)(buttons|AMOTION_EVENT_BUTTON_PRIMARY);
    }
    if (state & SDL_BUTTON_RMASK) {
		buttons = (android_motionevent_buttons)(buttons|AMOTION_EVENT_BUTTON_SECONDARY);
    }
    if (state & SDL_BUTTON_MMASK) {
		buttons = (android_motionevent_buttons)(buttons|AMOTION_EVENT_BUTTON_TERTIARY);
    }
    if (state & SDL_BUTTON_X1MASK) {
		buttons = (android_motionevent_buttons)(buttons|AMOTION_EVENT_BUTTON_BACK);
    }
    if (state & SDL_BUTTON_X2MASK) {
		buttons = (android_motionevent_buttons)(buttons|AMOTION_EVENT_BUTTON_FORWARD);
    }
    return buttons;
}

struct point screen_convert_window_to_frame_coords(struct screen *screen, int32_t x, int32_t y) {
    int ww, wh, dw, dh;
    SDL_GetWindowSize(screen->window, &ww, &wh);
    SDL_GL_GetDrawableSize(screen->window, &dw, &dh);
    x = (int64_t) x * dw / ww;
    y = (int64_t) y * dh / wh;
    unsigned rotation = screen->rotation;
    assert(rotation < 4);
    int32_t w = screen->content_size.width;
    int32_t h = screen->content_size.height;
    x = (int64_t) (x - screen->rect.x) * w / screen->rect.w;
    y = (int64_t) (y - screen->rect.y) * h / screen->rect.h;
    struct point result;
    switch (rotation) {
        case 0:
            result.x = x;
            result.y = y;
            break;
        case 1:
            result.x = h - y;
            result.y = x;
            break;
        case 2:
            result.x = w - x;
            result.y = h - y;
            break;
        default:
            assert(rotation == 3);
            result.x = y;
            result.y = w - x;
            break;
    }
    return result;
}

static struct point inverse_point(struct point point, struct size size) {
    point.x = size.width - point.x;
    point.y = size.height - point.y;
    return point;
}

static bool simulate_virtual_finger(SOCKET socket, enum android_motionevent_action action,  struct point point) {
    bool up = action == AMOTION_EVENT_ACTION_UP;
    struct control_msg msg;
    msg.type = CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT;
    msg.inject_touch_event.action = action;
    msg.inject_touch_event.position.screen_size = screen->frame_size;
    msg.inject_touch_event.position.point = point;
    msg.inject_touch_event.pointer_id =(uint64_t)(-2);
    msg.inject_touch_event.pressure = up ? 0.0f : 1.0f;
    msg.inject_touch_event.buttons = AMOTION_EVENT_BUTTON_PRIMARY;
    sendMsg(socket,msg);
    return true;
}

static bool convert_mouse_motion(const SDL_MouseMotionEvent *from, struct screen *screen, struct control_msg *to) {
    to->type = CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT;
    to->inject_touch_event.action = AMOTION_EVENT_ACTION_MOVE;
    to->inject_touch_event.pointer_id = (uint64_t)(-1);
    to->inject_touch_event.position.screen_size = screen->frame_size;
    to->inject_touch_event.position.point.x = from->x*1080/400;
	to->inject_touch_event.position.point.y = from->y*1920/712;
    to->inject_touch_event.pressure = 1.f;
    to->inject_touch_event.buttons = convert_mouse_buttons(from->state);
    return true;
}

static bool convert_mouse_wheel(const SDL_MouseWheelEvent *from, struct screen *screen,   struct control_msg *to) {
    int mouse_x;
    int mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    struct position position = {
        screen->frame_size,
        screen_convert_window_to_frame_coords(screen,mouse_x, mouse_y),
    };
    to->type = CONTROL_MSG_TYPE_INJECT_SCROLL_EVENT;
    to->inject_scroll_event.position = position;
    to->inject_scroll_event.hscroll = from->x;
    to->inject_scroll_event.vscroll = from->y;
    return true;
}

static bool convert_mouse_button(const SDL_MouseButtonEvent *from, struct screen *screen, struct control_msg *to) {
    to->type = CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT;

    if (!convert_mouse_action((SDL_EventType)from->type, &to->inject_touch_event.action)) {
        return false;
    }
	to->inject_touch_event.buttons = convert_mouse_buttons(SDL_BUTTON(from->button));
    to->inject_touch_event.pointer_id = (uint64_t)(-1);
    to->inject_touch_event.position.screen_size = screen->frame_size;
	to->inject_touch_event.position.point.x = from->x*1080/400;
	to->inject_touch_event.position.point.y = from->y*1920/712;
	to->inject_touch_event.pressure = from->type == SDL_MOUSEBUTTONDOWN ? 1.f : 0.f;
    return true;
}

void input_manager_process_mouse_motion(SOCKET socket, const SDL_MouseMotionEvent *event) {
    if (!event->state) {
        return;
    }
    if (event->which == SDL_TOUCH_MOUSEID) {
        return;
    }
    struct control_msg msg;
    if (convert_mouse_motion(event, screen, &msg)) {
         sendMsg(socket,msg);
    }
    if (vfinger_down) {
        struct point mouse = msg.inject_touch_event.position.point;
        struct point vfinger = inverse_point(mouse, screen->frame_size);
        simulate_virtual_finger(socket, AMOTION_EVENT_ACTION_MOVE, vfinger);
    }
}

void input_manager_process_mouse_wheel(SOCKET socket, const SDL_MouseWheelEvent *event) {	
	struct control_msg msg;
    if (convert_mouse_wheel(event, screen, &msg)) {
        sendMsg(socket,msg);
    }
}

void input_manager_process_mouse_button(SOCKET socket,  const SDL_MouseButtonEvent *event) {	
    if (event->which == SDL_TOUCH_MOUSEID) {
        return;
    }
	struct control_msg msg;
    bool down = event->type == SDL_MOUSEBUTTONDOWN;
    if (down) {
        if (event->button == SDL_BUTTON_RIGHT) {
            msg.type = CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON;
			sendMsg(socket,msg);
            return;
        }
        if (event->button == SDL_BUTTON_MIDDLE) {
            send_keycode(&msg, AKEYCODE_HOME, ACTION_DOWN | ACTION_UP, "HOME");	
			sendMsg(socket,msg);
            return;
        }
    }

    if (!convert_mouse_button(event, screen, &msg)) {
        return;
    }

	sendMsg(socket,msg);

	#define CTRL_PRESSED (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
    if ((down && !vfinger_down && CTRL_PRESSED) || (!down && vfinger_down)) {
        struct point mouse = msg.inject_touch_event.position.point;
		struct point vfinger = inverse_point(mouse, screen->frame_size);
        enum android_motionevent_action action = down? AMOTION_EVENT_ACTION_DOWN : AMOTION_EVENT_ACTION_UP;
        if (!simulate_virtual_finger(socket, action, vfinger)) {
            return;
        }
        vfinger_down = down;
    }
 }

bool convert_touch_action(SDL_EventType from, enum android_motionevent_action *to) {
    switch (from) {
        MAP(SDL_FINGERMOTION, AMOTION_EVENT_ACTION_MOVE);
        MAP(SDL_FINGERDOWN,   AMOTION_EVENT_ACTION_DOWN);
        MAP(SDL_FINGERUP,     AMOTION_EVENT_ACTION_UP);
        FAIL;
    }
}

struct point screen_convert_drawable_to_frame_coords(struct screen *screen, int32_t x, int32_t y) {
    unsigned rotation = screen->rotation;
    assert(rotation < 4);
    int32_t w = screen->content_size.width;
    int32_t h = screen->content_size.height;
    x = (int64_t) (x - screen->rect.x) * w / screen->rect.w;
    y = (int64_t) (y - screen->rect.y) * h / screen->rect.h;
    struct point result;
    switch (rotation) {
        case 0:
            result.x = x;
            result.y = y;
            break;
        case 1:
            result.x = h - y;
            result.y = x;
            break;
        case 2:
            result.x = w - x;
            result.y = h - y;
            break;
        default:
            assert(rotation == 3);
            result.x = y;
            result.y = w - x;
            break;
    }
    return result;
}

static bool convert_touch(const SDL_TouchFingerEvent *from, struct screen *screen, struct control_msg *to) {
    to->type = CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT;
    if (!convert_touch_action((SDL_EventType)from->type, &to->inject_touch_event.action)) {
        return false;
    }
    to->inject_touch_event.pointer_id = from->fingerId;
    to->inject_touch_event.position.screen_size = screen->frame_size;

    int dw;
    int dh;
    SDL_GL_GetDrawableSize(screen->window, &dw, &dh);

    // SDL touch event coordinates are normalized in the range [0; 1]
    int32_t x = from->x * dw;
    int32_t y = from->y * dh;
    to->inject_touch_event.position.point = screen_convert_drawable_to_frame_coords(screen, x, y);
    to->inject_touch_event.pressure = from->pressure;
    to->inject_touch_event.buttons = AMOTION_EVENT_BUTTON_PRIMARY;
    return true;
}

void input_manager_process_touch(SOCKET socket, const SDL_TouchFingerEvent *event) {
    struct control_msg msg;
    if (convert_touch(event, screen, &msg)) {
		sendMsg(socket, msg);
    }
}

//Controller start
Controller::Controller(void)
{
	mPort = 9008;
	socket_lis = INVALID_SOCKET;
	socket_cli = INVALID_SOCKET;
	screen = &flyscreen;
	screen->content_size.width = 400;
	screen->content_size.height = 712;
	screen->frame_size.width = 1080;
	screen->frame_size.height = 1920;
	screen->rotation = 0;
    screen->rect.x = 0;
	screen->rect.y = 0;
	screen->rect.w = 400;
	screen->rect.h = 712;
}

Controller::~Controller(void)
{
}

void Controller::start(HWND hwnd,Dxva2D3DUtils *mDxva2D3DUtils)
{
	this->mHwnd = hwnd;
	this->mDxva2D3DUtils = mDxva2D3DUtils;
	isStop = false;	
	m_socketThread = CreateThread(NULL, 0, &Controller::socketThread, this, CREATE_SUSPENDED, NULL);  
	if (NULL!= m_socketThread) {  
		ResumeThread(m_socketThread);  
	}
}

DWORD CALLBACK Controller::socketThread(LPVOID lp)
{
	TRACE("Controller socketThread start. \n");
	Controller *mPtr=(Controller *)lp;
	struct sockaddr_in sin;
	struct sockaddr_in remoteAddr;

	mPtr->isRunning = true;

	mPtr->socket_lis = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mPtr->socket_lis == INVALID_SOCKET)
	{
		TRACE("Controller socket error ! \n");
		return -1;
	} 		
	sin.sin_family = AF_INET;
	sin.sin_port = htons(mPtr->mPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(mPtr->socket_lis, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		TRACE("Controller bind error! \n");
		return -1;
	}
	if (listen(mPtr->socket_lis, 32) == SOCKET_ERROR)
	{
		TRACE("Controller listen error! \n");
		return -1;
	}	
	int nAddrlen = sizeof(remoteAddr);
	while (!mPtr->isStop)
	{
		mPtr->socket_cli = accept(mPtr->socket_lis, (SOCKADDR *)&remoteAddr, &nAddrlen);
		TRACE("Controller accept socke_cli=%d.\n",mPtr->socket_cli);
		if(mPtr->socket_cli != INVALID_SOCKET){			
			char buff[1024];
			memset(buff,0,1024);
			int len = recv(mPtr->socket_cli,buff,1024,0);
			int id = atoi(buff);
			id = (id/10-1)*7+id%10-1;
			TRACE("Controller client id=%d\n",id);
			VideoService *mVideoService = new VideoService();
			mVideoService->start(mPtr->mHwnd,mPtr->mDxva2D3DUtils,id);
			//mPtr->m_sendThread = CreateThread(NULL, 0, &Controller::sendThread, lp, CREATE_SUSPENDED, NULL);  
			//if (NULL!= mPtr->m_sendThread) {  
			//	ResumeThread(mPtr->m_sendThread);  
			//}	
		}else{
			if(mPtr->socket_lis != INVALID_SOCKET){
				closesocket(mPtr->socket_lis);
			}
			mPtr->isRunning = false;
			TRACE("Controller socketThread exit 1. \n"); 
			return 0;
		}
	}
	if(mPtr->socket_cli != INVALID_SOCKET){
		closesocket(mPtr->socket_cli);
	}
	if(mPtr->socket_lis != INVALID_SOCKET){
		closesocket(mPtr->socket_lis);
	}
	mPtr->isRunning = false;
	TRACE("Controller socketThread exit 2. \n"); 
	return 0;
}

DWORD CALLBACK Controller::sendThread(LPVOID lp)
{
	Controller *mPtr=(Controller *)lp;
	SOCKET  m_socket = mPtr->socket_cli;
	SDL_Event event;	
	while (!mPtr->isStop&&SDL_WaitEvent(&event)) {
		switch (event.type) {
		case EVENT_STOP:
			closesocket(m_socket);
			m_socket=INVALID_SOCKET;
			closesocket(mPtr->socket_lis);
			mPtr->socket_lis=INVALID_SOCKET;
			TRACE("Controller SDLWindow stop\n");
			return 0;
		case SDL_QUIT:
			closesocket(m_socket);
			m_socket=INVALID_SOCKET;
			closesocket(mPtr->socket_lis);
			mPtr->socket_lis=INVALID_SOCKET;
			TRACE("Controller SDL_WaitEvent SDL_QUIT\n");			
			return 0;
		case SDL_WINDOWEVENT:
			break;
		case SDL_TEXTINPUT:
			break;
		case SDL_KEYDOWN:
			break;
		case SDL_KEYUP:
			break;
		case SDL_MOUSEMOTION:
			input_manager_process_mouse_motion(m_socket,&(event.motion));
			break;
		case SDL_MOUSEWHEEL:
			input_manager_process_mouse_wheel(m_socket, &(event.wheel));
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:	
			input_manager_process_mouse_button(m_socket,&(event.button));	
			break;
		case SDL_FINGERMOTION:
		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
			input_manager_process_touch(m_socket, &(event.tfinger));
			break;
		}
		bool ret = IsSocketClosed(m_socket);
		if(ret){
			closesocket(m_socket);
			break;
		}		
	}
	return 0;
}

void Controller::sendMouseMotionEvent(SDL_MouseMotionEvent *event)
{
	if(socket_cli!=INVALID_SOCKET){
		//input_manager_process_mouse_motion(socket_cli,event);	
	}
}

void Controller::sendMouseWheelEvent(SDL_MouseWheelEvent *event)
{
	if(socket_cli!=INVALID_SOCKET){
		//input_manager_process_mouse_wheel(socket_cli,event);	
	}
}

void Controller::sendMouseButtonEvent(SDL_MouseButtonEvent *event)
{
	if(socket_cli!=INVALID_SOCKET){
		//input_manager_process_mouse_button(socket_cli,event);	
	}
}


void Controller::stop()
{
	isStop = true;	
	SDL_Event stop_event;
	stop_event.type = EVENT_STOP;
    SDL_PushEvent(&stop_event);
	if(socket_cli != INVALID_SOCKET){
		closesocket(socket_cli);
	}
	if(socket_lis != INVALID_SOCKET){
		closesocket(socket_lis);
	}
	while (isRunning){
		TRACE("Controller thread is running\n");
		Sleep(1000);
	}
}