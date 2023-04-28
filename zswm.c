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
#include "pango/pango-attributes.h"
#include "pango/pango-layout.h"
#include "pango/pangocairo.h"
#include "pango/pango-types.h"
#include "utils.h"

zswm_global_t global;

typedef struct {
    uint8_t lrpad;
    uint8_t barheight;
    PangoLayout *layout;
} PangoInit;

static void init_cursors(void);
static PangoInit init_pango(char **families, int length, uint8_t size);
static void init_bar_window(Monitor *monitor, uint8_t barheight);
static xcb_screen_t *check_other_wm(xcb_connection_t *connection);
static void copy_screen_info(Monitor *m, xcb_xinerama_screen_info_t *s);
static Monitor *monitor_scan(xcb_connection_t *conn);

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

PangoInit init_pango(char **families, int length, uint8_t size) {
    PangoFontMap *fontmap = pango_cairo_font_map_new();
    PangoContext *context = pango_font_map_create_context(fontmap);
    PangoLayout *layout = pango_layout_new(context);
    PangoLanguage *lang = pango_context_get_language(context);
    PangoAttrList *list = pango_attr_list_new();

    PangoInit init = { .layout = layout, .barheight = 0 };

    uint8_t padding = 0;
    uint8_t bh = 0;

    for (int i = 0; i < length; i++) {
        PangoFontDescription *desc = pango_font_description_new();
        pango_font_description_set_family(desc, families[i]);
        pango_font_description_set_size(desc, size * PANGO_SCALE);

        PangoAttribute *attr = pango_attr_font_desc_new(desc);
        pango_attr_list_insert(list, attr);

        PangoFontMetrics *metrics;
        metrics =pango_context_get_metrics(context, desc, lang);

        int height = PANGO_PIXELS(pango_font_metrics_get_height(metrics));
        int ascent = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics));
        int descent = PANGO_PIXELS(pango_font_metrics_get_descent(metrics));

        int inner_bh = MIN(height, ascent + descent);
        padding = MIN(inner_bh, bh);
        bh = MAX(inner_bh, bh);

        free(desc);
        free(metrics);
    }

    init.lrpad = padding / 2 - 1;

    pango_layout_set_attributes(layout, list);

    if (bh % 2) {
        bh += 1;
    }

    init.barheight = bh;

    return init;
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
    int length = LENGTH(font_families);
    PangoInit init = init_pango(font_families, length, font_size);

    global.conn = conn;
    global.screen = screen;
    global.visual = visual;
    global.monitor = monitor;
    global.lrpad = init.lrpad;
    global.barheight = init.barheight;
    global.layout = init.layout;
    global.running = true;

    global.color = ecalloc(LENGTH(colors), sizeof(PangoColor *));
    for (int i = 0; i < LENGTH(colors); i++) {
        global.color[i] = create_scheme(colors[i], ColLast);
    }

    init_bar_window(monitor, init.barheight);
    init_cursors();
    update_monitor_bar(monitor, init.layout, init.barheight, init.lrpad);

    xcb_flush(conn);

    grabkeys();
    xcb_generic_event_t *event;
    while (global.running && (event = xcb_wait_for_event(conn))) {
        event_handle(event);
    }

    xcb_disconnect(conn);

    return EXIT_SUCCESS;
}
