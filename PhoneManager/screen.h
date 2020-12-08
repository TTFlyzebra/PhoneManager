#include "SDL.h"

enum screen_power_mode {
    SCREEN_POWER_MODE_OFF = 0,
    SCREEN_POWER_MODE_NORMAL = 2,
};

struct size {
    uint16_t width;
    uint16_t height;
};

struct point {
    int32_t x;
    int32_t y;
};

struct position {
    struct size screen_size;
    struct point point;
};

struct port_range {
    uint16_t first;
    uint16_t last;
};

struct screen {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool use_opengl;
    struct sc_opengl *gl;
    struct size frame_size;
    struct size content_size; // rotated frame_size

    bool resize_pending; // resize requested while fullscreen or maximized
    // The content size the last time the window was not maximized or
    // fullscreen (meaningful only when resize_pending is true)
    struct size windowed_content_size;

    // client rotation: 0, 1, 2 or 3 (x90 degrees counterclockwise)
    unsigned rotation;
    // rectangle of the content (excluding black borders)
    struct SDL_Rect rect;
    bool has_frame;
    bool fullscreen;
    bool maximized;
    bool no_window;
    bool mipmaps;
};

static struct screen flyscreen;
static struct screen *screen;