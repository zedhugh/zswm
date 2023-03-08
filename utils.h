#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zswm.h"

#ifndef __ZS_WM_UTILS__
#define __ZS_WM_UTILS__

#define LENGTH(X)               (sizeof X / sizeof X[0])

double get_time();

void die(const char *fmt, ...);

void *ecalloc(size_t nmemb, size_t size);

void spawn (const Arg *arg);

#endif
