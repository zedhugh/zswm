#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xinerama.h>
#include <xcb/xproto.h>

#include "config.h"
#include "event.h"
#include "utils.h"
#include "zswm.h"


xcb_connection_t *connection;
xcb_screen_t *screen;
xcb_window_t root;
xcb_key_symbols_t *keysyms;
int running;
xcb_cursor_t cursor[CurLast];


void check_other_wm(xcb_connection_t *connection) {
    const xcb_setup_t *setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    screen = iter.data;

    uint32_t mask = XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_ENTER_WINDOW |
        /* XCB_EVENT_MASK_POINTER_MOTION | */
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(connection, screen->root, XCB_CW_EVENT_MASK, &mask);

    xcb_generic_error_t *error = xcb_request_check(connection, cookie);
    if (error) {
        xcb_disconnect(connection);
        die("another window manager is already running");
    }
    root = screen->root;
}

void copy_screen_info(Monitor *m, xcb_xinerama_screen_info_t *s) {
    m->mx = s->x_org;
    m->my = s->y_org;
    m->mw = s->width;
    m->mh = s->height;
    m->next = NULL;
}

Monitor *monitor_scan(xcb_connection_t *connection) {
    Monitor *monitor, *temp_monitor;

    xcb_xinerama_query_screens_cookie_t cookie = xcb_xinerama_query_screens(connection);
    xcb_xinerama_query_screens_reply_t *screens_reply = xcb_xinerama_query_screens_reply(connection, cookie, NULL);
    xcb_xinerama_screen_info_t *screen_info = xcb_xinerama_query_screens_screen_info(screens_reply);

    if (screen_info == NULL) {
        die("no monitor finded");
    }

    monitor = temp_monitor = ecalloc(1, sizeof(Monitor));
    copy_screen_info(monitor, screen_info);

    int count = xcb_xinerama_query_screens_screen_info_length(screens_reply);

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
    xcb_ungrab_key(connection, XCB_GRAB_ANY, root, XCB_MOD_MASK_ANY);
    xcb_flush(connection);

    keysyms = xcb_key_symbols_alloc(connection);

    for (int i = 0; i < LENGTH(keys); i++) {
        xcb_keycode_t *keycodesPtr = xcb_key_symbols_get_keycode(keysyms, keys[i].keysym);
        xcb_grab_key(connection, XCB_GRAB_ANY, root, keys[i].modifier, *keycodesPtr, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
    xcb_flush(connection);
}

static void init_cursors() {
    xcb_font_t font = xcb_generate_id(connection);
    const char *cursorfont = "cursor";
    xcb_open_font_checked(connection, font, strlen(cursorfont), cursorfont);

    for (int i = CurNormal; i < CurLast; i++) {
        xcb_cursor_t temp_cursor = xcb_generate_id(connection);
        xcb_create_glyph_cursor_checked(connection, temp_cursor,
                                        font, font,
                                        XC_left_ptr, XC_left_ptr + 1,
                                        0, 0, 0, 0, 0, 0);
        cursor[i] = temp_cursor;
    }
}

static void update_bar(Monitor *monitor, uint16_t barheight) {
    for (Monitor *m = monitor; m; m = m->next) {
        if (m->barwin) continue;

        xcb_window_t win = xcb_generate_id(connection);
        xcb_void_cookie_t cookie;
        uint32_t mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_BACK_PIXMAP |
            XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_CURSOR;
        xcb_create_window_value_list_t value = {
            .override_redirect = 1,
            .background_pixmap = XCB_BACK_PIXMAP_PARENT_RELATIVE,
            .background_pixel = alloc_color("#00FF00"),
            .event_mask = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
            .cursor = cursor[CurNormal],
        };
        cookie = xcb_create_window_aux_checked(connection,
                                               screen->root_depth,
                                               win,
                                               screen->root,
                                               m->mx, m->my,
                                               m->mw, barheight,
                                               0,
                                               XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                               screen->root_visual,
                                               mask, &value);
        if (xcb_request_check(connection, cookie)) {
            die("create bar window:");
        }



        cookie = xcb_map_window(connection, win);
        if (xcb_request_check(connection, cookie)) {
            die("map bar window:");
        }

        /**
         * set wm class use xcb api, reference https://github.com/awesomeWM/awesome/blob/master/xwindow.h
         * it's equal to X11 api below
         * XClassHint ch = { "zswm", "zswm" };
         * XSetClassHint(dpy, win, &ch);
         */
        char class_name[] = "zswm\0zswm"; /* class and instance splited by \0 */
        xcb_icccm_set_wm_class(connection, win, sizeof(class_name), class_name);

        xcb_aux_sync(connection);
        m->barwin = win;
    }
}


int main() {
    connection = xcb_connect(NULL, NULL);
    int err_code = xcb_connection_has_error(connection);
    if (err_code != 0) {
        die("connection:");
    }
    check_other_wm(connection);

    running = 1;

    Monitor *monitor = monitor_scan(connection);
    print_monitor_info(monitor);
    init_cursors();
    update_bar(monitor, 20);

    grabkeys();
    xcb_generic_event_t *event;
    while (running && (event = xcb_wait_for_event(connection))) {
        event_handle(event);
    }

    xcb_disconnect(connection);

    return EXIT_SUCCESS;
}
