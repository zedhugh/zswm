#include <math.h>
#include <stdint.h>

#ifndef __ZSWM_STATUS_UTILS_
#define __ZSWM_STATUS_UTILS_

#define LENGTH(X) ((ssize_t)sizeof(X) / (ssize_t)sizeof(X[0]))
#define volume_to_percent(v) ((int)round((double)(v) * 100 / PA_VOLUME_NORM))

void bytes_to_readable_size(uint64_t bytes, char *str);

#endif
