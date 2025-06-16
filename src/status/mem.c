#include <glib.h>
#include <glibconfig.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "mem.h"
#include "utils.h"

#define MEM_FILE "/proc/meminfo"

/*****************************************************************************/
/*                        private function declaration                       */
/*****************************************************************************/
MemUsage calc_mem_usage(MemUsage usage);

/*****************************************************************************/
/*                                 interface                                 */
/*****************************************************************************/
bool get_mem_usage(MemUsage *u, GError *error) {
    gchar *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(MEM_FILE, &contents, &length, &error)) {
        return false;
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
    *u = calc_mem_usage(usage);

    g_strfreev(lines);

    return true;
}

/*****************************************************************************/
/*                        private function definition                        */
/*****************************************************************************/
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
