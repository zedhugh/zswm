#include <pango/pangocairo.h>

#include "types.h"
#include "utils.h"

#define COLOR_MAX 0xFFFF

/* static function declarations */
static PangoRectangle get_layout_rect();
static PangoAttribute *create_pango_fg_attr(PangoColor color);
static void set_cairo_color(cairo_t *cr, PangoColor color);

/* static variables */
static PangoLayout *layout;
static PangoAttrList *attrs;
static uint8_t barheight = 0;
static uint8_t lrpad;

/* static function implementations */
PangoRectangle get_layout_rect() {
    PangoRectangle rect;

    PangoRectangle ir, lr;
    pango_layout_get_extents(layout, &ir, &lr);
    int width = MAX(PANGO_PIXELS(ir.width), PANGO_PIXELS(lr.width));
    int height = MIN(PANGO_PIXELS(ir.height), PANGO_PIXELS(lr.height));
    int iascent = PANGO_PIXELS(PANGO_ASCENT(ir));
    int lascent = PANGO_PIXELS(PANGO_ASCENT(lr));
    int ascent = MIN(iascent, lascent);
    int y = ascent + (barheight - height) / 2;

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
void init_pango_layout(char **families, size_t length, uint8_t size) {
    if (barheight)
        return;

    PangoFontMap *fontmap = pango_cairo_font_map_new();
    PangoContext *context = pango_font_map_create_context(fontmap);
    PangoLanguage *lang = pango_context_get_language(context);

    layout = pango_layout_new(context);
    attrs = pango_attr_list_new();

    for (int i = 0; i < length; i++) {
        PangoFontDescription *desc = pango_font_description_new();
        pango_font_description_set_family(desc, families[i]);
        pango_font_description_set_size(desc, size * PANGO_SCALE);

        PangoAttribute *attr = pango_attr_font_desc_new(desc);
        pango_attr_list_insert(attrs, attr);

        PangoFontMetrics *metrics;
        metrics = pango_context_get_metrics(context, desc, lang);

        int height = PANGO_PIXELS(pango_font_metrics_get_height(metrics));
        int ascent = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics));
        int descent = PANGO_PIXELS(pango_font_metrics_get_descent(metrics));

        int inner_bh = MIN(height, ascent + descent);
        barheight = MAX(inner_bh, barheight);

        free(desc);
        free(metrics);
    }

    pango_layout_set_attributes(layout, attrs);

    lrpad = barheight / 2 - 1;
    if (barheight % 2) {
        barheight += 1;
    }
}

uint8_t get_barheight() { return barheight; }

int get_text_width(const char *text) {
    pango_layout_set_text(layout, text, -1);
    PangoRectangle rect = get_layout_rect();

    return rect.width + 2 * lrpad;
}

void draw_text(cairo_t *cr, const char *text, Color scheme[ColLast], int x) {
    pango_layout_set_text(layout, text, -1);
    PangoColor fgcolor = scheme[ColFg].pango_color;
    PangoAttribute *fg = create_pango_fg_attr(fgcolor);
    pango_attr_list_change(attrs, fg);
    PangoRectangle rect = get_layout_rect();

    cairo_move_to(cr, x, 0);
    cairo_rectangle(cr, x, 0, rect.width + lrpad * 2, barheight);
    set_cairo_color(cr, scheme[ColBg].pango_color);
    cairo_fill(cr);

    pango_cairo_update_layout(cr, layout);
    cairo_move_to(cr, x + lrpad, rect.y);
    pango_cairo_show_layout(cr, layout);
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
