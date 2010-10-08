/*
 * File:   ms_task.c
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */

#include "config.h"

#include <inttypes.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "ms_thread.h"
#include "ms_setting.h"
#include "ms_atomic.h"

/* command distribution adjustment cycle */
#define CMD_DISTR_ADJUST_CYCLE    1000
#define DISADJUST_FACTOR          0.03 /**
                                 * In one adjustment cycle, if undo set or get
                                 * operations proportion is more than 3% , means
                                 * there are too many new item or need more new
                                 * item in the window. This factor shows it.
                                 */

/* get item from task window */
static ms_task_item_t *ms_get_cur_opt_item(ms_conn_t *c);
static ms_task_item_t *ms_get_next_get_item(ms_conn_t *c);
static ms_task_item_t *ms_get_next_set_item(ms_conn_t *c);
static ms_task_item_t *ms_get_random_overwrite_item(ms_conn_t *c);


/* select next operation to do */
static void ms_select_opt(ms_conn_t *c, ms_task_t *task);


/* set and get speed estimate for controlling and adjustment */
static bool ms_is_set_too_fast(ms_task_t *task);
static bool ms_is_get_too_fast(ms_task_t *task);
static void ms_kick_out_item(ms_task_item_t *item);


/* miss rate adjustment */
static bool ms_need_overwrite_item(ms_task_t *task);
static bool ms_adjust_opt(ms_conn_t *c, ms_task_t *task);


/* deal with data verification initialization */
static void ms_task_data_verify_init(ms_task_t *task);
static void ms_task_expire_verify_init(ms_task_t *task);


/* select a new task to do */
static ms_task_t *ms_get_task(ms_conn_t *c, bool warmup);


/* run the selected task */
static void ms_update_set_result(ms_conn_t *c, ms_task_item_t *item);
static void ms_update_stat_result(ms_conn_t *c);
static void ms_update_multi_get_result(ms_conn_t *c);
static void ms_update_single_get_result(ms_conn_t *c, ms_task_item_t *item);
static void ms_update_task_result(ms_conn_t *c);
static void ms_single_getset_task_sch(ms_conn_t *c);
static void ms_multi_getset_task_sch(ms_conn_t *c);
static void ms_send_signal(ms_sync_lock_t *sync_lock);
static void ms_warmup_server(ms_conn_t *c);
static int ms_run_getset_task(ms_conn_t *c);


/**
 * used to get the current operation item(object)
 *
 * @param c, pointer of the concurrency
 *
 * @return ms_task_item_t*, current operating item
 */
static ms_task_item_t *ms_get_cur_opt_item(ms_conn_t *c)
{
  return c->curr_task.item;
}


/**
 * used to get the next item to do get operation
 *
 * @param c, pointer of the concurrency
 *
 * @return ms_task_item_t*, the pointer of the next item to do
 *         get operation
 */
static ms_task_item_t *ms_get_next_get_item(ms_conn_t *c)
{
  ms_task_item_t *item= NULL;

  if (c->set_cursor <= 0)
  {
    /* the first item in the window */
    item= &c->item_win[0];
  }
  else if (c->set_cursor > 0 && c->set_cursor < (uint32_t)c->win_size)
  {
    /* random get one item set before */
    item= &c->item_win[random() % (int64_t)c->set_cursor];
  }
  else
  {
    /* random get one item from the window */
    item= &c->item_win[random() % c->win_size];
  }

  return item;
} /* ms_get_next_get_item */


/**
 * used to get the next item to do set operation
 *
 * @param c, pointer of the concurrency
 *
 * @return ms_task_item_t*, the pointer of the next item to do
 *         set operation
 */
static ms_task_item_t *ms_get_next_set_item(ms_conn_t *c)
{
  /**
   *  when a set command successes, the cursor will plus 1. If set
   *  fails, the cursor doesn't change. it isn't necessary to
   *  increase the cursor here.
   */
  return &c->item_win[(int64_t)c->set_cursor % c->win_size];
}


/**
 * If we need do overwrite, we could select a item set before.
 * This function is used to get a item set before to do
 * overwrite.
 *
 * @param c, pointer of the concurrency
 *
 * @return ms_task_item_t*, the pointer of the previous item of
 *         set operation
 */
