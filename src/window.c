#include "window.h"
#include "global.h"
#include <xcb/xproto.h>

void change_window_border_color(xcb_window_t window, uint32_t color_pixel) {
    xcb_connection_t *c = global.conn;
    uint32_t mask = XCB_CW_BORDER_PIXEL;
    xcb_params_cw_t params = {.border_pixel = color_pixel};
    xcb_aux_change_window_attributes(c, window, mask, &params);

    xcb_flush(c);
}

xcb_get_window_attributes_reply_t *get_window_attributes(xcb_window_t window) {
    xcb_connection_t *conn = global.conn;
    xcb_get_window_attributes_cookie_t cookie =
        xcb_get_window_attributes(conn, window);
    xcb_get_window_attributes_reply_t *replay =
        xcb_get_window_attributes_reply(conn, cookie, NULL);
    return replay;
}

void manage_window(xcb_window_t window) {
    xcb_connection_t *conn = global.conn;
    xcb_cw_t change_mask = XCB_CW_EVENT_MASK | XCB_CW_BORDER_PIXEL;
    xcb_params_cw_t params = {
        .event_mask =
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY,
        .border_pixel = global.color[SchemeNorm][ColBorder].xcb_color_pixel,
    };
    xcb_aux_change_window_attributes(conn, window, change_mask, &params);

    xcb_config_window_t config_mask =
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH;
    int border = 1;
    xcb_params_configure_window_t window_config = {
        .x = global.current_monitor->wx,
        .y = global.current_monitor->wy,
        .width = global.current_monitor->ww - (border * 2),
        .height = global.current_monitor->wh - (border * 2),
        .border_width = border,
    };
    xcb_aux_configure_window(conn, window, config_mask, &window_config);
    xcb_flush(conn);
}
