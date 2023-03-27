#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#ifndef __ZS_WM__GLOBAL__

typedef struct Monitor Monitor;
struct Monitor {
    /* monitor rectangle */
    int16_t mx, my;
    uint16_t mw, mh;

    /* window rectangle */
    int16_t wx, wy;
    uint16_t ww, wh;

    xcb_window_t barwin;

    Monitor *next;
};

/* cursor for window manager */
enum { CurNormal, CurResize, CurMove, CurLast };

typedef struct {
    bool running;
    uint32_t barheight;
    int screen_nbr;
    Display *dpy;
    xcb_connection_t *conn;
    xcb_screen_t *screen;
    xcb_key_symbols_t *keysymbol;
    xcb_cursor_t cursors[CurLast];
    Monitor *mon;
} zswm_global_t;

extern zswm_global_t global;

#endif
