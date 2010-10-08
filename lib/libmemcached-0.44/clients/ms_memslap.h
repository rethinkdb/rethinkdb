/*
 * File:   ms_memslap.h
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */
#ifndef MS_MEMSLAP_H
#define MS_MEMSLAP_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#if !defined(__cplusplus)
# include <stdbool.h>
#endif
#include <math.h>

#include "ms_stats.h"

#ifdef __cplusplus
extern "C" {
#endif

/* command line option  */
typedef enum
{
  OPT_VERSION= 'V',
  OPT_HELP= 'h',
  OPT_UDP= 'U',
  OPT_SERVERS= 's',
  OPT_EXECUTE_NUMBER= 'x',
  OPT_THREAD_NUMBER= 'T',
  OPT_CONCURRENCY= 'c',
  OPT_FIXED_LTH= 'X',
  OPT_VERIFY= 'v',
  OPT_GETS_DIVISION= 'd',
  OPT_TIME= 't',
  OPT_CONFIG_CMD= 'F',
  OPT_WINDOW_SIZE= 'w',
  OPT_EXPIRE= 'e',
  OPT_STAT_FREQ= 'S',
  OPT_RECONNECT= 'R',
  OPT_VERBOSE= 'b',
  OPT_FACEBOOK_TEST= 'a',
  OPT_SOCK_PER_CONN= 'n',
  OPT_BINARY_PROTOCOL= 'B',
  OPT_OVERWRITE= 'o',
  OPT_TPS= 'P',
  OPT_REP_WRITE_SRV= 'p',
} ms_options_t;

/* global statistic of response time */
typedef struct statistic
{
  pthread_mutex_t stat_mutex;       /* synchronize the following members */

  ms_stat_t get_stat;               /* statistics of get command */
  ms_stat_t set_stat;               /* statistics of set command */
  ms_stat_t total_stat;             /* statistics of both get and set commands */
} ms_statistic_t;

/* global status statistic structure */
typedef struct stats
{
  volatile uint32_t active_conns;   /* active connections */
  size_t bytes_read;              /* read bytes */
  size_t bytes_written;           /* written bytes */
  size_t obj_bytes;               /* object bytes */
  size_t pre_cmd_get;             /* previous total get command count */
  size_t pre_cmd_set;             /* previous total set command count */
  size_t cmd_get;                 /* current total get command count */
  size_t cmd_set;                 /* current total set command count */
  size_t get_misses;              /* total objects of get miss */
  size_t vef_miss;                /* total objects of verification miss  */
  size_t vef_failed;              /* total objects of verification failed  */
  size_t unexp_unget;             /* total objects which is unexpired but not get */
  size_t exp_get;                 /* total objects which is expired but get  */
  volatile size_t pkt_disorder;            /* disorder packages of UDP */
  size_t pkt_drop;                /* packages dropped of UDP */
  size_t udp_timeout;             /* how many times timeout of UDP happens */
} ms_stats_t;

/* lock adapter */
typedef struct sync_lock
{
  uint32_t count;
  pthread_mutex_t lock;
  pthread_cond_t cond;
} ms_sync_lock_t;

/* global variable structure */
typedef struct global
{
  /* synchronize lock */
  ms_sync_lock_t init_lock;
  ms_sync_lock_t warmup_lock;
  ms_sync_lock_t run_lock;

  /* mutex for outputing error log synchronously when memslap crashes */
  pthread_mutex_t quit_mutex;

  /* mutex for generating key prefix */
  pthread_mutex_t seq_mutex;

  /* global synchronous flags for slap mode */
  bool finish_warmup;
  bool time_out;
} ms_global_t;

/* global structure */
ms_global_t ms_global;

/* global stats information structure */
ms_stats_t ms_stats;

/* global statistic structure */
ms_statistic_t ms_statistic;

#ifdef __cplusplus
}
#endif

#endif /* end of MS_MEMSLAP_H */