static ms_task_item_t *ms_get_random_overwrite_item(ms_conn_t *c)
{
    return ms_get_next_get_item(c);
} /* ms_get_random_overwrite_item */

/**
 * According to the proportion of operations(get or set), select
 * an operation to do.
 *
 * @param c, pointer of the concurrency
 * @param task, pointer of current task in the concurrency
 */
static void ms_select_opt(ms_conn_t *c, ms_task_t *task)
{
  double get_prop= ms_setting.cmd_distr[CMD_GET].cmd_prop;
  double set_prop= ms_setting.cmd_distr[CMD_SET].cmd_prop;

  /* update cycle operation number if necessary */
  if ((task->cycle_undo_get == 0) || (task->cycle_undo_set == 0))
  {
    task->cycle_undo_get+= (int)(CMD_DISTR_ADJUST_CYCLE * get_prop);
    task->cycle_undo_set+= (int)(CMD_DISTR_ADJUST_CYCLE * set_prop);
  }

  /**
   *  According to operation distribution to choose doing which
   *  operation. If it can't set new object to sever, just change
   *  to do get operation.
   */
  if ((set_prop > PROP_ERROR)
      && ((double)task->get_opt * set_prop >= (double)task->set_opt
          * get_prop))
  {
    task->cmd= CMD_SET;
    task->item= ms_get_next_set_item(c);
  }
  else
  {
    task->cmd= CMD_GET;
    task->item= ms_get_next_get_item(c);
  }
} /* ms_select_opt */


/**
 * used to judge whether the number of get operations done is
 * more than expected number of get operations to do right now.
 *
 * @param task, pointer of current task in the concurrency
 *
 * @return bool, if get too fast, return true, else return false
 */
static bool ms_is_get_too_fast(ms_task_t *task)
{
  double get_prop= ms_setting.cmd_distr[CMD_GET].cmd_prop;
  double set_prop= ms_setting.cmd_distr[CMD_SET].cmd_prop;

  /* no get operation */
  if (get_prop < PROP_ERROR)
  {
    return false;
  }

  int max_undo_set= (int)(set_prop / get_prop * (1.0 + DISADJUST_FACTOR))
                    * task->cycle_undo_get;

  if (((double)task->get_opt * set_prop > (double)task->set_opt * get_prop)
      && (task->cycle_undo_set > max_undo_set))
  {
    return true;
  }

  return false;
} /* ms_is_get_too_fast */


/**
 * used to judge whether the number of set operations done is
 * more than expected number of set operations to do right now.
 *
 * @param task, pointer of current task in the concurrency
 *
 * @return bool, if set too fast, return true, else return false
 */
static bool ms_is_set_too_fast(ms_task_t *task)
{
  double get_prop= ms_setting.cmd_distr[CMD_GET].cmd_prop;
  double set_prop= ms_setting.cmd_distr[CMD_SET].cmd_prop;

  /* no set operation */
  if (set_prop < PROP_ERROR)
  {
    return false;
  }

  /* If it does set operation too fast, skip some */
  int max_undo_get= (int)((get_prop / set_prop * (1.0 + DISADJUST_FACTOR))
                          * (double)task->cycle_undo_set);

  if (((double)task->get_opt * set_prop < (double)task->set_opt * get_prop)
      && (task->cycle_undo_get > max_undo_get))
  {
    return true;
  }

  return false;
} /* ms_is_set_too_fast */


/**
 * kick out the old item in the window, and add a new item to
 * overwrite the old item. When we don't want to do overwrite
 * object, and the current item to do set operation is an old
 * item, we could kick out the old item and add a new item. Then
 * we can ensure we set new object every time.
 *
 * @param item, pointer of task item which includes the object
 *            information
 */
static void ms_kick_out_item(ms_task_item_t *item)
{
  /* allocate a new item */
  item->key_prefix= ms_get_key_prefix();

  item->key_suffix_offset++;
  item->value_offset= INVALID_OFFSET;       /* new item use invalid value offset */
  item->client_time= 0;
} /* ms_kick_out_item */


