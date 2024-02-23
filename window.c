#include "global.h"
#include "window.h"

void change_window_border_color(xcb_window_t window, uint32_t color_pixel) {
    xcb_connection_t *c = global.conn;
    uint32_t mask = XCB_CW_BORDER_PIXEL;
    xcb_params_cw_t params = {.border_pixel = color_pixel};
    xcb_aux_change_window_attributes(c, window, mask, &params);

    xcb_flush(c);
}
