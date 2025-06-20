#include <cairo.h>
#include <math.h>
#include <pango/pango-context.h>
#include <pango/pango-font.h>
#include <pango/pango-gravity.h>
#include <pango/pango-layout.h>
#include <pango/pangocairo.h>
#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "utils.h"

#define COLOR_MAX 0xFFFF

typedef struct {
    int width;
    int height;
} Size;

/* static function declarations */
static PangoRectangle get_layout_rect();
static PangoAttribute *create_pango_fg_attr(PangoColor color);
static void set_cairo_color(cairo_t *cr, PangoColor color);

/* static variables */
static PangoLayout *layout;
static PangoAttrList *attrs;
static uint8_t text_height = 0;

/* static function implementations */
PangoRectangle get_layout_rect() {
    PangoRectangle rect;

    PangoRectangle ir, lr;
    pango_layout_get_pixel_extents(layout, &ir, &lr);
    int width = MAX(ir.width, lr.width);
    int height = MIN(ir.height, lr.height);
    int iascent = PANGO_ASCENT(ir);
    int lascent = PANGO_ASCENT(lr);
    int ascent = MIN(iascent, lascent);
    int y = ascent + (text_height - height) / 2;

    rect.x = 0;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    return rect;
}

PangoAttribute *create_pango_fg_attr(PangoColor color) {
    guint16 red = color.red;
    guint16 green = color.green;
    guint16 blue = color.blue;

    return pango_attr_foreground_new(red, green, blue);
}

void set_cairo_color(cairo_t *cr, PangoColor color) {
    double red = (double)color.red / COLOR_MAX;
    double green = (double)color.green / COLOR_MAX;
    double blue = (double)color.blue / COLOR_MAX;
    cairo_set_source_rgb(cr, red, green, blue);
}

/* public function implementations */
uint8_t init_pango_layout(const char *const families, uint8_t size,
                          double dpi) {
    if (text_height) {
        return text_height;
    }

    PangoFontMap *fontmap = pango_cairo_font_map_new();
    PangoContext *context = pango_font_map_create_context(fontmap);
    PangoLanguage *lang = pango_context_get_language(context);
    pango_cairo_context_set_resolution(context, dpi);

    layout = pango_layout_new(context);
    attrs = pango_attr_list_new();

    PangoFontDescription *desc = pango_font_description_from_string(families);
    pango_font_description_set_size(desc, size * PANGO_SCALE);

    PangoAttribute *attr = pango_attr_font_desc_new(desc);
    pango_attr_list_insert(attrs, attr);

    PangoFontMetrics *metrics;
    metrics = pango_context_get_metrics(context, desc, lang);

    int height = PANGO_PIXELS(pango_font_metrics_get_height(metrics));
    int ascent = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics));
    int descent = PANGO_PIXELS(pango_font_metrics_get_descent(metrics));

    int inner_bh = MIN(height, ascent + descent);
    text_height = MAX(inner_bh, text_height);

    free(desc);
    free(metrics);

    pango_layout_set_attributes(layout, attrs);
    return text_height;
}

int get_text_width(const char *text) {
    pango_layout_set_text(layout, text, -1);
    PangoRectangle rect = get_layout_rect();

    return rect.width;
}

void draw_text(cairo_t *cr, const char *text, Color scheme[ColLast], int x,
               int y, int width, int height, bool left_align) {
    pango_layout_set_text(layout, text, -1);
    PangoColor fgcolor = scheme[ColFg].pango_color;
    PangoAttribute *fg = create_pango_fg_attr(fgcolor);
    pango_attr_list_change(attrs, fg);
    PangoRectangle rect = get_layout_rect();

    cairo_move_to(cr, x, y);
    cairo_rectangle(cr, x, 0, width, height);
    set_cairo_color(cr, scheme[ColBg].pango_color);
    cairo_fill(cr);

    double tx = left_align ? 0 : (double)(width - rect.width) / 2;
    double ty = (double)(height - rect.height) / 2;
    pango_cairo_update_layout(cr, layout);
    cairo_move_to(cr, x + tx, y + ty);
    pango_cairo_show_layout(cr, layout);
}

void draw_bg(cairo_t *cr, Color color, int16_t x, int16_t y, uint16_t width,
             uint16_t height) {
    cairo_move_to(cr, x, y);
    cairo_rectangle(cr, x, y, width, height);
    set_cairo_color(cr, color.pango_color);
    cairo_fill(cr);
}

Color create_color(xcb_connection_t *conn, xcb_colormap_t cmap,
                   const char *colorname) {
    Color color;

    if (!pango_color_parse(&color.pango_color, colorname)) {
        die("error: cannot parse color '%s'", colorname);
    }

    xcb_alloc_color_cookie_t cookie;
    xcb_alloc_color_reply_t *reply;

    cookie = xcb_alloc_color(conn, cmap, color.pango_color.red,
                             color.pango_color.green, color.pango_color.blue);
    reply = xcb_alloc_color_reply(conn, cookie, NULL);
    color.xcb_color_pixel = reply->pixel;
    free(reply);

    return color;
}

cairo_surface_t *create_png_surface(const char *png_path, uint16_t width,
                                    uint16_t height) {
    cairo_surface_t *png = cairo_image_surface_create_from_png(png_path);
    if (cairo_surface_status(png) != CAIRO_STATUS_SUCCESS) {
        return NULL;
    }

    int w = cairo_image_surface_get_width(png);
    int h = cairo_image_surface_get_height(png);
    double scale_x = (double)width / w;
    double scale_y = (double)height / h;
    double scale = fmax(scale_x, scale_y);
    double x = (w * scale - width) / 2;
    double y = (h * scale - height) / 2;

    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(surface);
    cairo_save(cr);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, png, x, y);
    cairo_paint(cr);
    cairo_restore(cr);
    cairo_destroy(cr);

    return surface;
}
