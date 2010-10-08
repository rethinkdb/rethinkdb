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

#include "config.h"

#include <inttypes.h>
#include "ms_stats.h"

#define array_size(x)    (sizeof(x) / sizeof((x)[0]))

static int ms_local_log2(uint64_t value);
static uint64_t ms_get_events(ms_stat_t *stat);


/**
 * get the index of local log2 array
 *
 * @param value
 *
 * @return return the index of local log2 array
 */
static int ms_local_log2(uint64_t value)
{
  int result= 0;

  while (result <= 63 && ((uint64_t)1 << result) < value)
  {
    result++;
  }

  return result;
} /* ms_local_log2 */


/**
 * initialize statistic structure
 *
 * @param stat, pointer of the statistic structure
 * @param name, name of the statistic
 */
void ms_init_stats(ms_stat_t *stat, const char *name)
{
  memset(stat, 0, sizeof(*stat));

  stat->name= (char *)name;
  stat->min_time= (uint64_t)-1;
  stat->max_time= 0;
  stat->period_min_time= (uint64_t)-1;
  stat->period_max_time= 0;
  stat->log_product= 0;
  stat->total_time= 0;
  stat->pre_total_time= 0;
  stat->squares= 0;
  stat->pre_squares= 0;
  stat->pre_events= 0;
  stat->pre_log_product= 0;
  stat->get_miss= 0;
  stat->pre_get_miss= 0;
} /* ms_init_stats */


/**
 * record one event
 *
 * @param stat, pointer of the statistic structure
 * @param total_time, response time of the command
 * @param get_miss, whether it gets miss
 */
void ms_record_event(ms_stat_t *stat, uint64_t total_time, int get_miss)
{
  stat->total_time+= total_time;

  if (total_time < stat->min_time)
  {
    stat->min_time= total_time;
  }

  if (total_time > stat->max_time)
  {
    stat->max_time= total_time;
  }

  if (total_time < stat->period_min_time)
  {
    stat->period_min_time= total_time;
  }

  if (total_time > stat->period_max_time)
  {
    stat->period_max_time= total_time;
  }

  if (get_miss)
  {
    stat->get_miss++;
  }

  stat->dist[ms_local_log2(total_time)]++;
  stat->squares+= (double)(total_time * total_time);

  if (total_time != 0)
  {
    stat->log_product+= log((double)total_time);
  }
} /* ms_record_event */


/**
 * get the events count
 *
 * @param stat, pointer of the statistic structure
 *
 * @return total events recorded
 */
static uint64_t ms_get_events(ms_stat_t *stat)
{
  uint64_t events= 0;

  for (uint32_t i= 0; i < array_size(stat->dist); i++)
  {
    events+= stat->dist[i];
  }

  return events;
} /* ms_get_events */


/**
 * dump the statistics
 *
 * @param stat, pointer of the statistic structure
 */
void ms_dump_stats(ms_stat_t *stat)
{
  uint64_t events= 0;
  int max_non_zero= 0;
  int min_non_zero= 0;
  double average= 0;

  for (uint32_t i= 0; i < array_size(stat->dist); i++)
  {
    events+= stat->dist[i];
    if (stat->dist[i] != 0)
    {
      max_non_zero= (int)i;
    }
  }

  if (events == 0)
  {
    return;
  }
  average= (double)(stat->total_time / events);

  printf("%s Statistics (%lld events)\n", stat->name, (long long)events);
  printf("   Min:  %8lld\n", (long long)stat->min_time);
  printf("   Max:  %8lld\n", (long long)stat->max_time);
  printf("   Avg:  %8lld\n", (long long)(stat->total_time / events));
  printf("   Geo:  %8.2lf\n", exp(stat->log_product / (double)events));

  if (events > 1)
  {
    printf("   Std:  %8.2lf\n",
           sqrt((stat->squares - (double)events * average
                 * average) / ((double)events - 1)));
  }
  printf("   Log2 Dist:");

  for (int i= 0; i <= max_non_zero - 4; i+= 4)
  {
    if ((stat->dist[i + 0] != 0)
        || (stat->dist[i + 1] != 0)
        || (stat->dist[i + 2] != 0)
        || (stat->dist[i + 3] != 0))
    {
      min_non_zero= i;
      break;
    }
  }

  for (int i= min_non_zero; i <= max_non_zero; i++)
  {
    if ((i % 4) == 0)
    {
      printf("\n      %2d:", (int)i);
    }
    printf("   %6" PRIu64 , stat->dist[i]);
  }

  printf("\n\n");
} /* ms_dump_stats */


