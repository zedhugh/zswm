#include <glib.h>
#include <stdint.h>

#ifndef __ZSWM_NET_
#define __ZSWM_NET_

typedef struct {
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    char up[128];
    char down[128];

} NetSpeed;

typedef void (*net_speed_notify)(NetSpeed speed);

void init_net_speed(net_speed_notify cb);
void clean_net_speed(void);

#endif
