#include <glib.h>
#include <stdbool.h>

#ifndef __ZSWM_CPU_
#define __ZSWM_CPU_

/**
 * @brief get cpu load of percent
 *
 * @param percent percent of usage, between 0 to 100
 */
bool get_cpu_usage(double *percent, GError *error);

#endif
