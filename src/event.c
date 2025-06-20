#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xproto.h>

#include "config.h"
#include "draw.h"
#include "event.h"
#include "utils.h"
#include "window.h"

static bool need_refresh_bar = false;

static void create_notify(xcb_create_notify_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n", ev->window, ev->parent,
           global.screen->root);
}

static void map_request(xcb_map_request_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n", ev->window, ev->parent,
           global.screen->root);
    if (get_client_of_window(ev->window)) {
        return;
    }

    if (ev->parent == global.screen->root) {
        manage_window(ev->window);
        xcb_flush(global.conn);
    }
}

static void map_notify(xcb_map_notify_event_t *ev) {
    need_refresh_bar = true;
    logger("window: %d, root: %d\n", ev->window, global.screen->root);
}

static void unmap_notify(xcb_unmap_notify_event_t *ev) {
    need_refresh_bar = true;
    logger("unmap window: %d\n", ev->window);

    Client *c = get_client_of_window(ev->window);
    if (c != NULL) {
        unmanage_client(c, false);
    }
}

static void configure_request(xcb_configure_request_event_t *ev) {
    logger("window: %d, parent: %d, root: %d\n", ev->window, ev->parent,
           global.screen->root);
    logger("x: %d, y: %d, width: %d, height: %d\n", ev->x, ev->y, ev->width,
           ev->height);

    xcb_cw_t mask = XCB_CW_BORDER_PIXEL;
    xcb_params_cw_t params = {
        .border_pixel = global.color[SchemeSel][ColBorder].xcb_color_pixel,
    };
    xcb_aux_change_window_attributes(global.conn, ev->window, mask, &params);

    xcb_config_window_t config_mask =
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH |
        XCB_CONFIG_WINDOW_STACK_MODE;
    int border = 1;
    xcb_params_configure_window_t window_config = {
        .x = global.current_monitor->wx,
        .y = global.current_monitor->wy,
        .width = global.current_monitor->ww - (border * 2),
        .height = global.current_monitor->wh - (border * 2),
        .border_width = border,
        .stack_mode = XCB_STACK_MODE_ABOVE,
    };
    xcb_aux_configure_window(global.conn, ev->window, config_mask,
                             &window_config);

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

            need_refresh_bar = true;
            return;
        }
    }
}

static void button_press(xcb_button_press_event_t *ev) {
    if (ev->event == global.current_monitor->barwin) {
        int i, x;
        i = x = 0;
        do {
            x += get_text_width(tags[i]) + tag_lrpad * 2;
        } while (ev->event_x >= x && (++i) < LENGTH(tags));

        if (i >= LENGTH(tags)) {
            return;
        }
        uint16_t target_tag = 1 << i;
        if (global.current_monitor->seltags & target_tag) {
            return;
        }

        Arg arg = {.ui = i + 1};
        change_select_tag(&arg);
        logger("=== jump to tag: %u\n", i + 1);
        need_refresh_bar = true;
    }
}

static void client_message(xcb_client_message_event_t *ev) {
    logger("format: %d, type: %d\n", ev->format, ev->type);
}

static void destory_notify(xcb_destroy_notify_event_t *ev) {
    Client *c = get_client_of_window(ev->window);
    if (c != NULL) {
        unmanage_client(c, true);
    }
    need_refresh_bar = true;
}

static void motion_notify(xcb_motion_notify_event_t *ev) {
    if (ev->root != global.screen->root)
        return;
    logger("motion notify: x: %d,\t y: %d\n", ev->root_x, ev->root_y);

    Monitor *mon = xy_to_monitor(global.monitors, ev->root_x, ev->root_y);
    global.current_monitor = mon;
}

static void enter_notify(xcb_enter_notify_event_t *ev) {
    xcb_window_t window = ev->event;
    if (window == global.screen->root)
        return;

    uint32_t color_pixel = global.color[SchemeSel][ColBorder].xcb_color_pixel;
    change_window_border_color(window, color_pixel);

    xcb_set_input_focus(global.conn, XCB_INPUT_FOCUS_POINTER_ROOT, window,
                        XCB_CURRENT_TIME);
    xcb_flush(global.conn);
}

static void leave_notify(xcb_leave_notify_event_t *ev) {
    xcb_window_t window = ev->event;
    if (window == global.screen->root)
        return;

    uint32_t color_pixel = global.color[SchemeNorm][ColBorder].xcb_color_pixel;
    change_window_border_color(window, color_pixel);
}

static void focus_in(xcb_focus_in_event_t *ev) {
    need_refresh_bar = true;
    logger("focus in\n");
}

static void focus_out(xcb_focus_out_event_t *ev) {
    need_refresh_bar = true;
    logger("focus out\n");
}

static void property_notify(xcb_leave_notify_event_t *ev) {
    logger("property notify\n");
}

bool event_handle(xcb_generic_event_t *event) {
    uint8_t event_type = XCB_EVENT_RESPONSE_TYPE(event);
    const char *label = xcb_event_get_label(event_type);
    if (event_type != XCB_MOTION_NOTIFY && event_type != XCB_BUTTON_PRESS &&
        event_type != XCB_ENTER_NOTIFY && event_type != XCB_LEAVE_NOTIFY) {
        logger("---------------- %s ---------------------\n", label);
    }

    need_refresh_bar = false;

    switch (event_type) {
#define EVENT(type, callback)                                                  \
    case type:                                                                 \
        callback((void *)event);                                               \
        break

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
        EVENT(XCB_FOCUS_IN, focus_in);
        EVENT(XCB_FOCUS_OUT, focus_out);
        EVENT(XCB_PROPERTY_NOTIFY, property_notify);
        EVENT(XCB_UNMAP_NOTIFY, unmap_notify);

#undef EVENT
    }

    free(event);

    return need_refresh_bar;
}
