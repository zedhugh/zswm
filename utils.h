#include "types.h"

#ifndef __ZS_WM_UTILS__
#define __ZS_WM_UTILS__

#define LENGTH(X) (sizeof X / sizeof X[0])

double get_time();

void die(const char *fmt, ...);

void logger(const char *fmt, ...);

void *ecalloc(size_t nmemb, size_t size);

void spawn(const Arg *arg);

uint32_t alloc_color(xcb_connection_t *conn, xcb_colormap_t cmap,
                     const char *color);

xcb_visualtype_t *find_visual(xcb_screen_t *screen, xcb_visualid_t visualid);

Monitor *xy_to_monitor(Monitor *monitors, int x, int y);

void set_window_class_instance(xcb_connection_t *conn, xcb_window_t window,
                               const char *class, const char *instance);

#endif
