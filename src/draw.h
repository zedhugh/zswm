#include <cairo.h>
#include <stdbool.h>

#include "types.h"

#ifndef __ZS_WM_DRAW__
#define __ZS_WM_DRAW__

void init_pango_layout(const char *const *families, size_t len, uint8_t size);
uint8_t get_barheight();
int get_text_width(const char *text);
void draw_text(cairo_t *cr, const char *text, Color scheme[ColLast], int x);
void draw_bg(cairo_t *cr, Color color, int16_t x, int16_t y, uint16_t width,
             uint16_t height);
Color create_color(xcb_connection_t *conn, xcb_colormap_t cmap,
                   const char *colorname);
cairo_surface_t *create_png_surface(const char *png_path, uint16_t width,
                                    uint16_t height);

#endif