/**
 * dump the format statistics
 *
 * @param stat, pointer of the statistic structure
 * @param run_time, the total run time
 * @param freq, statistic frequency
 * @param obj_size, average object size
 */
void ms_dump_format_stats(ms_stat_t *stat,
                          int run_time,
                          int freq,
                          int obj_size)
{
  uint64_t events= 0;
  double global_average= 0;
  uint64_t global_tps= 0;
  double global_rate= 0;
  double global_std= 0;
  double global_log= 0;

  uint64_t diff_time= 0;
  uint64_t diff_events= 0;
  double diff_squares= 0;
  double diff_log_product= 0;
  double period_average= 0;
  uint64_t period_tps= 0;
  double period_rate= 0;
  double period_std= 0;
  double period_log= 0;

  if ((events= ms_get_events(stat)) == 0)
  {
    return;
  }

  global_average= (double)(stat->total_time / events);
  global_tps= events / (uint64_t)run_time;
  global_rate= (double)events * obj_size / 1024 / 1024 / run_time;
  global_std= sqrt((stat->squares - (double)events * global_average
                    * global_average) / (double)(events - 1));
  global_log= exp(stat->log_product / (double)events);

  diff_time= stat->total_time - stat->pre_total_time;
  diff_events= events - stat->pre_events;
  if (diff_events >= 1)
  {
    period_average= (double)(diff_time / diff_events);
    period_tps= diff_events / (uint64_t)freq;
    period_rate= (double)diff_events * obj_size / 1024 / 1024 / freq;
    diff_squares= (double)stat->squares - (double)stat->pre_squares;
    period_std= sqrt((diff_squares - (double)diff_events * period_average
                      * period_average) / (double)(diff_events - 1));
    diff_log_product= stat->log_product - stat->pre_log_product;
    period_log= exp(diff_log_product / (double)diff_events);
  }

  printf("%s Statistics\n", stat->name);
  printf("%-8s %-8s %-12s %-12s %-10s %-10s %-8s %-10s %-10s %-10s %-10s\n",
         "Type",
         "Time(s)",
         "Ops",
         "TPS(ops/s)",
         "Net(M/s)",
         "Get_miss",
         "Min(us)",
         "Max(us)",
         "Avg(us)",
         "Std_dev",
         "Geo_dist");

  printf(
    "%-8s %-8d %-12llu %-12lld %-10.1f %-10lld %-8lld %-10lld %-10lld %-10.2f %.2f\n",
    "Period",
    freq,
    (long long)diff_events,
    (long long)period_tps,
    global_rate,
    (long long)(stat->get_miss - stat->pre_get_miss),
    (long long)stat->period_min_time,
    (long long)stat->period_max_time,
    (long long)period_average,
    period_std,
    period_log);

  printf(
    "%-8s %-8d %-12llu %-12lld %-10.1f %-10lld %-8lld %-10lld %-10lld %-10.2f %.2f\n\n",
    "Global",
    run_time,
    (long long)events,
    (long long)global_tps,
    period_rate,
    (long long)stat->get_miss,
    (long long)stat->min_time,
    (long long)stat->max_time,
    (long long)global_average,
    global_std,
    global_log);

  stat->pre_events= events;
  stat->pre_squares= (uint64_t)stat->squares;
  stat->pre_total_time= stat->total_time;
  stat->pre_log_product= stat->log_product;
  stat->period_min_time= (uint64_t)-1;
  stat->period_max_time= 0;
  stat->pre_get_miss= stat->get_miss;
} /* ms_dump_format_stats */
