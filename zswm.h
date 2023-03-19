#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#ifndef __ZS_WM__
#define __ZS_WM__

typedef struct Monitor Monitor;

struct Monitor {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
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

extern xcb_connection_t *connection;
extern xcb_screen_t *screen;
extern xcb_window_t root;
extern xcb_key_symbols_t *keysyms;
extern int running;

#endif
