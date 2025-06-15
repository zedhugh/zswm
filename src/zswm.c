#include <X11/cursorfont.h>
#include <cairo-xcb.h>
#include <cairo.h>
#include <glib.h>
#include <glibconfig.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_cursor.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xinerama.h>
#include <xcb/xproto.h>

#include "config.h"
#include "draw.h"
#include "event.h"
#include "status.h"
#include "utils.h"
#include "window.h"
#include "xcursor.h"

zswm_global_t global;

static void init_cursors(void);
static void init_bar_window(Monitor *monitor, uint8_t barheight);
static xcb_screen_t *check_other_wm(xcb_connection_t *connection);
static void copy_screen_info(Monitor *m, xcb_xinerama_screen_info_t *s);
static Monitor *monitor_scan(xcb_connection_t *conn);
static void draw_tags(Monitor *monitor, Color scheme[SchemeLast][ColLast]);
static void update_monitor_bar(Monitor *monitor);

xcb_screen_t *check_other_wm(xcb_connection_t *connection) {
    const xcb_setup_t *setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = iter.data;

    uint32_t mask = XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_ENTER_WINDOW |
                    XCB_EVENT_MASK_POINTER_MOTION |
                    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    xcb_void_cookie_t cookie;
    cookie = xcb_change_window_attributes_checked(connection, screen->root,
                                                  XCB_CW_EVENT_MASK, &mask);

    xcb_generic_error_t *error = xcb_request_check(connection, cookie);
    if (error) {
        xcb_disconnect(connection);
        die("another window manager is already running");
    }

    return screen;
}

void copy_screen_info(Monitor *m, xcb_xinerama_screen_info_t *s) {
    m->mx = s->x_org;
    m->my = s->y_org;
    m->mw = s->width;
    m->mh = s->height;
    m->next = NULL;
}

Monitor *monitor_scan(xcb_connection_t *conn) {
    Monitor *monitors, *prev_mon;

    xcb_xinerama_query_screens_cookie_t cookie;
    xcb_xinerama_query_screens_reply_t *reply;
    xcb_xinerama_screen_info_t *screen_info;

    cookie = xcb_xinerama_query_screens(conn);
    reply = xcb_xinerama_query_screens_reply(conn, cookie, NULL);
    screen_info = xcb_xinerama_query_screens_screen_info(reply);

    if (screen_info == NULL) {
        die("no monitor finded");
    }

    int count = xcb_xinerama_query_screens_screen_info_length(reply);
    for (int i = 0; i < count; i++) {
        Monitor *m = ecalloc(1, sizeof(Monitor));
        copy_screen_info(m, &screen_info[i]);
        m->seltags = 1;

        if (!i) {
            monitors = m;
        } else {
            prev_mon->next = m;
        }
        prev_mon = m;
    }

    return monitors;
}

void draw_tags(Monitor *monitor, Color scheme[SchemeLast][ColLast]) {
    int x = 0;
    Color *color;

    for (int i = 0; i < LENGTH(tags); i++) {
        uint16_t sel = (monitor->seltags >> i) & 1;
        color = scheme[sel ? SchemeSel : SchemeNorm];
        const char *tag = tags[i];
        draw_text(monitor->cr, tag, color, x);
        x += get_text_width(tag);
    }
}

void update_monitor_bar(Monitor *monitor) {
    for (Monitor *mon = monitor; mon; mon = mon->next) {
        Color bg = global.color[SchemeNorm][ColBg];
        draw_bg(mon->cr, bg, 0, 0, mon->mw, get_barheight());
        draw_tags(mon, global.color);
        cairo_surface_flush(mon->surface);
    }
    xcb_flush(global.conn);
}

void print_monitor_info(Monitor *m) {
    for (Monitor *curr = m; curr; curr = curr->next) {
        logger("x:\t%d,\ty:\t%d\n", curr->mx, curr->my);
        logger("width:\t%d,\theight:\t%d\n", curr->mw, curr->mh);
    }
}

