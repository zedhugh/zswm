#include <glib.h>
#include <glibconfig.h>
#include <time.h>

#include "time.h"
#include "utils.h"

#define INTERVAL 1

static status_time_notify status_time_listener = NULL;
static guint timer = 0;

gboolean update_status_time(gpointer data);

void init_status_time(status_time_notify cb) {
    status_time_listener = cb;
    update_status_time(NULL);
    timer = g_timeout_add_seconds(INTERVAL, update_status_time, NULL);
}

void clean_status_time(void) {
    g_source_remove(timer);
    status_time_listener = NULL;
}

gboolean update_status_time(gpointer data) {
    time_t current_time = time(NULL);
    struct tm *time_info = localtime(&current_time);

    StatusTime t;
    strftime(t.full, sizeof(t.full), "%Y-%m-%d %H:%M:%S", time_info);
    strftime(t.date, sizeof(t.date), "%Y-%m-%d", time_info);
    strftime(t.time, sizeof(t.time), "%H:%M:%S", time_info);

    if (status_time_listener != NULL) {
        status_time_listener(t);
    }

    return TRUE;
}
