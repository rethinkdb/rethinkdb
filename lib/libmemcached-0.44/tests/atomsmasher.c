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
  Sample test application.
*/
#include "config.h"

#include "libmemcached/memcached.h"
#include "libmemcached/watchpoint.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "server.h"
#include "../clients/generator.h"
#include "../clients/execute.h"

#include "test.h"

/* Number of items generated for tests */
#define GLOBAL_COUNT 100000

/* Number of times to run the test loop */
#define TEST_COUNTER 500000
static uint32_t global_count;

static pairs_st *global_pairs;
static char *global_keys[GLOBAL_COUNT];
static size_t global_keys_length[GLOBAL_COUNT];

static test_return_t cleanup_pairs(memcached_st *memc __attribute__((unused)))
{
  pairs_free(global_pairs);

  return 0;
}

static test_return_t generate_pairs(memcached_st *memc __attribute__((unused)))
{
  global_pairs= pairs_generate(GLOBAL_COUNT, 400);
  global_count= GLOBAL_COUNT;

  for (size_t x= 0; x < global_count; x++)
  {
    global_keys[x]= global_pairs[x].key; 
    global_keys_length[x]=  global_pairs[x].key_length;
  }

  return 0;
}

static test_return_t drizzle(memcached_st *memc)
{
  memcached_return_t rc;
  char *return_value;
  size_t return_value_length;
  uint32_t flags;

infinite:
  for (size_t x= 0; x < TEST_COUNTER; x++)
  {
    uint32_t test_bit;
    uint8_t which;

    test_bit= (uint32_t)(random() % GLOBAL_COUNT);
    which= (uint8_t)(random() % 2);

    if (which == 0)
    {
      return_value= memcached_get(memc, global_keys[test_bit], global_keys_length[test_bit],
                                  &return_value_length, &flags, &rc);
      if (rc == MEMCACHED_SUCCESS && return_value)
      {
        free(return_value);
      }
      else if (rc == MEMCACHED_NOTFOUND)
      {
        continue;
      }
      else
      {
        WATCHPOINT_ERROR(rc);
        WATCHPOINT_ASSERT(rc);
      }
    } 
    else
    {
      rc= memcached_set(memc, global_pairs[test_bit].key, 
                        global_pairs[test_bit].key_length,
                        global_pairs[test_bit].value, 
                        global_pairs[test_bit].value_length,
                        0, 0);
      if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_BUFFERED)
      {
        WATCHPOINT_ERROR(rc);
        WATCHPOINT_ASSERT(0);
      }
    }
  }

  if (getenv("MEMCACHED_ATOM_BURIN_IN"))
    goto infinite;

  return TEST_SUCCESS;
}

static test_return_t pre_nonblock(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);

  return TEST_SUCCESS;
}

/* 
  Set the value, then quit to make sure it is flushed.
  Come back in and test that add fails.
*/
static test_return_t add_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "when we sanitize";
  unsigned long long setting_value;

  setting_value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NO_BLOCK);

  rc= memcached_set(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  memcached_quit(memc);
  rc= memcached_add(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);

  /* Too many broken OS'es have broken loopback in async, so we can't be sure of the result */
  if (setting_value)
  {
    test_true(rc == MEMCACHED_NOTSTORED || rc == MEMCACHED_STORED);
  }
  else
  {
    test_true(rc == MEMCACHED_NOTSTORED);
  }

  return 0;
}

/*
 * repeating add_tests many times
 * may show a problem in timing
 */
static test_return_t many_adds(memcached_st *memc)
{
  for (size_t x= 0; x < TEST_COUNTER; x++)
  {
    add_test(memc);
  }
  return 0;
}

test_st smash_tests[] ={
  {"generate_pairs", 1, (test_callback_fn)generate_pairs },
  {"drizzle", 1, (test_callback_fn)drizzle },
  {"cleanup", 1, (test_callback_fn)cleanup_pairs },
  {"many_adds", 1, (test_callback_fn)many_adds },
  {0, 0, 0}
};

#define BENCHMARK_TEST_LOOP 20000

struct benchmark_state_st
{
  bool create_init;
  bool clone_init;
  memcached_st *create;
  memcached_st *clone;
} benchmark_state;

static test_return_t memcached_create_benchmark(memcached_st *memc __attribute__((unused)))
{
  benchmark_state.create_init= true;

  for (size_t x= 0; x < BENCHMARK_TEST_LOOP; x++)
  {
    memcached_st *ptr;
    ptr= memcached_create(&benchmark_state.create[x]);

    test_true(ptr);
  }

  return TEST_SUCCESS;
}

static test_return_t memcached_clone_benchmark(memcached_st *memc)
{
  benchmark_state.clone_init= true;

  for (size_t x= 0; x < BENCHMARK_TEST_LOOP; x++)
  {
    memcached_st *ptr;
    ptr= memcached_clone(&benchmark_state.clone[x], memc);

    test_true(ptr);
  }

  return TEST_SUCCESS;
}

static test_return_t pre_allocate(memcached_st *memc __attribute__((unused)))
{
  memset(&benchmark_state, 0, sizeof(benchmark_state));

  benchmark_state.create= (memcached_st *)calloc(BENCHMARK_TEST_LOOP, sizeof(memcached_st));
  test_true(benchmark_state.create);
  benchmark_state.clone= (memcached_st *)calloc(BENCHMARK_TEST_LOOP, sizeof(memcached_st));
  test_true(benchmark_state.clone);

  return TEST_SUCCESS;
}

static test_return_t post_allocate(memcached_st *memc __attribute__((unused)))
{
  for (size_t x= 0; x < BENCHMARK_TEST_LOOP; x++)
  {
    if (benchmark_state.create_init)
      memcached_free(&benchmark_state.create[x]);

    if (benchmark_state.clone_init)
      memcached_free(&benchmark_state.clone[x]);
  }

  free(benchmark_state.create);
  free(benchmark_state.clone);

  return TEST_SUCCESS;
}


test_st micro_tests[] ={
  {"memcached_create", 1, (test_callback_fn)memcached_create_benchmark },
  {"memcached_clone", 1, (test_callback_fn)memcached_clone_benchmark },
  {0, 0, 0}
};


collection_st collection[] ={
  {"smash", 0, 0, smash_tests},
  {"smash_nonblock", (test_callback_fn)pre_nonblock, 0, smash_tests},
  {"micro-benchmark", (test_callback_fn)pre_allocate, (test_callback_fn)post_allocate, micro_tests},
  {0, 0, 0, 0}
};


#define SERVERS_TO_CREATE 5

#include "libmemcached_world.h"

void get_world(world_st *world)
{
  world->collections= collection;

  world->create= (test_callback_create_fn)world_create;
  world->destroy= (test_callback_fn)world_destroy;

  world->test.startup= (test_callback_fn)world_test_startup;
  world->test.flush= (test_callback_fn)world_flush;
  world->test.pre_run= (test_callback_fn)world_pre_run;
  world->test.post_run= (test_callback_fn)world_post_run;
  world->test.on_error= (test_callback_error_fn)world_on_error;

  world->collection.startup= (test_callback_fn)world_container_startup;
  world->collection.shutdown= (test_callback_fn)world_container_shutdown;

  world->runner= &defualt_libmemcached_runner;
}
