#include <glib.h>
#include <glibconfig.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "net.h"
#include "utils.h"

#define NET_FILE "/proc/net/dev"
#define INTERVAL 1

/*****************************************************************************/
/*                        file private data structure                        */
/*****************************************************************************/
typedef struct {
    uint64_t rx_bytes;
    uint64_t tx_bytes;
} NetStats;

/*****************************************************************************/
/*                        file static global variables                       */
/*****************************************************************************/
static net_speed_notify net_speed_listener = NULL;
static NetStats prev = {0};
static NetStats curr = {0};
static gboolean inited = FALSE;
static gboolean will_stop = FALSE;

/*****************************************************************************/
/*                        private function declaration                       */
/*****************************************************************************/
gboolean update_speed(gpointer data);
NetSpeed calc_speed(NetStats current, NetStats previous);

/*****************************************************************************/
/*                                 interface                                 */
/*****************************************************************************/
void init_net_speed(net_speed_notify cb) {
    inited = FALSE;
    will_stop = FALSE;
    net_speed_listener = cb;
    update_speed(NULL);
    g_timeout_add_seconds(INTERVAL, update_speed, NULL);
}

void clean_net_speed(void) {
    inited = FALSE;
    will_stop = TRUE;
    net_speed_listener = NULL;
}

/*****************************************************************************/
/*                        private function definition                        */
/*****************************************************************************/
gboolean update_speed(gpointer data) {
    if (will_stop) {
        return FALSE;
    }

    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;

    if (!g_file_get_contents(NET_FILE, &contents, &length, &error)) {
        g_printerr("Error reading %s: %s\n", NET_FILE, error->message);
        return FALSE;
    }

    gchar **lines = g_strsplit(contents, "\n", 0);
    g_free(contents);

    NetStats stat = {.rx_bytes = 0, .tx_bytes = 0};
    for (gchar **line = lines; *line != NULL; ++line) {
        if (strchr(*line, ':') == NULL) {
            continue;
        }

        char name[16];
        uint64_t rx_bytes, tx_bytes;
        sscanf(*line, " %[^:]: %lu %*u %*u %*u %*u %*u %*u %*u %lu", name,
               &rx_bytes, &tx_bytes);
        if (g_str_equal(name, "lo")) {
            continue;
        }

        stat.rx_bytes += rx_bytes;
        stat.tx_bytes += tx_bytes;
    }

    if (inited) {
        prev = curr;
    } else {
        inited = TRUE;
        prev = stat;
    }
    curr = stat;
    NetSpeed speed = calc_speed(curr, prev);

    if (net_speed_listener != NULL) {
        net_speed_listener(speed);
    }

    g_strfreev(lines);

    return TRUE;
}

NetSpeed calc_speed(NetStats current, NetStats previous) {
    NetSpeed speed = {.down = "0", .up = "0", .rx_bytes = 0, .tx_bytes = 0};
    speed.rx_bytes = current.rx_bytes - previous.rx_bytes;
    speed.tx_bytes = current.tx_bytes - previous.tx_bytes;
    bytes_to_readable_size(speed.rx_bytes / INTERVAL, speed.down);
    bytes_to_readable_size(speed.tx_bytes / INTERVAL, speed.up);
    return speed;
}
