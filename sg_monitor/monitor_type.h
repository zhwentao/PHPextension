#ifndef MONITOR_TYPE_H 
#define MONITOR_TYPE_H

/**
 * URI monitor statistic struct
 */
typedef struct {
    int32_t cpu_load;                       /* cpu*/
    size_t  mem_phys;                       /* physical mem size */
    size_t  mem_virt;                       /* virtual mem size */
    int32_t latency;                        /* response latency (ms)*/
    int32_t uri_count;                      /* uri request count */
    int32_t failed_count;                   /* failed count, request failed count */
} uri_stat;

/**
 * function call monitor statistic struct
 */
typedef struct {
    size_t  mem_phys;                       /* physical mem size */
    size_t  mem_virt;                       /* virtual mem size */
    int32_t latency;                        /* response latency */
    int32_t call_count;                     /* function call count */
    int32_t failed_count;                   /* failed count, function call failed count */
} func_stat;

#endif
