#include <stdio.h>

#include "utils.h"

static char *units[] = {"B", "K", "M", "G"};

void bytes_to_readable_size(uint64_t bytes, char *str) {
    double size = (double)bytes;
    int i = 0;
    while (size > 1024) {
        size /= 1024;
        ++i;
    }

    sprintf(str, "%.1lf%s", size, units[i]);
}
