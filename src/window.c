#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "config.h"
#include "utils.h"
#include "window.h"

static void show_clients(Monitor *monitors) {
    printf("================== show monitors and clients ==================\n");
    int mi = 0;
    for (Monitor *m = monitors; m; m = m->next) {
        printf("=== monitor: %d\n", mi++);
        int ci = 0;
        for (Client *c = m->clients; c; c = c->next) {
            printf("=== client %d: %s\n", ci++, c->name);
        }
    }
    printf("================ show monitors and clients end ================\n");
}

/**
 * set wm class use xcb api,
 * reference https://github.com/awesomeWM/awesome/blob/master/xwindow.h.
 * It's equal to X11 api below:
 * XClassHint ch = { "zswm", "zswm" };
 * XSetClassHint(dpy, win, &ch);
 */
void set_window_class_instance(xcb_connection_t *conn, xcb_window_t window,
                               const char *class, const char *instance) {
    size_t class_length = strlen(class) + 1;
    size_t instance_length = strlen(instance) + 1;
    size_t length = class_length + instance_length;

    char *str = malloc(length * sizeof(char));
    for (size_t i = 0; i < length; i++) {
        if (i < class_length) {
            str[i] = class[i];
        } else {
            str[i] = instance[i - class_length];
        }
    }

    xcb_icccm_set_wm_class(conn, window, length, str);
    free(str);
}

xcb_icccm_get_wm_class_reply_t get_window_class_instance(xcb_connection_t *conn,
                                                         xcb_window_t window) {
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(conn, window);
    xcb_icccm_get_wm_class_reply_t prop;
    xcb_icccm_get_wm_class_reply(conn, cookie, &prop, NULL);
    return prop;
}

/**
 * get client of window
 *
 * if client exist return the client, otherwise return NULL.
 */
Client *get_client_of_window(xcb_window_t window) {
    for (Monitor *m = global.monitors; m; m = m->next) {
        for (Client *c = m->clients; c; c = c->next) {
            if (c->win == window) {
                return c;
            }
        }
    }
    return NULL;
}

void change_window_rect(xcb_window_t window, int32_t x, int32_t y,
                        uint32_t width, uint32_t height,
                        uint32_t border_width) {
    xcb_connection_t *conn = global.conn;
    xcb_config_window_t mask =
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH;
    xcb_params_configure_window_t config = {.x = x,
                                            .y = y,
                                            .width = width,
                                            .height = height,
                                            .border_width = border_width};
    xcb_aux_configure_window(conn, window, mask, &config);
    xcb_flush(conn);
}

void change_window_border_color(xcb_window_t window, uint32_t color_pixel) {
    xcb_connection_t *c = global.conn;
    uint32_t mask = XCB_CW_BORDER_PIXEL;
    xcb_params_cw_t params = {.border_pixel = color_pixel};
    xcb_aux_change_window_attributes(c, window, mask, &params);

    xcb_flush(c);
}

xcb_get_window_attributes_reply_t get_window_attributes(xcb_window_t window) {
    xcb_connection_t *conn = global.conn;
    xcb_get_window_attributes_cookie_t cookie =
        xcb_get_window_attributes(conn, window);
    xcb_get_window_attributes_reply_t *reply =
        xcb_get_window_attributes_reply(conn, cookie, NULL);
    xcb_get_window_attributes_reply_t attributes = *reply;
    free(reply);
    return attributes;
}

xcb_get_geometry_reply_t get_window_geometry(xcb_window_t window) {
    xcb_connection_t *conn = global.conn;
    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, window);
    xcb_get_geometry_reply_t *reply =
        xcb_get_geometry_reply(conn, cookie, NULL);
    xcb_get_geometry_reply_t geometry = *reply;
    free(reply);
    return geometry;
}

xcb_window_t get_transient_window_for(xcb_window_t window) {
    xcb_connection_t *conn = global.conn;
    xcb_window_t transient_for = XCB_NONE;
    xcb_get_property_cookie_t cookie =
        xcb_icccm_get_wm_transient_for(conn, window);
    xcb_icccm_get_wm_transient_for_reply(conn, cookie, &transient_for, NULL);
    return transient_for;
}

void configure_client(xcb_window_t window) {
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
}

