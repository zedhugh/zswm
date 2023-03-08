#include <stdio.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xproto.h>

#include "event.h"
#include "zswm.h"

static void create_notify(xcb_create_notify_event_t *ev) {
}

static void map_request(xcb_map_request_event_t *ev) {
    xcb_map_window(connection, ev->window);
    xcb_map_subwindows(connection, ev->window);
    xcb_flush(connection);
}

static void configure_request(xcb_configure_request_event_t *ev) {
    xcb_params_configure_window_t params;
    params.x = ev->x;
    params.y = ev->y;
    params.width = ev->width;
    params.height = ev->height;
    params.sibling = ev->sibling;
    params.stack_mode = ev->stack_mode;
    params.border_width = ev->border_width;
    xcb_aux_configure_window(connection, ev->window, ev->value_mask, &params);
    printf("config: %d, x: %d, y: %d, width: %d, height: %d\n", ev->window, ev->x, ev->y, ev->width, ev->height);
    xcb_flush(connection);
}

static void client_message(xcb_client_message_event_t *ev) {
}

static void destory_notify(xcb_destroy_notify_event_t *ev) {
}

void event_handle(xcb_generic_event_t *event) {
    uint8_t event_type = XCB_EVENT_RESPONSE_TYPE(event);

    switch (event_type) {
#define EVENT(type, callback) case type: callback((void *) event); return
        EVENT(XCB_CREATE_NOTIFY, create_notify);
        EVENT(XCB_MAP_REQUEST, map_request);
        EVENT(XCB_CONFIGURE_REQUEST, configure_request);
        EVENT(XCB_DESTROY_NOTIFY, destory_notify);
        EVENT(XCB_CLIENT_MESSAGE, client_message);
#undef EVENT
    default: {
        const char *label = xcb_event_get_label(event_type);
        printf("recv event: %d, %s\n", event_type, label);
    } }
}
