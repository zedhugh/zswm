#include <glib.h>
#include <glibconfig.h>
#include <stdint.h>
#include <stdio.h>

#include "mem.h"
#include "utils.h"

#define MEM_FILE "/proc/meminfo"
#define INTERVAL 1

/*****************************************************************************/
/*                        file static global variables                       */
/*****************************************************************************/
static mem_usage_notify mem_usage_listener = NULL;
static gboolean will_stop = FALSE;

/*****************************************************************************/
/*                        private function declaration                       */
/*****************************************************************************/
gboolean update_mem_usage(gpointer data);
MemUsage calc_mem_usage(MemUsage usage);

/*****************************************************************************/
/*                                 interface                                 */
/*****************************************************************************/
void init_mem_usage(mem_usage_notify cb) {
    will_stop = FALSE;
    mem_usage_listener = cb;
    update_mem_usage(NULL);
    g_timeout_add_seconds(INTERVAL, update_mem_usage, NULL);
}

void clean_mem_usage(void) {
    will_stop = TRUE;
    mem_usage_listener = NULL;
}

/*****************************************************************************/
/*                        private function definition                        */
/*****************************************************************************/
gboolean update_mem_usage(gpointer data) {
    if (will_stop) {
        return FALSE;
    }

    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;

    if (!g_file_get_contents(MEM_FILE, &contents, &length, &error)) {
        g_printerr("Error reading %s: %s\n", MEM_FILE, error->message);
        return FALSE;
    }

    gchar **lines = g_strsplit(contents, "\n", 0);
    g_free(contents);

    MemUsage usage = {0};

    for (gchar **line = lines; *line != NULL; ++line) {
        char name[32];
        uint64_t kb = 0;
        sscanf(*line, "%[^:]: %lu", name, &kb);
        if (g_str_equal(name, "MemTotal")) {
            usage.mem_total = kb;
        } else if (g_str_equal(name, "MemFree")) {
            usage.mem_free = kb;
        } else if (g_str_equal(name, "Buffers")) {
            usage.buffers = kb;
        } else if (g_str_equal(name, "Cached")) {
            usage.cached = kb;
        } else if (g_str_equal(name, "SwapTotal")) {
            usage.swap_total = kb;
        } else if (g_str_equal(name, "SwapFree")) {
            usage.swap_free = kb;
        } else if (g_str_equal(name, "SReclaimable")) {
            usage.s_reclaimable = kb;
        }
    }
    usage = calc_mem_usage(usage);

    if (mem_usage_listener != NULL) {
        mem_usage_listener(usage);
    }

    g_strfreev(lines);

    return TRUE;
}

MemUsage calc_mem_usage(MemUsage usage) {
    MemUsage u = usage;
    u.mem_used =
        u.mem_total - u.mem_free - u.buffers - u.cached - u.s_reclaimable;
    u.swap_used = u.swap_total - u.swap_free;

    u.mem_percent = ((float)u.mem_used) / ((float)u.mem_total) * 100;
    if (u.swap_total == 0) {
        u.swap_percent = 0;
    }

    bytes_to_readable_size(u.mem_used * 1024, u.mem_used_text);
    bytes_to_readable_size(u.swap_used * 1024, u.swap_used_text);

    return u;
}
