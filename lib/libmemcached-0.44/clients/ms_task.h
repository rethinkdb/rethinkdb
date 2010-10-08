/*
 * File:   ms_task.h
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */
#ifndef MS_TASK_H
#define MS_TASK_H

#include <sys/types.h>
#include <stdint.h>
#if !defined(__cplusplus)
# include <stdbool.h>
#endif
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNIT_ITEMS_COUNT     1024               /* each window unit has 1024 items */
#define KEY_PREFIX_SIZE      (sizeof(uint64_t)) /* key prefix length: 8 bytes */
#define INVALID_OFFSET       (-1)               /* invalid offset in the character table */
#define FIXED_EXPIRE_TIME    60                 /* default expire time is 60s */
#define EXPIRE_TIME_ERROR    5                  /* default expire time error is 5s */

/* information of a task item(object) */
typedef struct task_item
{
  uint64_t key_prefix;                  /* prefix of the key, 8 bytes, binary */
  int key_size;                         /* key size */
  int key_suffix_offset;                /* suffix offset in the global character table */

  int value_size;                       /* data size */
  int value_offset;                     /* data offset in the global character table */

  time_t client_time;                   /* the current client time */
  int exp_time;                         /* expire time */
} ms_task_item_t;

/* task item for multi-get */
typedef struct mlget_task_item
{
  ms_task_item_t *item;                 /* task item */
  bool verify;                          /* whether verify data or not */
  bool finish_verify;                   /* whether finish data verify or not */
  bool get_miss;                        /* whether get miss or not */
} ms_mlget_task_item_t;

/* information of multi-get task */
typedef struct mlget_task
{
  ms_mlget_task_item_t *mlget_item;        /* multi-get task array */
  int mlget_num;                           /* how many tasks in mlget_task array */
  int value_index;                         /* the nth value received by the connect, for multi-get */
} ms_mlget_task_t;

/* structure used to store the state of the running task */
typedef struct task
{
  int cmd;                              /* command name */
  bool verify;                          /* whether verify data or not */
  bool finish_verify;                   /* whether finish data verify or not */
  bool get_miss;                        /* whether get miss or not */
  ms_task_item_t *item;                 /* task item */

  /* counter for command distribution adjustment */
  uint64_t get_opt;                     /* number of total get operations */
  uint64_t set_opt;                     /* number of total set operations, no including warmup set count */
  int cycle_undo_get;                   /* number of undo get in an adjustment cycle */
  int cycle_undo_set;                   /* number of undo set in an adjustment cycle */
  uint64_t verified_get;                /* number of total verified get operations */
  uint64_t overwrite_set;               /* number of total overwrite set operations */
} ms_task_t;

struct conn;

/* the state machine call the function to execute task.*/
int ms_exec_task(struct conn *c);


/* calculate the difference value of two time points */
int64_t ms_time_diff(struct timeval *start_time, struct timeval *end_time);


#ifdef __cplusplus
}
#endif

#endif /* end of MS_TASK_H */