void grabkeys(void) {
    xcb_connection_t *conn = global.conn;
    xcb_window_t root = global.screen->root;

    xcb_keycode_t key = XCB_GRAB_ANY;
    uint16_t modifiers = XCB_MOD_MASK_ANY;
    xcb_ungrab_key(conn, key, root, modifiers);

    global.keysymbol = xcb_key_symbols_alloc(conn);

    for (int i = 0; i < LENGTH(keys); i++) {
        xcb_key_symbols_t *syms = global.keysymbol;
        xcb_keysym_t keysym = keys[i].keysym;
        xcb_keycode_t *keycodesPtr = xcb_key_symbols_get_keycode(syms, keysym);
        xcb_void_cookie_t cookie = xcb_grab_key(
            conn, XCB_GRAB_ANY, root, keys[i].modifier, *keycodesPtr,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        xcb_generic_error_t *error = xcb_request_check(conn, cookie);
        if ((error)) {
            xcb_disconnect(conn);
            die("cannot grap key");
        }

        xcb_aux_sync(conn);
    }
}

void init_cursors() {
    xcb_connection_t *conn = global.conn;
    xcb_cursor_t *cursors = global.cursors;

    if (xcb_cursor_context_new(conn, global.screen, &global.cursor_ctx) < 0) {
        die("Failed to initialize xcb-cursor");
    }

    uint16_t cursor[] = {XC_left_ptr, XC_bottom_right_corner, XC_fleur};

    for (int i = CurNormal; i < CurLast; i++) {
        xcb_cursor_t temp_cursor = xcursor_new(global.cursor_ctx, cursor[i]);
        cursors[i] = temp_cursor;
    }

    uint32_t mask = XCB_CW_CURSOR;
    xcb_change_window_attributes_value_list_t value = {
        .cursor = cursors[CurNormal],
    };
    xcb_change_window_attributes_aux(conn, global.screen->root, mask, &value);
}

void init_bar_window(Monitor *monitor, uint8_t height) {
    xcb_connection_t *conn = global.conn;
    xcb_screen_t *screen = global.screen;
    xcb_cursor_t *cursors = global.cursors;

    for (Monitor *m = monitor; m; m = m->next) {
        if (m->barwin)
            continue;

        xcb_window_t win = xcb_generate_id(conn);
        xcb_void_cookie_t cookie;
        uint32_t value_mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_BACK_PIXMAP |
                              XCB_CW_EVENT_MASK | XCB_CW_CURSOR;
        xcb_create_window_value_list_t value = {
            .override_redirect = 1,
            .background_pixmap = XCB_BACK_PIXMAP_PARENT_RELATIVE,
            .event_mask = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
            .cursor = cursors[CurNormal],
        };

        uint16_t class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
        cookie = xcb_create_window_aux_checked(
            conn, screen->root_depth, win, screen->root, m->mx, m->my, m->mw,
            height, 0, class, screen->root_visual, value_mask, &value);
        if (xcb_request_check(conn, cookie)) {
            die("create bar window:");
        }

        cookie = xcb_map_window(conn, win);
        if (xcb_request_check(conn, cookie)) {
            die("map bar window:");
        }

        set_window_class_instance(conn, win, "zswm", "zswm");

        m->barwin = win;

        m->surface =
            cairo_xcb_surface_create(conn, win, global.visual, m->mw, height);
        m->cr = cairo_create(m->surface);

        m->wx = m->mx;
        m->wy = m->my + height;
        m->ww = m->mw;
        m->wh = m->mh - height;

        xcb_aux_sync(conn);
    }
}

void scan() {
    xcb_connection_t *conn = global.conn;
    xcb_window_t root = global.screen->root;
    xcb_query_tree_cookie_t cookie = xcb_query_tree(conn, root);
    xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn, cookie, NULL);

    xcb_window_t *windows = xcb_query_tree_children(reply);
    int len = xcb_query_tree_children_length(reply);
    for (int i = 0; i < len; i++) {
        xcb_window_t window = windows[i];
        xcb_get_window_attributes_reply_t *wa = get_window_attributes(window);

        uint8_t override_redirect = wa->override_redirect;
        uint8_t map_state = wa->map_state;

        free(wa);

        if (override_redirect || map_state != XCB_MAP_STATE_VIEWABLE) {
            continue;
        }

        xcb_window_t trans_reply = XCB_NONE;
        xcb_icccm_get_wm_transient_for_reply(
            conn, xcb_icccm_get_wm_transient_for(conn, window), &trans_reply,
            NULL);

        if (trans_reply != XCB_NONE) {
            continue;
        }

        logger("window: %#x\n", window);

        manage_window(window);
    }
}