/**
 *  used to judge whether we need overwrite object based on the
 *  options user specified
 *
 * @param task, pointer of current task in the concurrency
 *
 * @return bool, if need overwrite, return true, else return
 *         false
 */
static bool ms_need_overwrite_item(ms_task_t *task)
{
  ms_task_item_t *item= task->item;

  assert(item != NULL);
  assert(task->cmd == CMD_SET);

  /**
   *  according to data overwrite percent to determine if do data
   *  overwrite.
   */
  if (task->overwrite_set < (double)task->set_opt
      * ms_setting.overwrite_percent)
  {
    return true;
  }

  return false;
} /* ms_need_overwirte_item */


/**
 * used to adjust operation. the function must be called after
 * select operation. the function change get operation to set
 * operation, or set operation to get operation based on the
 * current case.
 *
 * @param c, pointer of the concurrency
 * @param task, pointer of current task in the concurrency
 *
 * @return bool, if success, return true, else return false
 */
static bool ms_adjust_opt(ms_conn_t *c, ms_task_t *task)
{
  ms_task_item_t *item= task->item;

  assert(item != NULL);

  if (task->cmd == CMD_SET)
  {
    /* If did set operation too fast, skip some */
    if (ms_is_set_too_fast(task))
    {
      /* get the item instead */
      if (item->value_offset != INVALID_OFFSET)
      {
        task->cmd= CMD_GET;
        return true;
      }
    }

    /* If the current item is not a new item, kick it out */
    if (item->value_offset != INVALID_OFFSET)
    {
      if (ms_need_overwrite_item(task))
      {
        /* overwrite */
        task->overwrite_set++;
      }
      else
      {
        /* kick out the current item to do set operation */
        ms_kick_out_item(item);
      }
    }
    else            /* it's a new item */
    {
      /* need overwrite */
      if (ms_need_overwrite_item(task))
      {
        /**
         *  overwrite not use the item with current set cursor, revert
         *  set cursor.
         */
        c->set_cursor--;

        item= ms_get_random_overwrite_item(c);
        if (item->value_offset != INVALID_OFFSET)
        {
          task->item= item;
          task->overwrite_set++;
        }
        else                /* item is a new item */
        {
          /* select the item to run, and cancel overwrite */
          task->item= item;
        }
      }
    }
    task->cmd= CMD_SET;
    return true;
  }
  else
  {
    if (item->value_offset == INVALID_OFFSET)
    {
      task->cmd= CMD_SET;
      return true;
    }

    /**
     *  If It does get operation too fast, it will change the
     *  operation to set.
     */
    if (ms_is_get_too_fast(task))
    {
      /* don't kick out the first item in the window */
      if (! ms_is_set_too_fast(task))
      {
        ms_kick_out_item(item);
        task->cmd= CMD_SET;
        return true;
      }
      else
      {
        return false;
      }
    }

    assert(item->value_offset != INVALID_OFFSET);

    task->cmd= CMD_GET;
    return true;
  }
} /* ms_adjust_opt */


/**
 * used to initialize the task which need verify data.
 *
 * @param task, pointer of current task in the concurrency
 */
static void ms_task_data_verify_init(ms_task_t *task)
{
  ms_task_item_t *item= task->item;

  assert(item != NULL);
  assert(task->cmd == CMD_GET);

  /**
   *  according to data verification percent to determine if do
   *  data verification.
   */
  if (task->verified_get < (double)task->get_opt
      * ms_setting.verify_percent)
  {
    /**
     *  currently it doesn't do verify, just increase the counter,
     *  and do verification next proper get command
     */
    if ((task->item->value_offset != INVALID_OFFSET)
        && (item->exp_time == 0))
    {
      task->verify= true;
      task->finish_verify= false;
      task->verified_get++;
    }
  }
} /* ms_task_data_verify_init */


/**
 * used to initialize the task which need verify expire time.
 *
 * @param task, pointer of current task in the concurrency
 */
static void ms_task_expire_verify_init(ms_task_t *task)
{
  ms_task_item_t *item= task->item;

  assert(item != NULL);
  assert(task->cmd == CMD_GET);
  assert(item->exp_time > 0);

  task->verify= true;
  task->finish_verify= false;
} /* ms_task_expire_verify_init */


