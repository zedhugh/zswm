#include <glib.h>
#include <glibconfig.h>
#include <stdbool.h>
#include <stddef.h>

#include "status.h"

#define INTERVAL 1
#define TIME_FORMAT "%A %m-%d %H:%M:%S"

/*****************************************************************************/
/*                           file static variables                           */
/*****************************************************************************/
static Status status;
static guint timer = 0;
static status_changed_notify listener = NULL;

/*****************************************************************************/
/*                              private function                             */
/*****************************************************************************/
void notify_status_change() {
    if (listener != NULL) {
        listener(status);
    }
}
void pulse_cb(Pulse p) {
    status.pulse = p;
    notify_status_change();
}

void get_date_time(char *time_text, size_t length) {
    time_t current_time = time(NULL);
    struct tm *time_info = localtime(&current_time);

    strftime(time_text, length, TIME_FORMAT, time_info);
}

gboolean update_status(gpointer data) {
    GError *error = NULL;
    get_net_speed(INTERVAL, &status.net_speed, error);
    get_mem_usage(&status.mem_usage, error);
    get_cpu_usage(&status.cpu_usage_percent, error);
    get_date_time(status.time, sizeof(status.time));
    notify_status_change();

    return TRUE;
}

/*****************************************************************************/
/*                                 interface                                 */
/*****************************************************************************/
void init_status(GMainContext *context, status_changed_notify callback) {
    listener = callback;
    init_pulse(context, pulse_cb);
    update_status(NULL);
    timer = g_timeout_add_seconds(INTERVAL, update_status, NULL);
}

void clean_status() {
    clean_pulse();
    g_source_remove(timer);
    listener = NULL;
}
