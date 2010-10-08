/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 */

/*
  Execute a memcached_set() a set of pairs.
  Return the number of rows set.
*/

#include "config.h"
#include "execute.h"

unsigned int execute_set(memcached_st *memc, pairs_st *pairs, unsigned int number_of)
{
  memcached_return_t rc;
  unsigned int x;
  unsigned int pairs_sent;

  for (x= 0, pairs_sent= 0; x < number_of; x++)
  {
    rc= memcached_set(memc, pairs[x].key, pairs[x].key_length,
                      pairs[x].value, pairs[x].value_length,
                      0, 0);
    if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_BUFFERED)
      fprintf(stderr, "Failured on insert of %.*s\n",
              (unsigned int)pairs[x].key_length, pairs[x].key);
    else
      pairs_sent++;
  }

  return pairs_sent;
}

/*
  Execute a memcached_get() on a set of pairs.
  Return the number of rows retrieved.
*/
unsigned int execute_get(memcached_st *memc, pairs_st *pairs, unsigned int number_of)
{
  memcached_return_t rc;
  unsigned int x;
  unsigned int retrieved;


  for (retrieved= 0,x= 0; x < number_of; x++)
  {
    char *value;
    size_t value_length;
    uint32_t flags;
    unsigned int fetch_key;

    fetch_key= (unsigned int)((unsigned int)random() % number_of);

    value= memcached_get(memc, pairs[fetch_key].key, pairs[fetch_key].key_length,
                         &value_length, &flags, &rc);

    if (rc != MEMCACHED_SUCCESS)
      fprintf(stderr, "Failured on read of %.*s\n",
              (unsigned int)pairs[fetch_key].key_length, pairs[fetch_key].key);
    else
      retrieved++;

    free(value);
  }

  return retrieved;
}

/**
 * Callback function to count the number of results
 */
static memcached_return_t callback_counter(const memcached_st *ptr,
                                           memcached_result_st *result,
                                           void *context)
{
  (void)ptr;
  (void)result;
  unsigned int *counter= (unsigned int *)context;
  *counter= *counter + 1;

  return MEMCACHED_SUCCESS;
}

/**
 * Try to run a large mget to get all of the keys
 * @param memc memcached handle
 * @param keys the keys to get
 * @param key_length the length of the keys
 * @param number_of the number of keys to try to get
 * @return the number of keys received
 */
unsigned int execute_mget(memcached_st *memc,
                          const char * const *keys,
                          size_t *key_length,
                          unsigned int number_of)
{
  unsigned int retrieved= 0;
  memcached_execute_fn callbacks[1]= { [0]= &callback_counter };
  memcached_return_t rc;
  rc= memcached_mget_execute(memc, keys, key_length,
                             (size_t)number_of, callbacks, &retrieved, 1);

  if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_NOTFOUND ||
          rc == MEMCACHED_BUFFERED || rc == MEMCACHED_END)
  {
    rc= memcached_fetch_execute(memc, callbacks, (void *)&retrieved, 1);
    if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_NOTFOUND && rc != MEMCACHED_END)
    {
      fprintf(stderr, "Failed to execute mget: %s\n",
              memcached_strerror(memc, rc));
      memcached_quit(memc);
      return 0;
    }
  }
  else
  {
    fprintf(stderr, "Failed to execute mget: %s\n",
            memcached_strerror(memc, rc));
    memcached_quit(memc);
    return 0;
  }

  return retrieved;
}