/**
 * used to get one task, the function initializes the task
 * structure.
 *
 * @param c, pointer of the concurrency
 * @param warmup, whether it need warmup
 *
 * @return ms_task_t*, pointer of current task in the
 *         concurrency
 */
static ms_task_t *ms_get_task(ms_conn_t *c, bool warmup)
{
  ms_task_t *task= &c->curr_task;

  while (1)
  {
    task->verify= false;
    task->finish_verify= true;
    task->get_miss= true;

    if (warmup)
    {
      task->cmd= CMD_SET;
      task->item= ms_get_next_set_item(c);

      return task;
    }

    /* according to operation distribution to choose doing which operation */
    ms_select_opt(c, task);

    if (! ms_adjust_opt(c, task))
    {
      continue;
    }

    if ((ms_setting.verify_percent > 0) && (task->cmd == CMD_GET))
    {
      ms_task_data_verify_init(task);
    }

    if ((ms_setting.exp_ver_per > 0) && (task->cmd == CMD_GET)
        && (task->item->exp_time > 0))
    {
      ms_task_expire_verify_init(task);
    }

    break;
  }

  /**
   *  Only update get and delete counter, set counter will be
   *  updated after set operation successes.
   */
  if (task->cmd == CMD_GET)
  {
    task->get_opt++;
    task->cycle_undo_get--;
  }

  return task;
} /* ms_get_task */


/**
 * send a signal to the main monitor thread
 *
 * @param sync_lock, pointer of the lock
 */
static void ms_send_signal(ms_sync_lock_t *sync_lock)
{
  pthread_mutex_lock(&sync_lock->lock);
  sync_lock->count++;
  pthread_cond_signal(&sync_lock->cond);
  pthread_mutex_unlock(&sync_lock->lock);
} /* ms_send_signal */


/**
 * If user only want to do get operation, but there is no object
 * in server , so we use this function to warmup the server, and
 * set some objects to server. It runs at the beginning of task.
 *
 * @param c, pointer of the concurrency
 */
static void ms_warmup_server(ms_conn_t *c)
{
  ms_task_t *task;
  ms_task_item_t *item;

  /**
   * Extra one loop to get the last command returned state.
   * Normally it gets the previous command returned state.
   */
  if ((c->remain_warmup_num >= 0)
      && (c->remain_warmup_num != c->warmup_num))
  {
    item= ms_get_cur_opt_item(c);
    /* only update the set command result state for data verification */
    if ((c->precmd.cmd == CMD_SET) && (c->precmd.retstat == MCD_STORED))
    {
      item->value_offset= item->key_suffix_offset;
      /* set success, update counter */
      c->set_cursor++;
    }
    else if (c->precmd.cmd == CMD_SET && c->precmd.retstat != MCD_STORED)
    {
      printf("key: %" PRIx64 " didn't set success\n", item->key_prefix);
    }
  }

  /* the last time don't run a task */
  if (c->remain_warmup_num-- > 0)
  {
    /* operate next task item */
    task= ms_get_task(c, true);
    item= task->item;
    ms_mcd_set(c, item);
  }

  /**
   *  finish warming up server, wait all connects initialize
   *  complete. Then all connects can start do task at the same
   *  time.
   */
  if (c->remain_warmup_num == -1)
  {
    ms_send_signal(&ms_global.warmup_lock);
    c->remain_warmup_num--;       /* never run the if branch */
  }
} /* ms_warmup_server */


/**
 * dispatch single get and set task
 *
 * @param c, pointer of the concurrency
 */
static void ms_single_getset_task_sch(ms_conn_t *c)
{
  ms_task_t *task;
  ms_task_item_t *item;

  /* the last time don't run a task */
  if (c->remain_exec_num-- > 0)
  {
    task= ms_get_task(c, false);
    item= task->item;
    if (task->cmd == CMD_SET)
    {
      ms_mcd_set(c, item);
    }
    else if (task->cmd == CMD_GET)
    {
      assert(task->cmd == CMD_GET);
      ms_mcd_get(c, item);
    }
  }
} /* ms_single_getset_task_sch */


/**
 * dispatch multi-get and set task
 *
 * @param c, pointer of the concurrency
 */
