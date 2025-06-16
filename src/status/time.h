#ifndef __ZSWM_TIME_
#define __ZSWM_TIME_

typedef struct {
    char time[16];
    char date[16];
    char full[32];
} StatusTime;

typedef void (*status_time_notify)(StatusTime time);
void init_status_time(status_time_notify cb);
void clean_status_time(void);

#endif
