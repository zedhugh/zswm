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

typedef void (*mem_usage_notify)(MemUsage usage);

void init_mem_usage(mem_usage_notify cb);
void clean_mem_usage(void);

#endif
