#include <glib.h>
#include <pulse/channelmap.h>
#include <stdint.h>

#ifndef __ZSWM_PULSE_
#define __ZSWM_PULSE_

typedef struct {
    uint32_t index;
    int mute;
    int avg_volume;
    int volumes[PA_CHANNEL_MAP_DEF_MAX];
    char device[512];
} Pulse;

typedef void (*pulse_notify)(Pulse pulse);
void init_pulse(GMainContext *g_context, pulse_notify cb);
void clean_pulse(void);
void change_pulse_volume(int step);
void toggle_pulse_mute(void);

#endif
