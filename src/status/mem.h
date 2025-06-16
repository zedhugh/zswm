#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef __ZSWM_MEM_
#define __ZSWM_MEM_

typedef struct {
    char mem_used_text[32];
    char swap_used_text[32];

    float mem_percent;
    float swap_percent;
    uint64_t mem_used;
    uint64_t swap_used;

    uint64_t mem_total;
    uint64_t mem_free;
    uint64_t buffers;
    uint64_t cached;
    uint64_t swap_total;
    uint64_t swap_free;
    uint64_t s_reclaimable;

} MemUsage;

bool get_mem_usage(MemUsage *usage, GError *error);

#endif
