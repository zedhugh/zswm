#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "utils.h"

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
        fputc(' ', stderr);
        perror(NULL);
    } else {
        fputc('\n', stderr);
    }

    exit(1);
}

void logger(const char *fmt, ...) {
#ifdef LOG_FILE
    va_list ap;
    va_start(ap, fmt);
    FILE *log_file = fopen(LOG_FILE, "a+t");
    vfprintf(log_file, fmt, ap);
    va_end(ap);
    fclose(log_file);
#endif
}

void *ecalloc(size_t nmemb, size_t size) {
    void *p;

    if (!(p = calloc(nmemb, size))) {
        die("calloc:");
    }

    return p;
}

void run_shell_cmd(char **cmd) {
    if (fork() == 0) {
        setsid();
        execvp(cmd[0], cmd);
        die("zswm execvp: '%s' failed", cmd);
    }
}

uint32_t alloc_color(xcb_connection_t *conn, xcb_colormap_t cmap,
                     const char *color) {
    uint16_t red, green, blue;
    xcb_alloc_color_cookie_t cookie;
    xcb_alloc_color_reply_t *reply;

    xcb_aux_parse_color(color, &red, &green, &blue);
    cookie = xcb_alloc_color(conn, cmap, red, green, blue);

    reply = xcb_alloc_color_reply(conn, cookie, NULL);
    uint32_t color_pixel = reply->pixel;

    free(reply);
    return color_pixel;
}

xcb_visualtype_t *find_visual(xcb_screen_t *screen, xcb_visualid_t visualid) {
    xcb_depth_iterator_t depth_iter;
    depth_iter = xcb_screen_allowed_depths_iterator(screen);
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter;
        visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        if (visual_iter.data->visual_id == visualid) {
            return visual_iter.data;
        }
    }

    return NULL;
}

Monitor *xy_to_monitor(Monitor *monitors, int x, int y) {
    int16_t mx, my;
    uint16_t mw, mh;

    for (Monitor *m = monitors; m; m = m->next) {
        mx = m->mx;
        my = m->my;
        mw = m->mw;
        mh = m->mh;
        if (x >= mx && x <= mx + mw && y >= my && y <= my + mh) {
            return m;
        }
    }

    return monitors;
}

Client *next_show_client(Client *client) {
    for (; client && !CLIENT_IS_VISIBLE(client); client = client->next) {
    }

    return client;
}
