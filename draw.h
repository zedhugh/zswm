#include <stddef.h>

#include "global.h"

#ifndef __ZS_WM_DRAW__
#define __ZS_WM_DRAW__

void update_monitor_bar(Monitor *monitor, PangoLayout *layout, uint8_t height, uint8_t lrpad);
Color create_color(const char *colorname);
Color *create_scheme(const char *colornames[], size_t length);

#endif
