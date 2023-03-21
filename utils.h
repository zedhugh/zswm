#ifndef __ZS_WM_UTILS__
#define __ZS_WM_UTILS__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zswm.h"


#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define LENGTH(X)               (sizeof X / sizeof X[0])

double get_time();

void die(const char *fmt, ...);

void logger(const char *fmt, ...);

void *ecalloc(size_t nmemb, size_t size);

void spawn (const Arg *arg);

uint32_t alloc_color(const char *color);

#endif
