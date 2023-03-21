#include <stdio.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "config.h"
#include "event.h"
#include "utils.h"
#include "zswm.h"

static void create_notify(xcb_create_notify_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n", ev->window, ev->parent, root);
}

static void map_request(xcb_map_request_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n", ev->window, ev->parent, root);
    xcb_map_window(connection, ev->window);
    xcb_map_subwindows(connection, ev->window);

    if (ev->parent == root) {
        uint32_t value_list[] = { XCB_EVENT_MASK_ENTER_WINDOW, XCB_EVENT_MASK_LEAVE_WINDOW };
        xcb_change_window_attributes(connection, ev->window, XCB_CW_EVENT_MASK, value_list);
        xcb_configure_window(connection, ev->window, XCB_CONFIG_WINDOW_Y, &(uint32_t[]){ 20 });
    }

    xcb_flush(connection);
}

static void map_notify(xcb_map_notify_event_t *ev) {
    logger("window: %d, root: %d\n", ev->window, root);
}

static void configure_request(xcb_configure_request_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n", ev->window, ev->parent, root);
    logger("x: %d, y: %d, width: %d, height: %d\n", ev->x, ev->y, ev->width, ev->height);
    xcb_params_configure_window_t params = {
        .x = ev->x,
        .y = ev->y,
        .width = ev->width,
        .height = ev->height,
        .border_width = ev->border_width,
        .sibling = ev->sibling,
        .stack_mode = ev->stack_mode,
    };
    xcb_aux_configure_window(connection, ev->window, ev->value_mask, &params);
    xcb_flush(connection);
}

static void keypress(xcb_key_press_event_t *ev) {
    xcb_keysym_t keysym = xcb_key_press_lookup_keysym(keysyms, ev, 0);
    logger("keysym: %d\n", keysym);

    for (int i = 0; i < LENGTH(keys); i++) {
        if (keysym == keys[i].keysym && ev->state == keys[i].modifier && keys[i].func) {
            keys[i].func(&keys[i].arg);
            return;
        }
    }
}

static void button_press(xcb_button_press_event_t *ev) {
    logger("root: %d, event: %d, child: %d\n", ev->root, ev->event, ev->child);
}

static void client_message(xcb_client_message_event_t *ev) {
    logger("format: %d, type: %d\n", ev->format, ev->type);
}

static void destory_notify(xcb_destroy_notify_event_t *ev) {
}

static void motion_notify(xcb_motion_notify_event_t *ev) {
    if (ev->root != root) return;
    /* logger("notion notify: x: %d,\t y: %d\n", ev->root_x, ev->root_y); */
}

static void enter_notify(xcb_enter_notify_event_t *ev) {
    logger("x: %d,\t y: %d,\t root: %d\n", ev->root_x, ev->root_y, root);
    logger("root: %d,\t event: %d,\t child: %d\n", ev->root, ev->event, ev->root);

    if (ev->event == root) return;

    uint16_t value_mask = XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH;
    xcb_configure_window_value_list_t value_list = {
        .y = 20,
        .width = screen->width_in_pixels - 2,
        .height = screen->height_in_pixels - 2 - 20,
        .border_width = 1,
    };
    xcb_configure_window_aux(connection, ev->event, value_mask, &value_list);
    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connection, ev->event);
    xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(connection, cookie, NULL);
    free(reply);

    uint32_t blue = alloc_color("#FF00FF");
    xcb_change_window_attributes(connection, ev->event, XCB_CW_BORDER_PIXEL, &blue);

    xcb_flush(connection);
}

static void leave_notify(xcb_leave_notify_event_t *ev) {
    uint32_t value_mask = XCB_CW_BORDER_PIXEL;
    uint32_t value_list[] = { 0 };
    logger("x: %d,\t y: %d,\t root: %d\n", ev->root_x, ev->root_y, root);
    logger("root: %d,\t event: %d,\t child: %d\n", ev->root, ev->event, ev->root);
    xcb_change_window_attributes(connection, ev->root, value_mask, value_list);
    xcb_flush(connection);
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
