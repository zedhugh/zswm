#include <xcb/xcb_cursor.h>

#ifndef __ZS_WM_CURSOR__
#define __ZS_WM_CURSOR__

xcb_cursor_t xcursor_new(xcb_cursor_context_t *ctx, uint16_t cursor_font);

#endif
