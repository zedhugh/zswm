#include "cairo.h"
#include "pango/pango-layout.h"
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
    cairo_surface_t *surface;
    cairo_t *cr;

    Monitor *next;
};

/* cursor for window manager */
enum { CurNormal, CurResize, CurMove, CurLast };

typedef struct {
    bool running;
    uint8_t barheight;
    xcb_connection_t *conn;
    xcb_screen_t *screen;
    xcb_visualtype_t *visual;
    xcb_key_symbols_t *keysymbol;
    xcb_cursor_t cursors[CurLast];
    Monitor *monitor;
    PangoLayout *layout;
} zswm_global_t;

extern zswm_global_t global;

#endif
