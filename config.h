#include <X11/keysym.h>

#include "global.h"
#include "utils.h"

#ifndef __ZS_WM_CONFIG__
#define __ZS_WM_CONFIG__

typedef struct {
    uint16_t modifier;
    xcb_keysym_t keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

static char *fontfamilies[] = { "Emacs Simsun", "Terminus" };
/* static char *font_families[] = { "Terminus", "Emacs Simsun" }; */
static int fontsize = 12;

static char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

#define TAGMASK ((1 << LENGTH(tags)) - 1)

static const char col_gray1[] = "#222222";
static const char col_gray2[] = "#444444";
static const char col_gray3[] = "#bbbbbb";
static const char col_gray4[] = "#eeeeee";
static const char col_cyan[]  = "#005577";
static const char *colors[SchemeLast][ColLast] = {
    [SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
    [SchemeSel]  = { col_gray4, col_cyan, col_cyan },
};

static const char dmenufont[] = "Terminus:size=10";
static const char dmenumon[2] = "0";
static const char *dmenucmd[] = {
    "dmenu_run",
    "-m",
    "0",
    "-c",
    "-l",
    "25",
    "-g",
    "3",
    "-fn",
    dmenufont,
    "-nb",
    col_gray1,
    "-nf",
    col_gray3,
    "-sb",
    col_cyan,
    "-sf",
    col_gray4,
    NULL
};

static void quit(const Arg *arg) {
    global.running = false;
    if (arg->i) {
        global.restart = 1;
    }
}

static Key keys[] = {
    { XCB_MOD_MASK_4, XK_p, spawn, { .v = dmenucmd } },
    { XCB_MOD_MASK_4, XK_q, quit, { .v = NULL } },
    { XCB_MOD_MASK_4, XK_r, quit, { .i = 1 } },
};

#endif