void update_client_name(Client *client) {
    xcb_get_property_cookie_t cookie =
        xcb_icccm_get_wm_name(global.conn, client->win);
    xcb_icccm_get_text_property_reply_t reply;
    xcb_icccm_get_wm_name_reply(global.conn, cookie, &reply, NULL);
    strncpy(client->name, reply.name, sizeof(client->name) - 1);
    if (client->name[0] == '\0') {
        strcpy(client->name, "broken");
    }
}

void attach_client(Client *client) {
    client->next = client->mon->clients;
    client->mon->clients = client;
}
void detach_client(Client *client) {
    Client **c = &client->mon->clients;
    while (*c && *c != client) {
        c = &(*c)->next;
    }
    *c = client->next;
}

void apply_rules(Client *client) {
    client->tags = global.current_monitor->seltags;
    xcb_connection_t *conn = global.conn;
    xcb_icccm_get_wm_class_reply_t prop =
        get_window_class_instance(conn, client->win);

    client->tags = 0;
    client->isfullscreen = false;

    const Rule *r = NULL;
    for (int i = 0; i < LENGTH(rules); i++) {
        r = &rules[i];
        if ((!r->class || strcmp(prop.class_name, r->class) == 0) &&
            (!r->instance || strcmp(prop.instance_name, r->instance) == 0)) {

            client->tags |= r->tags;
            client->isfullscreen = r->fullscreen;
            Monitor *m = NULL;
            for (m = global.monitors; m && m->num == r->monitor; m = m->next) {
            }
            if (m) {
                client->mon = m;
            }
            if (r->switch_to_tag) {
                client->mon->seltags = r->tags;
            }
        }
    }
    unsigned int target_tag = client->tags & TAGMASK;
    client->tags = target_tag ? target_tag : client->mon->seltags;
}

void manage_window(xcb_window_t window) {
    Client *c = ecalloc(1, sizeof(Client));
    c->win = window;

    xcb_get_geometry_reply_t geometry = get_window_geometry(window);
    c->x = c->old_x = geometry.x;
    c->y = c->old_y = geometry.y;
    c->width = c->old_width = geometry.width;
    c->height = c->old_height = geometry.height;
    c->old_bw = geometry.border_width;
    c->bw = border_px;
    update_client_name(c);

    xcb_window_t trans_for = get_transient_window_for(window);
    Client *t = NULL;
    if (trans_for != XCB_NONE && (t = get_client_of_window(trans_for))) {
        c->mon = t->mon;
        c->tags = t->tags;
    } else {
        c->mon = global.current_monitor;
        apply_rules(c);
    }

    attach_client(c);

    if (c->mon && c->mon->layout && c->mon->layout->arrange) {
        c->mon->layout->arrange(c->mon);
    }
    configure_client(window);

    show_clients(global.monitors);

    xcb_connection_t *conn = global.conn;

    xcb_map_window(conn, window);
    xcb_map_subwindows(conn, window);
    show_and_hide_client(c);

    xcb_flush(conn);
}

void unmanage_client(Client *client, bool destroyed) {
    Monitor *m = client->mon;
    detach_client(client);
    show_clients(global.monitors);
    if (!destroyed) {
        xcb_connection_t *conn = global.conn;
        xcb_window_t window = client->win;
        xcb_cw_t change_mask = XCB_CW_EVENT_MASK;
        xcb_params_cw_t params = {.event_mask = XCB_EVENT_MASK_NO_EVENT};
        xcb_aux_change_window_attributes(conn, window, change_mask, &params);
        xcb_config_window_t config_mask = XCB_CONFIG_WINDOW_BORDER_WIDTH;
        xcb_params_configure_window_t window_config = {
            .border_width = client->old_bw,
        };
        xcb_aux_configure_window(conn, window, config_mask, &window_config);
    }
    free(client);

    if (m && m->layout && m->layout->arrange) {
        m->layout->arrange(m);
    }
}

void show_and_hide_client(Client *client) {
    if (!client) {
        return;
    }

    xcb_connection_t *conn = global.conn;
    xcb_config_window_t config_mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
    xcb_params_configure_window_t window_config;
    if (CLIENT_IS_VISIBLE(client)) {
        window_config.x = client->x;
        window_config.y = client->y;
    } else {
        window_config.x = client->mon->mx;
        window_config.y = client->mon->mx + client->mon->mh;
    }
    xcb_aux_configure_window(conn, client->win, config_mask, &window_config);
}
