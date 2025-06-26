#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "layout.h"
#include "utils.h"
#include "window.h"

void monocle(Monitor *monitor) {
    Client *c = next_show_client(monitor->clients);

    for (; c; c = next_show_client(c->next)) {
        Monitor *m = c->mon;
        c->width = m->ww - 2 * c->bw;
        c->height = m->wh - 2 * c->bw;
        c->x = m->wx;
        c->y = m->wy;
        change_window_rect(c->win, c->x, c->y, c->width, c->height, c->bw);
    }
}

void tile(Monitor *monitor) {
    uint32_t n;
    Client *c = NULL;
    for (n = 0, c = next_show_client(monitor->clients); c;
         c = next_show_client(c->next), ++n) {
    }

    if (n == 0) {
        return;
    }

    uint32_t columns = (uint32_t)floor(sqrt(n));
    if (columns * (columns + 1) <= n) {
        columns++;
    }
    uint32_t rows_in_other_cols = n / columns;
    uint32_t rows_in_main_col;
    while ((rows_in_main_col = n - (columns - 1) * rows_in_other_cols) >
           rows_in_other_cols) {
        rows_in_other_cols++;
    }

    uint32_t width_avg = monitor->ww / columns;
    uint32_t width_for_main_col = monitor->ww - (columns - 1) * width_avg;
    uint32_t width_for_other_cols = width_avg;

    uint32_t i = 0, row = 0, col = 0, row_count, width, height;
    int32_t y = monitor->wy, x = monitor->wx;
    for (c = next_show_client(monitor->clients); c;
         c = next_show_client(c->next), i++) {
        if (i < rows_in_main_col) {
            row = i;
            width = width_for_main_col;
            row_count = rows_in_main_col;
        } else {
            width = width_for_other_cols;
            row_count = rows_in_other_cols;
            if (((i - rows_in_main_col) % row_count) == 0) {
                col++;
                row = 0;
                x += col == 1 ? width_for_main_col : width_for_other_cols;
                y = monitor->wy;
            }
        }

        uint32_t h_avg = monitor->wh / row_count;
        uint32_t h_main = monitor->wh - h_avg * (row_count - 1);
        height = row == 0 ? h_main : h_avg;
        c->x = x;
        c->y = y;
        c->width = width;
        c->height = height;
        y += height;
        change_window_rect(c->win, c->x, c->y, c->width, c->height, c->bw);
    }
}
