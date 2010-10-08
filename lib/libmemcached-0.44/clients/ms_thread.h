/*
 * File:   ms_thread.h
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */

/**
 * Asynchronous memslap has the similar implementation of
 * multi-threads with memcached. Asynchronous memslap creates
 * one or more self-governed threads; each thread is bound with
 * one CPU core if the system supports setting CPU core
 * affinity. And every thread has private variables. There is
 * less communication or some shared resources among all the
 * threads. It can improve the performance because there are
 * fewer locks and competition. In addition, each thread has a
 * libevent to manage the events of network. Each thread has one
 * or more self-governed concurrencies; each concurrency has one
 * or more socket connections. All the concurrencies don't
 * communicate with each other even though they are in the same
 * thread.
 */
#ifndef MS_THREAD_H
#define MS_THREAD_H

#include <sched.h>
#include "ms_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Time relative to server start. Smaller than time_t on 64-bit systems. */
typedef unsigned int   rel_time_t;

/* Used to store the context of each thread */
typedef struct thread_ctx
{
  uint32_t thd_idx;                          /* the thread index */
  uint32_t nconns;                           /* how many connections included by the thread */
  uint32_t srv_idx;                          /* index of the thread */
  int tps_perconn;                      /* expected throughput per connection */
  int64_t exec_num_perconn;             /* execute number per connection */
} ms_thread_ctx_t;

/* Used to store the private variables of each thread */
typedef struct thread
{
  ms_conn_t *conn;                      /* conn array to store all the conn in the thread */
  uint32_t nactive_conn;                     /* how many connects are active */

  ms_thread_ctx_t *thread_ctx;          /* thread context from the caller */
  struct event_base *base;              /* libevent handler created by this thread */

  rel_time_t curr_time;                 /* current time */
  struct event clock_event;             /* clock event to time each one second */
  bool initialized;                     /* whether clock_event has been initialized */

  struct timeval startup_time;          /* start time of the thread */
} ms_thread_t;

/* initialize threads */
void ms_thread_init(void);


/* cleanup some resource of threads when all the threads exit */
void ms_thread_cleanup(void);


#ifdef __cplusplus
}
#endif

#endif /* end of MS_THREAD_H */