gboolean handle_xcb_event(GIOChannel *channel, GIOCondition condition,
                          gpointer userdata) {
    xcb_connection_t *conn = (xcb_connection_t *)userdata;
    xcb_generic_event_t *event;

    while ((event = xcb_poll_for_event(conn)) != NULL) {
        if (!global.running) {
            g_main_loop_quit(global.loop);
            return FALSE;
        }

        if (event_handle(event)) {
            update_monitor_bar(global.monitors);
        }
    }

    return TRUE;
}

void init_xcb_event() {
    int xcb_fd = xcb_get_file_descriptor(global.conn);
    GIOChannel *channel = g_io_channel_unix_new(xcb_fd);
    g_io_channel_set_encoding(channel, NULL, NULL);
    guint source_id = g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_ERR,
                                     handle_xcb_event, global.conn);
    if (source_id == 0) {
        die("cannot handle xcb_event");
    }

    global.event_channel = channel;
    global.event_source_id = source_id;
}

void show_pulse(Pulse pulse) {
    logger("device: %s\n", pulse.device);
    logger("avg volume: %d\n", pulse.avg_volume);
    logger("muted: %d\n", pulse.mute);
    for (int i = 0; i < LENGTH(pulse.volumes) && pulse.volumes[i] != -1; ++i) {
        logger("volume[%d]: %d%%\n", i, pulse.volumes[i]);
    }
}

void show_net_speed(NetSpeed speed) {
    logger("=======================================\n");
    logger("down: %s, up: %s\n", speed.down, speed.up);
    logger("rx bytes: %lu, tx bytes: %lu\n", speed.rx_bytes, speed.tx_bytes);
}

int main(int argc, char *argv[]) {
    xcb_connection_t *conn = xcb_connect(NULL, NULL);

    if (!conn || xcb_connection_has_error(conn)) {
        die("connection:");
    }

    xcb_screen_t *screen = check_other_wm(conn);
    xcb_visualtype_t *visual = find_visual(screen, screen->root_visual);

    global.conn = conn;
    global.screen = screen;
    global.visual = visual;
    global.running = true;

    xcb_colormap_t cmap = screen->default_colormap;
    for (int i = 0; i < SchemeLast; i++) {
        for (int j = 0; j < ColLast; j++) {
            global.color[i][j] = create_color(conn, cmap, colors[i][j]);
        }
    }

    cairo_surface_t *surface = cairo_xcb_surface_create(
        conn, screen->root, visual, screen->width_in_pixels,
        screen->height_in_pixels);
    cairo_t *cr = cairo_create(surface);
    Color bg = global.color[SchemeNorm][ColBg];
    draw_bg(cr, bg, 0, 0, screen->width_in_pixels, screen->height_in_pixels);
    cairo_surface_flush(surface);
    cairo_destroy(cr);
    free(surface);

    uint16_t mask = XCB_CW_BACK_PIXEL;
    uint32_t params[] = {bg.xcb_color_pixel};
    xcb_change_window_attributes_checked(conn, screen->root, mask, params);

    Monitor *monitors = monitor_scan(conn);
    print_monitor_info(monitors);

    init_pango_layout(fontfamilies, LENGTH(fontfamilies), fontsize);

    global.monitors = monitors;
    global.current_monitor = monitors;

    init_cursors();
    init_bar_window(monitors, get_barheight());
    update_monitor_bar(monitors);

    scan();

    xcb_flush(conn);

    grabkeys();

    global.loop = g_main_loop_new(NULL, FALSE);
    init_xcb_event();
    init_pulse(g_main_loop_get_context(global.loop), show_pulse);
    init_net_speed(show_net_speed);
    g_main_loop_run(global.loop);

    if (global.restart) {
        execvp(argv[0], argv);
    }
    xcb_disconnect(conn);

    return EXIT_SUCCESS;
}