static void ms_multi_getset_task_sch(ms_conn_t *c)
{
  ms_task_t *task;
  ms_mlget_task_item_t *mlget_item;

  while (1)
  {
    if (c->remain_exec_num-- > 0)
    {
      task= ms_get_task(c, false);
      if (task->cmd == CMD_SET)             /* just do it */
      {
        ms_mcd_set(c, task->item);
        break;
      }
      else
      {
        assert(task->cmd == CMD_GET);
        mlget_item= &c->mlget_task.mlget_item[c->mlget_task.mlget_num];
        mlget_item->item= task->item;
        mlget_item->verify= task->verify;
        mlget_item->finish_verify= task->finish_verify;
        mlget_item->get_miss= task->get_miss;
        c->mlget_task.mlget_num++;

        /* enough multi-get task items can be done */
        if ((c->mlget_task.mlget_num >= ms_setting.mult_key_num)
            || ((c->remain_exec_num == 0) && (c->mlget_task.mlget_num > 0)))
        {
          ms_mcd_mlget(c);
          break;
        }
      }
    }
    else
    {
      if ((c->remain_exec_num <= 0) && (c->mlget_task.mlget_num > 0))
      {
        ms_mcd_mlget(c);
      }
      break;
    }
  }
} /* ms_multi_getset_task_sch */


/**
 * calculate the difference value of two time points
 *
 * @param start_time, the start time
 * @param end_time, the end time
 *
 * @return uint64_t, the difference value between start_time and end_time in us
 */
int64_t ms_time_diff(struct timeval *start_time, struct timeval *end_time)
{
  int64_t endtime= end_time->tv_sec * 1000000 + end_time->tv_usec;
  int64_t starttime= start_time->tv_sec * 1000000 + start_time->tv_usec;

  assert(endtime >= starttime);

  return endtime - starttime;
} /* ms_time_diff */


/**
 * after get the response from server for multi-get, the
 * function update the state of the task and do data verify if
 * necessary.
 *
 * @param c, pointer of the concurrency
 */
static void ms_update_multi_get_result(ms_conn_t *c)
{
  ms_mlget_task_item_t *mlget_item;
  ms_task_item_t *item;
  char *orignval= NULL;
  char *orignkey= NULL;

  if (c == NULL)
  {
    return;
  }
  assert(c != NULL);

  for (int i= 0; i < c->mlget_task.mlget_num; i++)
  {
    mlget_item= &c->mlget_task.mlget_item[i];
    item= mlget_item->item;
    orignval= &ms_setting.char_block[item->value_offset];
    orignkey= &ms_setting.char_block[item->key_suffix_offset];

    /* update get miss counter */
    if (mlget_item->get_miss)
    {
      atomic_add_size(&ms_stats.get_misses, 1);
    }

    /* get nothing from server for this task item */
    if (mlget_item->verify && ! mlget_item->finish_verify)
    {
      /* verify expire time if necessary */
      if (item->exp_time > 0)
      {
        struct timeval curr_time;
        gettimeofday(&curr_time, NULL);

        /* object doesn't expire but can't get it now */
        if (curr_time.tv_sec - item->client_time
            < item->exp_time - EXPIRE_TIME_ERROR)
        {
          atomic_add_size(&ms_stats.unexp_unget, 1);

          if (ms_setting.verbose)
          {
            char set_time[64];
            char cur_time[64];
            strftime(set_time, 64, "%Y-%m-%d %H:%M:%S",
                     localtime(&item->client_time));
            strftime(cur_time, 64, "%Y-%m-%d %H:%M:%S",
                     localtime(&curr_time.tv_sec));
            fprintf(stderr,
                    "\n\t<%d expire time verification failed, object "
                    "doesn't expire but can't get it now\n"
                    "\tkey len: %d\n"
                    "\tkey: %" PRIx64 " %.*s\n"
                    "\tset time: %s current time: %s "
                    "diff time: %d expire time: %d\n"
                    "\texpected data len: %d\n"
                    "\texpected data: %.*s\n"
                    "\treceived data: \n",
                    c->sfd,
                    item->key_size,
                    item->key_prefix,
                    item->key_size - (int)KEY_PREFIX_SIZE,
                    orignkey,
                    set_time,
                    cur_time,
                    (int)(curr_time.tv_sec - item->client_time),
                    item->exp_time,
                    item->value_size,
                    item->value_size,
                    orignval);
            fflush(stderr);
          }
        }
      }
      else
      {
        atomic_add_size(&ms_stats.vef_miss, 1);

        if (ms_setting.verbose)
        {
          fprintf(stderr, "\n<%d data verification failed\n"
                          "\tkey len: %d\n"
                          "\tkey: %" PRIx64 " %.*s\n"
                          "\texpected data len: %d\n"
                          "\texpected data: %.*s\n"
                          "\treceived data: \n",
                  c->sfd, item->key_size, item->key_prefix,
                  item->key_size - (int)KEY_PREFIX_SIZE,
                  orignkey, item->value_size, item->value_size, orignval);
          fflush(stderr);
        }
      }
    }
  }
  c->mlget_task.mlget_num= 0;
  c->mlget_task.value_index= INVALID_OFFSET;
} /* ms_update_multi_get_result */


