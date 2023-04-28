#include <X11/cursorfont.h>
#include <stdint.h>
#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xinerama.h>

#include "cairo.h"
#include "cairo-xcb.h"
#include "config.h"
#include "draw.h"
#include "event.h"
#include "utils.h"

zswm_global_t global;

static void init_cursors(void);
static void init_bar_window(Monitor *monitor, uint8_t barheight);
static xcb_screen_t *check_other_wm(xcb_connection_t *connection);
static void copy_screen_info(Monitor *m, xcb_xinerama_screen_info_t *s);
static Monitor *monitor_scan(xcb_connection_t *conn);
static void draw_tags(Monitor *m, Color scheme[SchemeLast][ColLast]);
static void update_monitor_bar(Monitor *monitor);


xcb_screen_t *check_other_wm(xcb_connection_t *connection) {
    const xcb_setup_t *setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = iter.data;

    uint32_t mask = XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_ENTER_WINDOW |
        /* XCB_EVENT_MASK_POINTER_MOTION | */
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    xcb_void_cookie_t cookie;
    cookie = xcb_change_window_attributes_checked(connection,
                                                  screen->root,
                                                  XCB_CW_EVENT_MASK,
                                                  &mask);

    xcb_generic_error_t *error = xcb_request_check(connection, cookie);
    if (error) {
        xcb_disconnect(connection);
        die("another window manager is already running");
    }

    return screen;
}

void copy_screen_info(Monitor *m, xcb_xinerama_screen_info_t *s) {
    m->mx = s->x_org;
    m->my = s->y_org;
    m->mw = s->width;
    m->mh = s->height;
    m->next = NULL;
}

Monitor *monitor_scan(xcb_connection_t *conn) {
    Monitor *monitor, *temp_monitor;

    xcb_xinerama_query_screens_cookie_t cookie;
    xcb_xinerama_query_screens_reply_t *reply;
    xcb_xinerama_screen_info_t *screen_info;

    cookie = xcb_xinerama_query_screens(conn);
    reply = xcb_xinerama_query_screens_reply(conn, cookie, NULL);
    screen_info = xcb_xinerama_query_screens_screen_info(reply);

    if (screen_info == NULL) {
        die("no monitor finded");
    }

    monitor = temp_monitor = ecalloc(1, sizeof(Monitor));
    copy_screen_info(monitor, screen_info);

    int count = xcb_xinerama_query_screens_screen_info_length(reply);

    for (int i = 1; i < count; i++) {
        temp_monitor->next = ecalloc(1, sizeof(Monitor));
        copy_screen_info(temp_monitor->next, &screen_info[i]);
        temp_monitor = temp_monitor->next;
    }

    return monitor;
}

void draw_tags(Monitor *m, Color scheme[SchemeLast][ColLast]) {
    int x = 0;
    Color *color;

    double start, end;
    start = get_time();
    for (int i = 0; i < LENGTH(tags); i++) {
        color = scheme[!(i % 2) ? SchemeSel : SchemeNorm];
        const char *tag = tags[i];
        draw_text(m->cr, tag, color, x);
        x += get_text_width(tag);
    }
    end = get_time();

    printf("time: %lf\n", end - start);
}

void update_monitor_bar(Monitor *monitor) {
    for (Monitor *mon = monitor; mon; mon = mon->next) {
        draw_tags(mon, global.color);
    }
}

void print_monitor_info(Monitor *m) {
    for (Monitor *curr = m; curr; curr = curr->next) {
        logger("x:\t%d,\ty:\t%d\n", curr->mx, curr->my);
        logger("width:\t%d,\theight:\t%d\n", curr->mw, curr->mh);
    }
}

void grabkeys(void) {
    xcb_connection_t *conn = global.conn;
    xcb_window_t root = global.screen->root;

    xcb_keycode_t key = XCB_GRAB_ANY;
    uint16_t modifiers = XCB_MOD_MASK_ANY;
    xcb_ungrab_key(conn, key, root, modifiers);
    xcb_flush(conn);

    global.keysymbol = xcb_key_symbols_alloc(conn);

    for (int i = 0; i < LENGTH(keys); i++) {
        xcb_key_symbols_t *syms = global.keysymbol;
        xcb_keysym_t keysym = keys[i].keysym;
        xcb_keycode_t *keycodesPtr = xcb_key_symbols_get_keycode(syms, keysym);
        xcb_grab_key(conn,
                     XCB_GRAB_ANY,
                     root,
                     keys[i].modifier,
                     *keycodesPtr,
                     XCB_GRAB_MODE_ASYNC,
                     XCB_GRAB_MODE_ASYNC);
    }
    xcb_flush(conn);
}

