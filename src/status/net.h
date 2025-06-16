#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef __ZSWM_NET_
#define __ZSWM_NET_

typedef struct {
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    char up[128];
    char down[128];

} NetSpeed;

bool get_net_speed(uint32_t interval, NetSpeed *speed, GError *error);

#endif
