#ifndef __ZS_WM__
#define __ZS_WM__

#include <inttypes.h>
#include <xcb/xcb.h>

typedef struct Monitor Monitor;

struct Monitor {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    Monitor *next;
};

extern xcb_connection_t *connection;

#endif
