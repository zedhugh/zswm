#include <X11/keysym.h>
#include <xcb/xproto.h>

#include "global.h"
#include "status.h"
#include "utils.h"

#ifndef __ZS_WM_CONFIG__
#define __ZS_WM_CONFIG__

static const char *const fontfamilies = "Terminus, Emacs Simsun";
static const int fontsize = 15;

static char *tags[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
static const int tag_lrpad = 10; /* left and right padding of tag */
static const int status_lrpad = 6; /* left and right padding of status */
static const int bar_tbpad = 3;    /* top and bottom padding of bar */

#define TAGMASK ((1 << LENGTH(tags)) - 1)

static const char col_gray1[] = "#222222";
static const char col_gray2[] = "#444444";
static const char col_gray3[] = "#bbbbbb";
static const char col_gray4[] = "#eeeeee";
static const char col_cyan[] = "#005577";
static const char *const colors[SchemeLast][ColLast] = {
    [SchemeNorm] = {col_gray3, col_gray1, col_gray2},
    [SchemeSel] = {col_gray4, col_cyan, col_cyan},
};

static const char dmenufont[] = "Terminus:size=10";
static const char dmenumon[2] = "0";
static const char *dmenucmd[] = {
    "dmenu_run", "-m",     "0",       "-c",      "-l",      "25",  "-g",
    "3",         "-fn",    dmenufont, "-nb",     col_gray1, "-nf", col_gray3,
    "-sb",       col_cyan, "-sf",     col_gray4, NULL};

static void quit(const Arg *arg) {
    global.running = false;
    if (arg->i) {
        global.restart = true;
    }
}

static void toggle_mute(const Arg *arg) { toggle_pulse_mute(); }
static void change_volume(const Arg *arg) { change_pulse_volume(arg->i); }

static Key keys[] = {
    {XCB_MOD_MASK_4, XK_p, spawn, {.v = dmenucmd}},
    {XCB_MOD_MASK_4, XK_q, quit, {.v = NULL}},
    {XCB_MOD_MASK_4, XK_r, quit, {.i = 1}},
    {XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT, XK_m, toggle_mute},
    {XCB_MOD_MASK_4, XK_Up, change_volume, {.i = 1}},
    {XCB_MOD_MASK_4, XK_Down, change_volume, {.i = -1}},
};

#endif
