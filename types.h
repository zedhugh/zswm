#include <pango/pangocairo.h>
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>

#ifndef __ZS_WM_TYPES__
#define __ZS_WM_TYPES__

typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg;

typedef struct {
    PangoColor pango_color;
    uint32_t xcb_color_pixel;
} Color;

typedef struct Monitor Monitor;
typedef struct Client Client;

struct Client {
    char name[256];
    int x, y, width, height;
    int oldx, oldy, old_width, old_height;
    int bw, oldbw; /* border width */
    unsigned int tags;
    bool isfullscreen;
    xcb_window_t win;
    Client *next;
    Monitor *mon;
};

struct Monitor {
    /* monitor rectangle */
    int16_t mx, my;
    uint16_t mw, mh;

    /* window rectangle */
    int16_t wx, wy;
    uint16_t ww, wh;

    uint16_t seltags;

    xcb_window_t barwin;
    cairo_surface_t *surface;
    cairo_t *cr;

    Monitor *next;
};

/* cursor for window manager */
enum { CurNormal, CurResize, CurMove, CurLast };

enum { SchemeNorm, SchemeSel, SchemeLast }; /* color schemes */
enum { ColFg, ColBg, ColBorder, ColLast };  /* color scheme index */
typedef enum { ClkTagBar, ClkRootWin } ClickType;

typedef struct {
    bool running;
    bool restart;
    xcb_connection_t *conn;
    xcb_screen_t *screen;
    xcb_visualtype_t *visual;
    xcb_key_symbols_t *keysymbol;
    xcb_cursor_t cursors[CurLast];
    Monitor *monitors;
    Monitor *current_monitor;
    Color color[SchemeLast][ColLast];
} zswm_global_t;

#endif
