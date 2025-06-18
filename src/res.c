#include <stdbool.h>
#include <stdlib.h>
#include <xcb/xcb_xrm.h>

#include "res.h"

bool get_dpi(xcb_connection_t *conn, double *dpi) {
    xcb_xrm_database_t *db = xcb_xrm_database_from_default(conn);
    if (db == NULL) {
        return false;
    }

    char *value = NULL;
    char *key = "Xft.dpi";
    bool r = xcb_xrm_resource_get_string(db, key, NULL, &value) == 0;

    if (r) {
        *dpi = atof(value);
    }
    xcb_xrm_database_from_default(conn);
    return r;
}