/**
 * after get the response from server for single get, the
 * function update the state of the task and do data verify if
 * necessary.
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 */
static void ms_update_single_get_result(ms_conn_t *c, ms_task_item_t *item)
{
  char *orignval= NULL;
  char *orignkey= NULL;

  if ((c == NULL) || (item == NULL))
  {
    return;
  }
  assert(c != NULL);
  assert(item != NULL);

  orignval= &ms_setting.char_block[item->value_offset];
  orignkey= &ms_setting.char_block[item->key_suffix_offset];

  /* update get miss counter */
  if ((c->precmd.cmd == CMD_GET) && c->curr_task.get_miss)
  {
    atomic_add_size(&ms_stats.get_misses, 1);
  }

  /* get nothing from server for this task item */
  if ((c->precmd.cmd == CMD_GET) && c->curr_task.verify
      && ! c->curr_task.finish_verify)
  {
    /* verify expire time if necessary */
    if (item->exp_time > 0)
    {
      struct timeval curr_time;
      gettimeofday(&curr_time, NULL);

      /* object doesn't expire but can't get it now */
      if (curr_time.tv_sec - item->client_time
          < item->exp_time - EXPIRE_TIME_ERROR)
      {
        atomic_add_size(&ms_stats.unexp_unget, 1);

        if (ms_setting.verbose)
        {
          char set_time[64];
          char cur_time[64];
          strftime(set_time, 64, "%Y-%m-%d %H:%M:%S",
                   localtime(&item->client_time));
          strftime(cur_time, 64, "%Y-%m-%d %H:%M:%S",
                   localtime(&curr_time.tv_sec));
          fprintf(stderr,
                  "\n\t<%d expire time verification failed, object "
                  "doesn't expire but can't get it now\n"
                  "\tkey len: %d\n"
                  "\tkey: %" PRIx64 " %.*s\n"
                  "\tset time: %s current time: %s "
                  "diff time: %d expire time: %d\n"
                  "\texpected data len: %d\n"
                  "\texpected data: %.*s\n"
                  "\treceived data: \n",
                  c->sfd,
                  item->key_size,
                  item->key_prefix,
                  item->key_size - (int)KEY_PREFIX_SIZE,
                  orignkey,
                  set_time,
                  cur_time,
                  (int)(curr_time.tv_sec - item->client_time),
                  item->exp_time,
                  item->value_size,
                  item->value_size,
                  orignval);
          fflush(stderr);
        }
      }
    }
    else
    {
      atomic_add_size(&ms_stats.vef_miss, 1);

      if (ms_setting.verbose)
      {
        fprintf(stderr, "\n<%d data verification failed\n"
                        "\tkey len: %d\n"
                        "\tkey: %" PRIx64 " %.*s\n"
                        "\texpected data len: %d\n"
                        "\texpected data: %.*s\n"
                        "\treceived data: \n",
                c->sfd, item->key_size, item->key_prefix,
                item->key_size - (int)KEY_PREFIX_SIZE,
                orignkey, item->value_size, item->value_size, orignval);
        fflush(stderr);
      }
    }
  }
} /* ms_update_single_get_result */


