#include <X11/keysym.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
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
    m->x = s->x_org;
    m->y = s->y_org;
    m->width = s->width;
    m->height = s->height;
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
        logger("x:\t%d,\ty:\t%d\n", curr->x, curr->y);
        logger("width:\t%d,\theight:\t%d\n", curr->width, curr->height);
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

    grabkeys();
    xcb_generic_event_t *event;
    while (running && (event = xcb_wait_for_event(connection))) {
        event_handle(event);
    }

    xcb_disconnect(connection);

    return EXIT_SUCCESS;
}
