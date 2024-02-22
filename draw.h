#include "types.h"

#ifndef __ZS_WM_DRAW__
#define __ZS_WM_DRAW__

void init_pango_layout(char **families, size_t length, uint8_t size);
uint8_t get_barheight();
int get_text_width(const char *text);
void draw_text(cairo_t *cr, const char *text, Color scheme[ColLast], int x);
Color create_color(xcb_connection_t *conn, xcb_colormap_t cmap,
                   const char *colorname);

#endif
