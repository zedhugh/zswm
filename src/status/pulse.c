#include <glib.h>
#include <pulse/channelmap.h>
#include <pulse/context.h>
#include <pulse/def.h>
#include <pulse/glib-mainloop.h>
#include <pulse/introspect.h>
#include <pulse/proplist.h>
#include <pulse/subscribe.h>
#include <pulse/volume.h>
#include <stdint.h>
#include <string.h>

#include "pulse.h"
#include "utils.h"

/*****************************************************************************/
/*                        file static global variables                       */
/*****************************************************************************/
static Pulse pulse;
static pa_cvolume current_volume;
static pa_context *context = NULL;
static pulse_notify pulse_callback = NULL;

/*****************************************************************************/
/*                        private function declaration                       */
/*****************************************************************************/
void state_callback(pa_context *c, void *userdata);
void server_info_callback(pa_context *c, const pa_server_info *i,
                          void *userdata);
void sink_info_callback(pa_context *c, const pa_sink_info *i, int eol,
                        void *userdata);
void subscribe_callback(pa_context *c, pa_subscription_event_type_t t,
                        uint32_t idx, void *userdata);
Pulse parse_sink(pa_sink_info i);

/*****************************************************************************/
/*                                 Interface                                 */
/*****************************************************************************/
void init_pulse(GMainContext *g_context, pulse_notify cb) {
    pulse_callback = cb;
    pa_glib_mainloop *loop = pa_glib_mainloop_new(g_context);
    pa_mainloop_api *api = pa_glib_mainloop_get_api(loop);
    pa_context *ctx = pa_context_new(api, "ZSWM-Status-Volume");
    pa_context_connect(ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
    pa_context_set_state_callback(ctx, state_callback, NULL);
}

void clean_pulse() {
    pa_context_disconnect(context);
    pa_context_unref(context);
    context = NULL;
    pulse_callback = NULL;
}

void change_pulse_volume(int step) {
    if (!step || step > 100 || step < -100) {
        return;
    }

    pa_cvolume vol = current_volume;
    if (step > 0) {
        pa_cvolume_inc(&vol, PA_VOLUME_NORM * step / 100);
        if (pa_cvolume_max(&vol) > PA_VOLUME_NORM) {
            pa_cvolume_set(&vol, vol.channels, PA_VOLUME_NORM);
        }
    } else {
        pa_cvolume_dec(&vol, PA_VOLUME_NORM * (-step) / 100);
        if (pa_cvolume_min(&vol) < PA_VOLUME_MUTED) {
            pa_cvolume_set(&vol, vol.channels, PA_VOLUME_MUTED);
        }
    }
    pa_context_set_sink_volume_by_index(context, pulse.index, &vol, NULL, NULL);
}

void toggle_pulse_mute() {
    uint32_t index = pulse.index;
    int mute = !pulse.mute;
    pa_context_set_sink_mute_by_index(context, index, mute, NULL, NULL);
}

/*****************************************************************************/
/*                        private function definition                        */
/*****************************************************************************/
void state_callback(pa_context *c, void *userdata) {
    pa_context_state_t state = pa_context_get_state(c);
    if (PA_CONTEXT_IS_GOOD(state)) {
        pa_context_set_subscribe_callback(c, subscribe_callback, NULL);
        pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL);
        pa_context_get_server_info(c, server_info_callback, userdata);
    }
}

void server_info_callback(pa_context *c, const pa_server_info *i,
                          void *userdata) {
    pa_context_get_sink_info_by_name(c, i->default_sink_name,
                                     sink_info_callback, NULL);
}

void sink_info_callback(pa_context *c, const pa_sink_info *i, int eol,
                        void *userdata) {
    if (eol < 0) {
        return;
    }
    if (eol > 0) {
        return;
    }

    if (i) {
        context = c;
        pulse = parse_sink(*i);
        current_volume = i->volume;

        if (pulse_callback != NULL) {
            pulse_callback(pulse);
        }
    }
}

void subscribe_callback(pa_context *c, pa_subscription_event_type_t t,
                        uint32_t idx, void *userdata) {
    pa_subscription_event_type_t event = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
    uint32_t facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

    if (facility != PA_SUBSCRIPTION_EVENT_SINK) {
        return;
    }

    if (event == PA_SUBSCRIPTION_EVENT_CHANGE) {
        pa_context_get_server_info(c, server_info_callback, NULL);
    }
}

Pulse parse_sink(pa_sink_info i) {
    Pulse p = {
        .index = i.index,
        .mute = i.mute,
        .volumes = {-1},
        .avg_volume = volume_to_percent(pa_cvolume_avg(&i.volume)),
    };
    strncpy(p.device, i.description, LENGTH(p.device));

    int len = 0;
    for (uint8_t ch = 0; ch < LENGTH(p.volumes); ++ch) {
        if (!pa_channel_map_has_position(&i.channel_map, ch)) {
            continue;
        }
        pa_volume_t v = pa_cvolume_get_position(&i.volume, &i.channel_map, ch);
        int volume = volume_to_percent(v);
        p.volumes[len++] = volume;
    }
    for (uint8_t i = len; i < LENGTH(p.volumes); ++i) {
        p.volumes[i] = -1;
    }

    const char *key = NULL;
    void *state = NULL;
    while ((key = pa_proplist_iterate(i.proplist, &state)) != NULL) {
        if (strcmp(key, "api.alsa.path") != 0 &&
            strcmp(key, "device.description") != 0) {
            continue;
        }

        const char *value = pa_proplist_gets(i.proplist, key);
        unsigned long len = strlen(value);

        if (len && (len < strlen(p.device))) {
            strncpy(p.device, value, LENGTH(p.device));
        }
    }

    return p;
}
