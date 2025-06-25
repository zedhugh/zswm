#include "types.h"

#ifndef __ZS_WM_USER_ACTION__
#define __ZS_WM_USER_ACTION__

void spawn(const Arg *arg);

void quit(const Arg *arg);

void change_select_tag(const Arg *arg);
void change_layout(const Arg *arg);
void change_current_monitor(const Arg *arg);

void kill_client(const Arg *arg);
void move_client(const Arg *arg);
void resize_client(const Arg *arg);

void toggle_mute(const Arg *arg);
void change_volume(const Arg *arg);

#endif
