#include <glib.h>
#include <glibconfig.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "net.h"
#include "utils.h"

#define NET_FILE "/proc/net/dev"

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
static NetStats prev = {0};
static bool inited = false;

/*****************************************************************************/
/*                        private function declaration                       */
/*****************************************************************************/
NetSpeed calc_speed(uint32_t interval, NetStats current, NetStats previous);

/*****************************************************************************/
/*                                 interface                                 */
/*****************************************************************************/
bool get_net_speed(uint32_t interval, NetSpeed *speed, GError *error) {
    gchar *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(NET_FILE, &contents, &length, &error)) {
        return false;
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

    if (!inited) {
        inited = true;
        prev = stat;
    }
    *speed = calc_speed(interval, stat, prev);
    prev = stat;

    g_strfreev(lines);

    return false;
}

/*****************************************************************************/
/*                        private function definition                        */
/*****************************************************************************/
NetSpeed calc_speed(uint32_t interval, NetStats current, NetStats previous) {
    NetSpeed speed = {.down = "0", .up = "0", .rx_bytes = 0, .tx_bytes = 0};
    speed.rx_bytes = current.rx_bytes - previous.rx_bytes;
    speed.tx_bytes = current.tx_bytes - previous.tx_bytes;
    bytes_to_readable_size(speed.rx_bytes / interval, speed.down);
    bytes_to_readable_size(speed.tx_bytes / interval, speed.up);
    return speed;
}
