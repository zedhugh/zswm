#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "user-action.h"
#include "utils.h"
#include "window.h"

void spawn(const Arg *arg) {
    char **cmd = (char **)arg->v;
    run_shell_cmd(cmd);
}

void quit(const Arg *arg) {
    global.running = false;
    global.restart = arg->b;
}

void change_select_tag(const Arg *arg) {
    unsigned int target_tag = (1 << ((arg->ui - 1) & TAGMASK));
    if (!target_tag || target_tag == global.current_monitor->seltags) {
        return;
    }
    global.current_monitor->seltags = target_tag;

    for (Client *c = global.current_monitor->clients; c; c = c->next) {
        show_and_hide_client(c);
    }
}

void change_layout(const Arg *arg) {
    bool forward = arg && arg->b;
    uint32_t index = 0;
    for (; index < LENGTH(layouts); index++) {
        const char *ltsymbol = global.current_monitor->layout->symbol;
        if (strcmp(ltsymbol, layouts[index].symbol) == 0) {
            break;
        }
    }
    index = (LENGTH(layouts) + index + (forward ? 1 : -1)) % LENGTH(layouts);
    global.current_monitor->layout = &layouts[index];
    if (global.current_monitor->layout->arrange) {
        global.current_monitor->layout->arrange(global.current_monitor);
    }
}

void change_current_monitor(const Arg *arg);

void kill_client(const Arg *arg);
void move_client(const Arg *arg);
void resize_client(const Arg *arg);

void toggle_mute(const Arg *arg) { toggle_pulse_mute(); }
void change_volume(const Arg *arg) { change_pulse_volume(arg->i); }
