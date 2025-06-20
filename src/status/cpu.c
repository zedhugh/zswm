#include <glib.h>
#include <glibconfig.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"

#define CPU_FILE "/proc/stat"

/*****************************************************************************/
/*                        file private data structure                        */
/*****************************************************************************/
typedef struct {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
    uint64_t guest;
    uint64_t guest_nice;
} CPUStat;

/*****************************************************************************/
/*                        file static global variables                       */
/*****************************************************************************/
static CPUStat prev_cpu_stat = {0};
static gboolean inited = FALSE;

/*****************************************************************************/
/*                        private function declaration                       */
/*****************************************************************************/
double calc_cpu_usage(CPUStat curr, CPUStat prev);

/*****************************************************************************/
/*                                 interface                                 */
/*****************************************************************************/
bool get_cpu_usage(double *usage, GError *error) {
    gchar *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(CPU_FILE, &contents, &length, &error)) {
        return FALSE;
    }

    gchar **lines = g_strsplit(contents, "\n", 0);
    g_free(contents);

    char cpu_line[256];
    for (gchar **line = lines; *line != NULL; ++line) {
        if (strncmp(*line, "cpu ", 4) == 0) {
            strncpy(cpu_line, *line, sizeof(cpu_line));
            break;
        }
    }

    CPUStat stat = {0};
    sscanf(cpu_line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &stat.user,
           &stat.nice, &stat.system, &stat.idle, &stat.iowait, &stat.irq,
           &stat.softirq, &stat.steal, &stat.guest, &stat.guest_nice);

    if (!inited) {
        prev_cpu_stat = stat;
        inited = TRUE;
    }

    double percent = calc_cpu_usage(stat, prev_cpu_stat);
    prev_cpu_stat = stat;
    *usage = percent;

    g_strfreev(lines);

    return TRUE;
}

/*****************************************************************************/
/*                        private function definition                        */
/*****************************************************************************/
double calc_cpu_usage(CPUStat curr, CPUStat prev) {
    uint64_t curr_idle = curr.idle + curr.iowait;
    uint64_t curr_total = curr.user + curr.nice + curr.system + curr.idle +
                          curr.iowait + curr.irq + curr.softirq + curr.steal +
                          curr.guest + curr.guest_nice;

    uint64_t prev_idle = prev.idle + prev.iowait;
    uint64_t prev_total = prev.user + prev.nice + prev.system + prev.idle +
                          prev.iowait + prev.irq + prev.softirq + prev.steal +
                          prev.guest + prev.guest_nice;

    uint64_t total = curr_total - prev_total;
    uint64_t idle = curr_idle - prev_idle;
    uint64_t active = total - idle;

    if (total == 0) {
        return 0.0;
    }

    double usage = ((double)active) / total * 100;
    return usage;
}
