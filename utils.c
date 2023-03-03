#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <sys/time.h>
#include "utils.h"

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return  tv.tv_sec + tv.tv_usec / 1000000.0;
}

void die(const char *fmt, ...)  {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
        fputc(' ', stderr);
        perror(NULL);
    } else {
        fputc('\n', stderr);
    }

    exit(1);
}
