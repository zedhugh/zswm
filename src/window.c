#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "config.h"
#include "global.h"
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

void configure_client(Client *client) {
    xcb_connection_t *conn = global.conn;
    xcb_cw_t change_mask = XCB_CW_EVENT_MASK | XCB_CW_BORDER_PIXEL;
    xcb_params_cw_t params = {
        .event_mask =
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY,
        .border_pixel = global.color[SchemeNorm][ColBorder].xcb_color_pixel,
    };

    xcb_aux_change_window_attributes(conn, client->win, change_mask, &params);

    xcb_config_window_t config_mask =
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH;
    xcb_params_configure_window_t window_config = {
        .x = client->x,
        .y = client->y,
        .width = client->width,
        .height = client->height,
        .border_width = border_px,
    };
    xcb_aux_configure_window(conn, client->win, config_mask, &window_config);
    xcb_flush(conn);
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

void manage_window(xcb_window_t window) {
    Client *c = ecalloc(1, sizeof(Client));
    c->win = window;

    xcb_get_geometry_reply_t geometry = get_window_geometry(window);
    c->x = c->old_x = geometry.x;
    c->y = c->old_y = geometry.y;
    c->width = c->old_width = geometry.width;
    c->height = c->old_height = geometry.height;
    c->bw = c->old_bw = geometry.border_width;

    xcb_window_t trans_for = get_transient_window_for(window);
    Client *t = NULL;
    if (trans_for != XCB_NONE && (t = get_client_of_window(trans_for))) {
        c->mon = t->mon;
        c->tags = t->tags;
    } else {
        c->mon = global.current_monitor;
        c->tags = global.current_monitor->seltags;
    }

    c->x = c->mon->wx;
    c->y = c->mon->wy;
    c->width = c->mon->ww;
    c->height = c->mon->wh;
    c->bw = border_px;

    update_client_name(c);
    configure_client(c);
    attach_client(c);

    show_clients(global.monitors);

    xcb_connection_t *conn = global.conn;

    xcb_map_window(conn, window);
    xcb_map_subwindows(conn, window);

    xcb_flush(conn);
}

void unmanage_client(Client *client, bool destroyed) {
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
