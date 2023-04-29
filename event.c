#include "config.h"
#include "draw.h"
#include "event.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xproto.h>

static void create_notify(xcb_create_notify_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n",
           ev->window, ev->parent, global.screen->root);
}

static void map_request(xcb_map_request_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n",
           ev->window, ev->parent, global.screen->root);
    xcb_map_window(global.conn, ev->window);
    xcb_map_subwindows(global.conn, ev->window);

    if (ev->parent == global.screen->root) {
        xcb_cw_t change_mask = XCB_CW_EVENT_MASK | XCB_CW_BORDER_PIXEL;
        xcb_change_window_attributes_value_list_t value_list = {
            .event_mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW,
            .border_pixel = global.color[SchemeNorm][ColBorder].xcb_color_pixel,
        };
        xcb_change_window_attributes(global.conn,
                                     ev->window,
                                     XCB_CW_EVENT_MASK,
                                     &value_list);

        xcb_config_window_t config_mask = XCB_CONFIG_WINDOW_X |
            XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
            XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH;
        xcb_configure_window_value_list_t window_config = {
            .x = global.monitors->wx,
            .y = global.monitors->wy,
            .width = global.monitors->ww,
            .height = global.monitors->wh,
            .border_width = 1,
        };
        xcb_configure_window_aux(global.conn,
                                 ev->window,
                                 config_mask,
                                 &window_config);
    }

    xcb_flush(global.conn);
}

static void map_notify(xcb_map_notify_event_t *ev) {
    logger("window: %d, root: %d\n", ev->window, global.screen->root);
}

static void configure_request(xcb_configure_request_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n",
           ev->window, ev->parent, global.screen->root);
    logger("x: %d, y: %d, width: %d, height: %d\n",
           ev->x, ev->y, ev->width, ev->height);
    xcb_params_configure_window_t params = {
        .x = ev->x,
        .y = ev->y,
        .width = ev->width,
        .height = ev->height,
        .border_width = ev->border_width,
        .sibling = ev->sibling,
        .stack_mode = ev->stack_mode,
    };

    xcb_window_t window = ev->window;
    uint16_t mask = ev->value_mask;
    xcb_aux_configure_window(global.conn, window, mask, &params);
    xcb_flush(global.conn);
}

static void keypress(xcb_key_press_event_t *ev) {
    xcb_key_symbols_t *syms = global.keysymbol;
    xcb_keysym_t keysym = xcb_key_press_lookup_keysym(syms, ev, 0);
    logger("keysym: %d\n", keysym);

    for (int i = 0; i < LENGTH(keys); i++) {
        Key key = keys[i];
        if (keysym == key.keysym && ev->state == key.modifier && key.func) {
            key.func(&key.arg);
            return;
        }
    }
}

static void button_press(xcb_button_press_event_t *ev) {
    ClickType click = ClkRootWin;
    if (ev->event == global.current_monitor->barwin) {
        int i, x;
        i = x = 0;
        do {
            x += get_text_width(tags[i]);
        } while (ev->event_x >= x && (++i) < LENGTH(tags));

        if (i < LENGTH(tags)) {
            global.current_monitor->seltags = 1 << i;
        }
    }
}

static void client_message(xcb_client_message_event_t *ev) {
    logger("format: %d, type: %d\n", ev->format, ev->type);
}

static void destory_notify(xcb_destroy_notify_event_t *ev) {
}

static void motion_notify(xcb_motion_notify_event_t *ev) {
    if (ev->root != global.screen->root) return;
    printf("motion notify: x: %d,\t y: %d\n", ev->root_x, ev->root_y);

    Monitor *mon = xy_to_monitor(global.monitors, ev->root_x, ev->root_y);
    global.current_monitor = mon;
}

static void enter_notify(xcb_enter_notify_event_t *ev) {
    const char *pos_fmt = "x: %d,\t y: %d,\t root: %d\n";
    const char *win_fmt = "root: %d,\t event: %d,\t child: %d\n";
    logger(pos_fmt, ev->root_x, ev->root_y, global.screen->root);
    logger(win_fmt, ev->root, ev->event, ev->root);

    if (ev->event == global.screen->root) return;

    uint16_t value_mask = XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH;
    xcb_configure_window_value_list_t value_list = {
        .y = 20,
        .width = global.screen->width_in_pixels - 2,
        .height = global.screen->height_in_pixels - 2 - 20,
        .border_width = 1,
    };

    xcb_connection_t *c = global.conn;
    xcb_window_t window = ev->event;
    xcb_configure_window_aux(c, window, value_mask, &value_list);
    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(c, window);
    xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(c, cookie, NULL);
    free(reply);

    uint32_t blue = alloc_color("#FF00FF");
    uint32_t mask = XCB_CW_BORDER_PIXEL;
    xcb_change_window_attributes(c, window, mask, &blue);

    xcb_flush(global.conn);
}

static void leave_notify(xcb_leave_notify_event_t *ev) {
    uint32_t value_mask = XCB_CW_BORDER_PIXEL;
    uint32_t value_list[] = { 0 };

    const char *pos_fmt = "x: %d,\t y: %d,\t root: %d\n";
    const char *win_fmt = "root: %d,\t event: %d,\t child: %d\n";
    logger(pos_fmt, ev->root_x, ev->root_y, global.screen->root);
    logger(win_fmt, ev->root, ev->event, ev->root);

    xcb_connection_t *c = global.conn;
    xcb_change_window_attributes(c, ev->root, value_mask, value_list);
    xcb_flush(c);
}


void event_handle(xcb_generic_event_t *event) {
    uint8_t event_type = XCB_EVENT_RESPONSE_TYPE(event);
    const char *label = xcb_event_get_label(event_type);
    logger("---------------- %s ---------------------\n", label);

    switch (event_type) {
#define EVENT(type, callback) case type: callback((void *) event); break
        EVENT(XCB_CREATE_NOTIFY, create_notify);
        EVENT(XCB_MAP_REQUEST, map_request);
        EVENT(XCB_MAP_NOTIFY, map_notify);
        EVENT(XCB_CONFIGURE_REQUEST, configure_request);
        EVENT(XCB_DESTROY_NOTIFY, destory_notify);
        EVENT(XCB_CLIENT_MESSAGE, client_message);
        EVENT(XCB_KEY_PRESS, keypress);
        EVENT(XCB_BUTTON_PRESS, button_press);
        EVENT(XCB_MOTION_NOTIFY, motion_notify);
        EVENT(XCB_ENTER_NOTIFY, enter_notify);
        EVENT(XCB_LEAVE_NOTIFY, leave_notify);
#undef EVENT
    }

    free(event);
}
