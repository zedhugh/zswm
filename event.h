#include <xcb/xcb.h>

#ifndef __ZS_WM_EVENT__
#define __ZS_WM_EVENT__

void event_handle(xcb_generic_event_t *event);

#endif