void init_cursors() {
    xcb_connection_t *conn = global.conn;
    xcb_cursor_t *cursors = global.cursors;

    xcb_font_t font = xcb_generate_id(conn);
    const char *cursorfont = "cursor";
    size_t name_len = strlen(cursorfont);
    xcb_open_font_checked(conn, font, name_len, cursorfont);

    for (int i = CurNormal; i < CurLast; i++) {
        xcb_cursor_t temp_cursor = xcb_generate_id(conn);
        xcb_create_glyph_cursor_checked(conn, temp_cursor, font, font,
                                        XC_left_ptr, XC_left_ptr + 1,
                                        0, 0, 0, 0, 0, 0);
        cursors[i] = temp_cursor;
    }
}

void init_bar_window(Monitor *monitor, uint8_t height) {
    xcb_connection_t *conn = global.conn;
    xcb_screen_t *screen = global.screen;
    xcb_cursor_t *cursors = global.cursors;

    for (Monitor *m = monitor; m; m = m->next) {
        if (m->barwin) continue;

        xcb_window_t win = xcb_generate_id(conn);
        xcb_void_cookie_t cookie;
        uint32_t value_mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_BACK_PIXMAP |
            XCB_CW_EVENT_MASK | XCB_CW_CURSOR;
        xcb_create_window_value_list_t value = {
            .override_redirect = 1,
            .background_pixmap = XCB_BACK_PIXMAP_PARENT_RELATIVE,
            .event_mask = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
            .cursor = cursors[CurNormal],
        };

        uint16_t class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
        cookie = xcb_create_window_aux_checked(conn, screen->root_depth,
                                               win, screen->root,
                                               m->mx, m->my,
                                               m->mw, height,
                                               0,
                                               class, screen->root_visual,
                                               value_mask, &value);
        if (xcb_request_check(conn, cookie)) {
            die("create bar window:");
        }

        cookie = xcb_map_window(conn, win);
        if (xcb_request_check(conn, cookie)) {
            die("map bar window:");
        }

        /**
         * set wm class use xcb api,
         * reference https://github.com/awesomeWM/awesome/blob/master/xwindow.h
         * it's equal to X11 api below
         * XClassHint ch = { "zswm", "zswm" };
         * XSetClassHint(dpy, win, &ch);
         */
        char class_name[] = "zswm\0zswm"; /* class and instance splited by \0 */
        size_t class_len = sizeof(class_name);
        xcb_icccm_set_wm_class(conn, win, class_len, class_name);

        m->barwin = win;

        cairo_surface_t *surface;
        surface = cairo_xcb_surface_create(conn, win,
                                           global.visual,
                                           m->mw, height);
        m->surface = surface;
        m->cr = cairo_create(surface);

        m->wx = m->mx;
        m->wy = m->my + height;
        m->ww = m->mw;
        m->wh = m->mh - height;

        xcb_aux_sync(conn);
    }
}

int main() {
    xcb_connection_t *conn = xcb_connect(NULL, NULL);

    if (!conn || xcb_connection_has_error(conn)) {
        die("connection:");
    }

    xcb_screen_t *screen = check_other_wm(conn);
    Monitor *monitor = monitor_scan(conn);
    print_monitor_info(monitor);
    xcb_visualtype_t *visual = find_visual(screen, screen->root_visual);

    init_pango_layout(fontfamilies, LENGTH(fontfamilies), fontsize);

    global.conn = conn;
    global.screen = screen;
    global.visual = visual;
    global.monitor = monitor;
    global.running = true;

    xcb_colormap_t cmap = screen->default_colormap;
    for (int i = 0; i < SchemeLast; i++) {
        for (int j = 0; j < ColLast; j++) {
            global.color[i][j] = create_color(conn, cmap, colors[i][j]);
        }
    }

    init_bar_window(monitor, get_barheight());
    init_cursors();
    update_monitor_bar(monitor);

    xcb_flush(conn);

    grabkeys();
    xcb_generic_event_t *event;

    while (global.running && (event = xcb_wait_for_event(conn))) {
        event_handle(event);
    }

    xcb_disconnect(conn);

    return EXIT_SUCCESS;
}
