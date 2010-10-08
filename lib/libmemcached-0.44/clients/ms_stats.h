/*
 * File:   ms_stats.h
 * Author: Mingqiang Zhuang
 *
 * Created on March 25, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */
#ifndef MS_STAT_H
#define MS_STAT_H

#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* statistic structure of response time */
typedef struct
{
  char *name;
  uint64_t total_time;
  uint64_t min_time;
  uint64_t max_time;
  uint64_t get_miss;
  uint64_t dist[65];
  double squares;
  double log_product;

  uint64_t period_min_time;
  uint64_t period_max_time;
  uint64_t pre_get_miss;
  uint64_t pre_events;
  uint64_t pre_total_time;
  uint64_t pre_squares;
  double pre_log_product;
} ms_stat_t;

/* initialize statistic */
void ms_init_stats(ms_stat_t *stat, const char *name);


/* record one event */
void ms_record_event(ms_stat_t *stat, uint64_t time, int get_miss);


/* dump the statistics */
void ms_dump_stats(ms_stat_t *stat);


/* dump the format statistics */
void ms_dump_format_stats(ms_stat_t *stat,
                          int run_time,
                          int freq,
                          int obj_size);


#ifdef __cplusplus
}
#endif

#endif  /* MS_STAT_H */
