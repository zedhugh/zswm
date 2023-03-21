#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#ifndef __ZS_WM__
#define __ZS_WM__

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

typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg;

typedef struct {
    uint16_t modifier;
    xcb_keysym_t keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */

extern xcb_connection_t *connection;
extern xcb_screen_t *screen;
extern xcb_window_t root;
extern xcb_key_symbols_t *keysyms;
extern int running;
extern xcb_cursor_t cursor[CurLast];

#endif
