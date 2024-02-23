#include "types.h"

#ifndef __ZS_WM__WINDOW__
#define __ZS_WM__WINDOW__

void change_window_border_color(xcb_window_t window, uint32_t color_pixel);
xcb_get_window_attributes_reply_t *get_window_attributes(xcb_window_t window);
void manage_window(xcb_window_t window);
#endif
