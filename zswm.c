#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xinerama.h>
#include <xcb/xproto.h>
#include <inttypes.h>
#include "utils.h"

double xcb(xcb_connection_t *connection) {
    double start, end;

    start = get_time();

    xcb_xinerama_query_screens_cookie_t cookie = xcb_xinerama_query_screens(connection);
    xcb_xinerama_query_screens_reply_t *screens_reply = xcb_xinerama_query_screens_reply(connection, cookie, NULL);
    int count = xcb_xinerama_query_screens_screen_info_length(screens_reply);
    xcb_xinerama_screen_info_t *screen_info = xcb_xinerama_query_screens_screen_info(screens_reply);

    for (int i = 0; i < count; i++) {
        xcb_xinerama_screen_info_t temp = screen_info[i];
        printf("x: %d, y: %d, width: %d, height: %d\n", temp.x_org, temp.y_org, temp.width, temp.height);
    }

    end = get_time();
    return end - start;
}

void check_other_wm(xcb_connection_t *connection) {
    const xcb_setup_t *setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = iter.data;

    uint32_t mask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(connection, screen->root, XCB_CW_EVENT_MASK, &mask);

    xcb_generic_error_t *error = xcb_request_check(connection, cookie);
    if (error) {
        xcb_disconnect(connection);
        die("another window manager is already running");
    }
}

int main() {
    double start = get_time();

    xcb_connection_t *connection = xcb_connect(NULL, NULL);
    check_other_wm(connection);
    double cost = xcb(connection);

    double end = get_time();
    printf("get monitor cost: %f, all cost: %f\n", cost, end - start);
    while (1);

    xcb_disconnect(connection);

    return EXIT_SUCCESS;
}
