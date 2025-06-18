#include <stdbool.h>
#include <xcb/xcb.h>

#ifndef __ZSWM_RES_
#define __ZSWM_RES_

bool get_dpi(xcb_connection_t *conn, double *dpi);

#endif
