/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: connects to a host, and makes sure it is alive.
 *
 */

/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#include "libmemcached/common.h"
#include "libmemcached/memcached_util.h"

#include <errno.h>
#include <pthread.h>

struct memcached_pool_st
{
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  memcached_st *master;
  memcached_st **mmc;
  int firstfree;
  uint32_t size;
  uint32_t current_size;
  char *version;
};

static memcached_return_t mutex_enter(pthread_mutex_t *mutex)
{
  int ret;
  do
    ret= pthread_mutex_lock(mutex);
  while (ret == -1 && errno == EINTR);

  return (ret == -1) ? MEMCACHED_ERRNO : MEMCACHED_SUCCESS;
}

static memcached_return_t mutex_exit(pthread_mutex_t *mutex)
{
  int ret;
  do
    ret= pthread_mutex_unlock(mutex);
  while (ret == -1 && errno == EINTR);

  return (ret == -1) ? MEMCACHED_ERRNO : MEMCACHED_SUCCESS;
}

/**
 * Grow the connection pool by creating a connection structure and clone the
 * original memcached handle.
 */
static int grow_pool(memcached_pool_st* pool)
{
  memcached_st *obj= calloc(1, sizeof(*obj));

  if (obj == NULL)
    return -1;

  if (memcached_clone(obj, pool->master) == NULL)
  {
    free(obj);
    return -1;
  }

  pool->mmc[++pool->firstfree] = obj;
  pool->current_size++;

  return 0;
}

memcached_pool_st *memcached_pool_create(memcached_st* mmc,
                                         uint32_t initial, uint32_t max)
{
  memcached_pool_st* ret = NULL;
  memcached_pool_st object = { .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .master = mmc,
    .mmc = calloc(max, sizeof(memcached_st*)),
    .firstfree = -1,
    .size = max,
    .current_size = 0 };

  if (object.mmc != NULL)
  {
    ret= calloc(1, sizeof(*ret));
    if (ret == NULL)
    {
      free(object.mmc);
      return NULL;
    }

    *ret = object;

    /*
      Try to create the initial size of the pool. An allocation failure at
      this time is not fatal..
    */
    for (unsigned int ii= 0; ii < initial; ++ii)
    {
      if (grow_pool(ret) == -1)
        break;
    }
  }

  return ret;
}

memcached_st*  memcached_pool_destroy(memcached_pool_st* pool)
{
  memcached_st *ret = pool->master;

  for (int xx= 0; xx <= pool->firstfree; ++xx)
  {
    memcached_free(pool->mmc[xx]);
    free(pool->mmc[xx]);
    pool->mmc[xx] = NULL;
  }

  pthread_mutex_destroy(&pool->mutex);
  pthread_cond_destroy(&pool->cond);
  free(pool->mmc);
  free(pool);

  return ret;
}

memcached_st* memcached_pool_pop(memcached_pool_st* pool,
                                 bool block,
                                 memcached_return_t *rc)
{
  memcached_st *ret= NULL;
  if ((*rc= mutex_enter(&pool->mutex)) != MEMCACHED_SUCCESS)
    return NULL;

  do
  {
    if (pool->firstfree > -1)
      ret= pool->mmc[pool->firstfree--];
    else if (pool->current_size == pool->size)
    {
      if (!block)
      {
        *rc= mutex_exit(&pool->mutex);
        return NULL;
      }

      if (pthread_cond_wait(&pool->cond, &pool->mutex) == -1)
      {
        int err = errno;
        mutex_exit(&pool->mutex);
        errno = err;
        *rc= MEMCACHED_ERRNO;
        return NULL;
      }
    }
    else if (grow_pool(pool) == -1)
    {
      *rc= mutex_exit(&pool->mutex);
      return NULL;
    }
  }
  while (ret == NULL);

  *rc= mutex_exit(&pool->mutex);

  return ret;
}

memcached_return_t memcached_pool_push(memcached_pool_st* pool,
                                       memcached_st *mmc)
{
  memcached_return_t rc= mutex_enter(&pool->mutex);

  if (rc != MEMCACHED_SUCCESS)
    return rc;

  char* version= memcached_get_user_data(mmc);
  /* Someone updated the behavior on the object.. */
  if (version != pool->version)
  {
    memcached_free(mmc);
    memset(mmc, 0, sizeof(*mmc));
    if (memcached_clone(mmc, pool->master) == NULL)
    {
      rc= MEMCACHED_SOME_ERRORS;
    }
  }

  pool->mmc[++pool->firstfree]= mmc;

  if (pool->firstfree == 0 && pool->current_size == pool->size)
  {
    /* we might have people waiting for a connection.. wake them up :-) */
    pthread_cond_broadcast(&pool->cond);
  }

  memcached_return_t rval= mutex_exit(&pool->mutex);
  if (rc == MEMCACHED_SOME_ERRORS)
    return rc;

  return rval;
}


memcached_return_t memcached_pool_behavior_set(memcached_pool_st *pool,
                                               memcached_behavior_t flag,
                                               uint64_t data)
{

  memcached_return_t rc= mutex_enter(&pool->mutex);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* update the master */
  rc= memcached_behavior_set(pool->master, flag, data);
  if (rc != MEMCACHED_SUCCESS)
  {
    mutex_exit(&pool->mutex);
    return rc;
  }

  ++pool->version;
  memcached_set_user_data(pool->master, pool->version);
  /* update the clones */
  for (int xx= 0; xx <= pool->firstfree; ++xx)
  {
    rc= memcached_behavior_set(pool->mmc[xx], flag, data);
    if (rc == MEMCACHED_SUCCESS)
      memcached_set_user_data(pool->mmc[xx], pool->version);
    else
    {
      memcached_free(pool->mmc[xx]);
      memset(pool->mmc[xx], 0, sizeof(*pool->mmc[xx]));
      if (memcached_clone(pool->mmc[xx], pool->master) == NULL)
      {
        /* I'm not sure what to do in this case.. this would happen
          if we fail to push the server list inside the client..
          I should add a testcase for this, but I believe the following
          would work, except that you would add a hole in the pool list..
          in theory you could end up with an empty pool....
        */
        free(pool->mmc[xx]);
        pool->mmc[xx]= NULL;
      }
    }
  }

  return mutex_exit(&pool->mutex);
}

memcached_return_t memcached_pool_behavior_get(memcached_pool_st *pool,
                                               memcached_behavior_t flag,
                                               uint64_t *value)
{
  memcached_return_t rc= mutex_enter(&pool->mutex);

  if (rc != MEMCACHED_SUCCESS)
  {
    return rc;
  }

  *value= memcached_behavior_get(pool->master, flag);

  return mutex_exit(&pool->mutex);
}
