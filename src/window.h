#include "types.h"

#ifndef __ZS_WM__WINDOW__
#define __ZS_WM__WINDOW__

void set_window_class_instance(xcb_connection_t *conn, xcb_window_t window,
                               const char *class, const char *instance);
Client *get_client_of_window(xcb_window_t window);
void change_window_rect(xcb_window_t window, int32_t x, int32_t y,
                        uint32_t width, uint32_t height, uint32_t border_width);
void change_window_border_color(xcb_window_t window, uint32_t color_pixel);
xcb_get_window_attributes_reply_t get_window_attributes(xcb_window_t window);
void manage_window(xcb_window_t window);
void unmanage_client(Client *client, bool destroyed);
void show_and_hide_client(Client *client);

#endif
