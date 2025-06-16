#include <glib.h>

#include "cpu.h"
#include "mem.h"
#include "net.h"
#include "pulse.h"

#ifndef __ZSWM_STATUS_
#define __ZSWM_STATUS_

typedef struct {
    Pulse pulse;
    NetSpeed net_speed;
    MemUsage mem_usage;
    double cpu_usage_percent;
    char time[256];
} Status;

typedef void (*status_changed_notify)(Status status);

void init_status(GMainContext *context, status_changed_notify callback);
void clean_status(void);

#endif
