#ifndef __ZSWM_CPU_
#define __ZSWM_CPU_

typedef void (*cpu_usage_notify)(double percent);
void init_cpu_usage(cpu_usage_notify cb);
void clean_cpu_usage(void);

#endif
