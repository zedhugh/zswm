#include <string.h>

#include "types.h"

#ifndef __ZS_WM_UTILS__
#define __ZS_WM_UTILS__

#define CLIENT_WIDTH(X) ((X)->width + 2 * (X)->bw)
#define CLIENT_HEIGHT(X) ((X)->height + 2 * (X)->bw)
#define CLIENT_IS_VISIBLE(C) ((C)->tags & (C)->mon->seltags)

#define LENGTH(X) ((ssize_t)sizeof(X) / (ssize_t)sizeof(X[0]))

/** \brief replace \c NULL strings with empty strings */
#define NONULL(x) (x ? x : "")

static inline int a_strcmp(const char *a, const char *b) {
    return strcmp(NONULL(a), NONULL(b));
}

#define A_STREQ(a, b) (((a) == (b)) || a_strcmp(a, b) == 0)
#define A_STRNEQ(a, b) (!A_STRNEQ(a, b))

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