/**
 * after get the response from server for set the function
 * update the state of the task and do data verify if necessary.
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 */
static void ms_update_set_result(ms_conn_t *c, ms_task_item_t *item)
{
  if ((c == NULL) || (item == NULL))
  {
    return;
  }
  assert(c != NULL);
  assert(item != NULL);

  if (c->precmd.cmd == CMD_SET)
  {
    switch (c->precmd.retstat)
    {
    case MCD_STORED:
      if (item->value_offset == INVALID_OFFSET)
      {
        /* first set with the same offset of key suffix */
        item->value_offset= item->key_suffix_offset;
      }
      else
      {
        /* not first set, just increase the value offset */
        item->value_offset+= 1;
      }

      /* set successes, update counter */
      c->set_cursor++;
      c->curr_task.set_opt++;
      c->curr_task.cycle_undo_set--;
      break;

    case MCD_SERVER_ERROR:
    default:
      break;
    } /* switch */
  }
} /* ms_update_set_result */


/**
 * update the response time result
 *
 * @param c, pointer of the concurrency
 */
static void ms_update_stat_result(ms_conn_t *c)
{
  bool get_miss= false;

  if (c == NULL)
  {
    return;
  }
  assert(c != NULL);

  gettimeofday(&c->end_time, NULL);
  uint64_t time_diff= (uint64_t)ms_time_diff(&c->start_time, &c->end_time);

  pthread_mutex_lock(&ms_statistic.stat_mutex);

  switch (c->precmd.cmd)
  {
  case CMD_SET:
    ms_record_event(&ms_statistic.set_stat, time_diff, false);
    break;

  case CMD_GET:
    if (c->curr_task.get_miss)
    {
      get_miss= true;
    }
    ms_record_event(&ms_statistic.get_stat, time_diff, get_miss);
    break;

  default:
    break;
  } /* switch */

  ms_record_event(&ms_statistic.total_stat, time_diff, get_miss);
  pthread_mutex_unlock(&ms_statistic.stat_mutex);
} /* ms_update_stat_result */


/**
 * after get response from server for the current operation, and
 * before doing the next operation, update the state of the
 * current operation.
 *
 * @param c, pointer of the concurrency
 */
static void ms_update_task_result(ms_conn_t *c)
{
  ms_task_item_t *item;

  if (c == NULL)
  {
    return;
  }
  assert(c != NULL);

  item= ms_get_cur_opt_item(c);
  if (item == NULL)
  {
    return;
  }
  assert(item != NULL);

  ms_update_set_result(c, item);

  if ((ms_setting.stat_freq > 0)
      && ((c->precmd.cmd == CMD_SET) || (c->precmd.cmd == CMD_GET)))
  {
    ms_update_stat_result(c);
  }

  /* update multi-get task item */
  if (((ms_setting.mult_key_num > 1)
       && (c->mlget_task.mlget_num >= ms_setting.mult_key_num))
      || ((c->remain_exec_num == 0) && (c->mlget_task.mlget_num > 0)))
  {
    ms_update_multi_get_result(c);
  }
  else
  {
    ms_update_single_get_result(c, item);
  }
} /* ms_update_task_result */


/**
 * run get and set operation
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_run_getset_task(ms_conn_t *c)
{
  /**
   * extra one loop to get the last command return state. get the
   * last command return state.
   */
  if ((c->remain_exec_num >= 0)
      && (c->remain_exec_num != c->exec_num))
  {
    ms_update_task_result(c);
  }

  /* multi-get */
  if (ms_setting.mult_key_num > 1)
  {
    /* operate next task item */
    ms_multi_getset_task_sch(c);
  }
  else
  {
    /* operate next task item */
    ms_single_getset_task_sch(c);
  }

  /* no task to do, exit */
  if ((c->remain_exec_num == -1) || ms_global.time_out)
  {
    return -1;
  }

  return 0;
} /* ms_run_getset_task */


/**
 * the state machine call the function to execute task.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
int ms_exec_task(struct conn *c)
{
  if (! ms_global.finish_warmup)
  {
    ms_warmup_server(c);
  }
  else
  {
    if (ms_run_getset_task(c) != 0)
    {
      return -1;
    }
  }

  return 0;
} /* ms_exec_task */
