#include "config.h"
#include "pango/pangocairo.h"
#include "types.h"
#include <stdint.h>

#define COLOR 0xFFFF

static PangoRectangle get_layout_rect(PangoLayout *layout, int barheight) {
    PangoRectangle rect;

    PangoRectangle ir, lr;
    pango_layout_get_extents(layout, &ir, &lr);
    int layout_width = MAX(PANGO_PIXELS(ir.width), PANGO_PIXELS(lr.width));
    int layout_height = MIN(PANGO_PIXELS(ir.height), PANGO_PIXELS(lr.height));
    int ascent = MIN(PANGO_PIXELS(PANGO_ASCENT(ir)), PANGO_PIXELS(PANGO_ASCENT(lr)));
    int y = ascent + (barheight - layout_height) / 2;

    rect.x = 0;
    rect.y = y;
    rect.width = layout_width;
    rect.height = layout_height;

    return rect;
}

static PangoAttribute *create_fg(PangoColor color) {
    guint16 red = color.red;
    guint16 green = color.green;
    guint16 blue = color.blue;

    return pango_attr_foreground_new(red, green, blue);
}

void draw_tags(Monitor *m, PangoLayout *layout, Color **colors, int barheight, uint8_t lrpad) {
    int x = 0;
    PangoRectangle rect;
    Color *color;

    for (int i = 0; i < LENGTH(tags); i++) {
        color = colors[!(i % 2) ? SchemeSel : SchemeNorm];

        const char *tag = tags[i];
        pango_layout_set_text(layout, tag, -1);
        rect = get_layout_rect(layout, barheight);

        PangoColor fgcolor = color[ColFg].pango_color;
        PangoAttribute *fg = create_fg(fgcolor);
        PangoAttrList *attrs = pango_layout_get_attributes(layout);
        pango_attr_list_change(attrs, fg);

        PangoColor bgcolor = color[ColBg].pango_color;
        double red = (double)bgcolor.red / COLOR;
        double green = (double)bgcolor.green / COLOR;
        double blue = (double)bgcolor.blue / COLOR;
        cairo_move_to(m->cr, x, 0);
        int width = rect.width + 2 * lrpad;
        cairo_rectangle(m->cr, x, 0, width, barheight);
        cairo_set_source_rgb(m->cr, red, green, blue);
        cairo_fill(m->cr);


        x += rect.x + lrpad;
        pango_cairo_update_layout(m->cr, layout);
        cairo_move_to(m->cr, x, rect.y);
        pango_cairo_show_layout(m->cr, layout);
        x += rect.width + lrpad;
    }
}

void update_monitor_bar(Monitor *monitor, PangoLayout *layout, uint8_t height, uint8_t lrpad) {
    for (Monitor *mon = monitor; mon; mon = mon->next) {
        draw_tags(mon, layout, global.color, height, lrpad);
    }
}

Color create_color(const char *colorname) {
    Color color;

    color.xcb_color_pixel = alloc_color(colorname);
    if (!pango_color_parse(&color.pango_color, colorname)) {
        die("error: cannot parse color '%s'", colorname);
    }

    return color;
}

Color *create_scheme(const char *colornames[], size_t length) {
    if (!colornames || length < 2) {
        return NULL;
    }

    Color *colors;
    if (!(colors = ecalloc(length, sizeof(Color)))) {
        return NULL;
    }

    for (size_t i = 0; i < length; i++) {
        colors[i] = create_color(colornames[i]);
    }

    return colors;
}
