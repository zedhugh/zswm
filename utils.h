#ifndef __ZS_WM_UTILS__
#define __ZS_WM_UTILS__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double get_time();

void die(const char *fmt, ...);

void *ecalloc(size_t nmemb, size_t size);

#endif
