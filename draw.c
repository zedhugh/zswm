#include <stdio.h>

#include "config.h"
#include "draw.h"
#include "global.h"
#include "pango/pango-layout.h"
#include "pango/pangocairo.h"
#include "utils.h"

static PangoRectangle get_layout_rect(PangoLayout *layout, int barheight) {
    PangoRectangle rect;

    PangoRectangle ir, lr;
    pango_layout_get_extents(layout, &ir, &lr);
    int layout_width = MAX(PANGO_PIXELS(ir.width), PANGO_PIXELS(lr.width));
    int layout_height = MIN(PANGO_PIXELS(ir.height), PANGO_PIXELS(lr.height));
    int ascent = MIN(PANGO_PIXELS(PANGO_ASCENT(ir)), PANGO_PIXELS(PANGO_ASCENT(lr)));
    int y = ascent + (barheight - layout_height) / 2;
    printf("y: %d, h: %u,lh: %u\n", y,  barheight, layout_height);

    rect.x = 0;
    rect.y = y;
    rect.width = layout_width;
    rect.height = layout_height;

    return rect;
}

void draw_tags(Monitor *m, PangoLayout *layout, int barheight) {
    int x = 0;
    PangoRectangle rect;
    for (int i = 0; i < LENGTH(tags); i++) {
        const char *tag = tags[i];
        pango_layout_set_text(layout, tag, -1);
        rect = get_layout_rect(layout, barheight);

        x += rect.x;
        pango_cairo_update_layout(m->cr, layout);
        cairo_move_to(m->cr, x, rect.y);
        pango_cairo_show_layout(m->cr, layout);
        x += rect.width;
    }
}

void update_monitor_bar() {
    Monitor *mon;
    PangoLayout *layout = global.layout;
    for (mon = global.monitor; mon; mon = mon->next) {
        draw_tags(mon, global.layout, global.barheight);
    }
}
