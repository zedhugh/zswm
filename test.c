#include "pango/pango-attributes.h"
#include "pango/pango-context.h"
#include "pango/pango-font.h"
#include "pango/pango-fontmap.h"
#include "pango/pango-fontset-simple.h"
#include "pango/pango-fontset.h"
#include "pango/pango-glyph.h"
#include "pango/pango-language.h"
#include "pango/pango-types.h"
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LENGTH(X)               (sizeof X / sizeof X[0])

char *familys[] = { "Terminus", "Emacs Simsun" };
int size = 12;
unsigned long len = LENGTH(familys);

int bar_height = 0;

void add_family_attr_to_list(PangoAttrList *list, char **family_list, int length, int font_size) {
    char family[4096] = { 0 };
    char *seperator = ", ";

    for (int i = 0; i < length; i++) {
        if (i) {
            strcat(family, seperator);
        }
        strcat(family, family_list[i]);
    }

    PangoAttribute *family_attr = pango_attr_family_new(family);
    PangoAttribute *size_attr = pango_attr_size_new(font_size * PANGO_SCALE);

    pango_attr_list_insert(list, family_attr);
    pango_attr_list_insert(list, size_attr);
}

int get_bar_height(PangoContext *context, char **family_list, int length,
                   int font_size) {
    PangoLanguage *lang = pango_context_get_language(context);

    int bh = 0;

    for (int i = 0; i < length; i++) {
        PangoFontDescription *desc = pango_font_description_new();
        pango_font_description_set_family(desc, family_list[i]);
        pango_font_description_set_size(desc, font_size * PANGO_SCALE);

        PangoFontMetrics *metrics =
            pango_context_get_metrics(context, desc, lang);
        int height = PANGO_PIXELS(pango_font_metrics_get_height(metrics));
        int ascent = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics));
        int descent = PANGO_PIXELS(pango_font_metrics_get_descent(metrics));

        int inner_bh = MIN(height, ascent + descent);
        bh = MAX(inner_bh, bh);

        free(desc);
        free(metrics);
    }

    if (bh % 2) {
        bh += 1;
    }

    return bh;
    ;
}

PangoLayout *pango_draw() {
    PangoFontMap *fontmap = pango_cairo_font_map_new();
    PangoContext *context = pango_font_map_create_context(fontmap);
    PangoLayout *layout = pango_layout_new(context);

    pango_context_set_font_map(context, fontmap);

    PangoAttrList *attrs = pango_attr_list_new();
    PangoAttribute *fg = pango_attr_foreground_new(0x0000, 0x0000, 0x0000);
    PangoAttribute *fga = pango_attr_foreground_alpha_new(0xFFFF);
    PangoAttribute *bg = pango_attr_background_new(0xFFFF, 0xFFFF, 0xFFFF);
    PangoAttribute *bga = pango_attr_background_alpha_new(0xFFFF);
    pango_attr_list_insert(attrs, fg);
    /* pango_attr_list_insert(attrs, fga); */
    pango_attr_list_insert(attrs, bg);
    /* pango_attr_list_insert(attrs, bga); */

    add_family_attr_to_list(attrs, familys, len, size);
    pango_layout_set_attributes(layout, attrs);

    bar_height = get_bar_height(context, familys, len, size);
    printf("bar height: %d\n", bar_height);

    char *text = "ABCDEFGUIJKLMNOPQRSTUVWXYZ abcdefguijklmnopqrstuvwxyz 0123456789 test.c - GNU Emacs at desktop Firefox 陈中辉";
    pango_layout_set_text(layout, text, -1);

    PangoRectangle irect;
    PangoRectangle lrect;
    pango_layout_get_extents(layout, &irect, &lrect);
    printf("ink rect x: %d, y: %d, width: %d, height: %d\n", PANGO_PIXELS(irect.x), PANGO_PIXELS(irect.y), PANGO_PIXELS(irect.width), PANGO_PIXELS(irect.height));
    printf("logical rect x: %d, y: %d, width: %d, height: %d\n", PANGO_PIXELS(lrect.x), PANGO_PIXELS(lrect.y), PANGO_PIXELS(lrect.width), PANGO_PIXELS(lrect.height));

    pango_attr_list_unref(attrs);
    g_object_unref(context);

    return layout;
}

int main()
{
    PangoLayout *layout = pango_draw();
    PangoRectangle irect;
    PangoRectangle lrect;
    pango_layout_get_extents(layout, &irect, &lrect);
    int width = PANGO_PIXELS(MAX(irect.width, lrect.width));
    int height = PANGO_PIXELS(MIN(irect.height, lrect.height));
    printf("width: %d, height: %d\n", width, height);

    int hp = 10;
    int vp = 0;
    width += hp;
    height += vp;

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, bar_height);

    // create a cairo context for the surface
    cairo_t *cr = cairo_create(surface);
    pango_cairo_update_layout(cr, layout);

    double y = PANGO_PIXELS(PANGO_ASCENT(irect)) + ((double)vp / 2) + ((double)(bar_height - height) / 2);
    cairo_move_to(cr, ((double)hp / 2), y);


    pango_cairo_show_layout(cr, layout);
    PangoFontMap *fontmap = pango_cairo_font_map_new();
    PangoContext *ctx = pango_context_new();
    pango_context_set_font_map(ctx, fontmap);
    layout = pango_layout_new(ctx);

    cairo_paint_with_alpha(cr, 0);

    g_object_unref(layout);

    // destroy cairo context
    cairo_destroy(cr);

    // write the image to file test.png
    cairo_surface_write_to_png(surface, "test.png");

    // free the surface
    cairo_surface_destroy(surface);

    return 0;
}
