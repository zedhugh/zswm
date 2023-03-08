#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "utils.h"
#include "zswm.h"

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

void *ecalloc(size_t nmemb, size_t size) {
    void *p;

    if (!(p = calloc(nmemb, size))) {
        die("calloc:");
    }

    return p;
}

void spawn(const Arg * arg) {
    if (fork() == 0) {
        setsid();
        execvp(((char **)arg->v)[0], arg->v);
        die("zswm execvp '%s' failed:", ((char **)arg->v)[0]);
    }
}
