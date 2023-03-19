#include <X11/keysym.h>
#include <xcb/xproto.h>

#include "utils.h"
#include "zswm.h"

#ifndef __ZS_WM_CONFIG__
#define __ZS_WM_CONFIG__

static const char col_gray1[] = "#222222";
static const char col_gray2[] = "#444444";
static const char col_gray3[] = "#bbbbbb";
static const char col_gray4[] = "#eeeeee";
static const char col_cyan[]  = "#005577";

static const char dmenufont[] = "Terminus:size=10";
static const char dmenumon[2] = "0";
static const char *dmenucmd[] = { "dmenu_run", "-m", "0", "-c", "-l", "25", "-g", "3", "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };

static void quit(const Arg *) {
    running = 0;
}

static Key keys[] = {
    {XCB_MOD_MASK_4, XK_p, spawn, { .v = dmenucmd } },
    {XCB_MOD_MASK_4, XK_q, quit, { .v = NULL } },
};

#endif
