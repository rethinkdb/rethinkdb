/* uTest
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

/*
  Sample test application.
*/

#include "config.h"

#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include "libmemcached/memcached.h"

#include "test.h"

static void world_stats_print(world_stats_st *stats)
{
  fputc('\n', stderr);
  fprintf(stderr, "Total Collections\t\t\t\t%u\n", stats->collection_total);
  fprintf(stderr, "\tFailed Collections\t\t\t%u\n", stats->collection_failed);
  fprintf(stderr, "\tSkipped Collections\t\t\t%u\n", stats->collection_skipped);
  fprintf(stderr, "\tSucceeded Collections\t\t%u\n", stats->collection_success);
  fputc('\n', stderr);
  fprintf(stderr, "Total\t\t\t\t%u\n", stats->total);
  fprintf(stderr, "\tFailed\t\t\t%u\n", stats->failed);
  fprintf(stderr, "\tSkipped\t\t\t%u\n", stats->skipped);
  fprintf(stderr, "\tSucceeded\t\t%u\n", stats->success);
}

long int timedif(struct timeval a, struct timeval b)
{
  long us, s;

  us = (int)(a.tv_usec - b.tv_usec);
  us /= 1000;
  s = (int)(a.tv_sec - b.tv_sec);
  s *= 1000;
  return s + us;
}

const char *test_strerror(test_return_t code)
{
  switch (code) {
  case TEST_SUCCESS:
    return "ok";
  case TEST_FAILURE:
    return "failed";
  case TEST_MEMORY_ALLOCATION_FAILURE:
    return "memory allocation";
  case TEST_SKIPPED:
    return "skipped";
  case TEST_MAXIMUM_RETURN:
  default:
    fprintf(stderr, "Unknown return value\n");
    abort();
  }
}

void create_core(void)
{
  if (getenv("LIBMEMCACHED_NO_COREDUMP") == NULL)
  {
    pid_t pid= fork();

    if (pid == 0)
    {
      abort();
    }
    else
    {
      while (waitpid(pid, NULL, 0) != pid)
      {
        ;
      }
    }
  }
}


static test_return_t _runner_default(test_callback_fn func, void *p)
{
  if (func)
  {
    return func(p);
  }
  else
  {
    return TEST_SUCCESS;
  }
}

static world_runner_st defualt_runners= {
  _runner_default,
  _runner_default,
  _runner_default
};

static test_return_t _default_callback(void *p)
{
  (void)p;

  return TEST_SUCCESS;
}

static inline void set_default_fn(test_callback_fn *fn)
{
  if (*fn == NULL)
  {
    *fn= _default_callback;
  }
}

static collection_st *init_world(world_st *world)
{
  if (! world->runner)
  {
    world->runner= &defualt_runners;
  }

  set_default_fn(&world->collection.startup);
  set_default_fn(&world->collection.shutdown);

  return world->collections;
}


int main(int argc, char *argv[])
{
  test_return_t return_code;
  unsigned int x;
  char *collection_to_run= NULL;
  char *wildcard= NULL;
  world_st world;
  collection_st *collection;
  collection_st *next;
  void *world_ptr;

  world_stats_st stats;

#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
  if (sasl_client_init(NULL) != SASL_OK)
  {
     fprintf(stderr, "Failed to initialize sasl library!\n");
     return 1;
  }
#endif

  memset(&stats, 0, sizeof(stats));
  memset(&world, 0, sizeof(world));
  get_world(&world);

  collection= init_world(&world);

  if (world.create)
  {
    test_return_t error;
    world_ptr= world.create(&error);
    if (error != TEST_SUCCESS)
      exit(1);
  }
  else
  {
    world_ptr= NULL;
  }

  if (argc > 1)
    collection_to_run= argv[1];

  if (argc == 3)
    wildcard= argv[2];

  for (next= collection; next->name; next++)
  {
    test_return_t collection_rc= TEST_SUCCESS;
    test_st *run;
    bool failed= false;
    bool skipped= false;

    run= next->tests;
    if (collection_to_run && fnmatch(collection_to_run, next->name, 0))
      continue;

    stats.collection_total++;

    collection_rc= world.collection.startup(world_ptr);

    if (collection_rc != TEST_SUCCESS)
      goto skip_pre;

    if (next->pre)
    {
      collection_rc= world.runner->pre(next->pre, world_ptr);
    }

skip_pre:
    switch (collection_rc)
    {
      case TEST_SUCCESS:
        fprintf(stderr, "\n%s\n\n", next->name);
        break;
      case TEST_FAILURE:
        fprintf(stderr, "\n%s [ failed ]\n\n", next->name);
        stats.collection_failed++;
        goto cleanup;
      case TEST_SKIPPED:
        fprintf(stderr, "\n%s [ skipping ]\n\n", next->name);
        stats.collection_skipped++;
        goto cleanup;
      case TEST_MEMORY_ALLOCATION_FAILURE:
      case TEST_MAXIMUM_RETURN:
      default:
        assert(0);
        break;
    }


    for (x= 0; run->name; run++)
    {
      struct timeval start_time, end_time;
      long int load_time= 0;

      if (wildcard && fnmatch(wildcard, run->name, 0))
        continue;

      fprintf(stderr, "Testing %s", run->name);

      if (world.test.startup)
      {
        world.test.startup(world_ptr);
      }

      if (run->requires_flush && world.test.flush)
      {
        world.test.flush(world_ptr);
      }

      if (world.test.pre_run)
      {
        world.test.pre_run(world_ptr);
      }


      // Runner code
      {
#if 0
        if (next->pre && world.runner->pre)
        {
          return_code= world.runner->pre(next->pre, world_ptr);

          if (return_code != TEST_SUCCESS)
          {
            goto error;
          }
        }
#endif

        gettimeofday(&start_time, NULL);
        return_code= world.runner->run(run->test_fn, world_ptr);
        gettimeofday(&end_time, NULL);
        load_time= timedif(end_time, start_time);

#if 0
        if (next->post && world.runner->post)
        {
          (void) world.runner->post(next->post, world_ptr);
        }
#endif
      }

      if (world.test.post_run)
      {
        world.test.post_run(world_ptr);
      }

      stats.total++;

      fprintf(stderr, "\t\t\t\t\t");

      switch (return_code)
      {
      case TEST_SUCCESS:
        fprintf(stderr, "%ld.%03ld ", load_time / 1000, load_time % 1000);
        stats.success++;
        break;
      case TEST_FAILURE:
        stats.failed++;
        failed= true;
        break;
      case TEST_SKIPPED:
        stats.skipped++;
        skipped= true;
        break;
      case TEST_MEMORY_ALLOCATION_FAILURE:
        fprintf(stderr, "Exhausted memory, quitting\n");
        abort();
      case TEST_MAXIMUM_RETURN:
      default:
        assert(0); // Coding error.
        break;
      }

      fprintf(stderr, "[ %s ]\n", test_strerror(return_code));

      if (world.test.on_error)
      {
        test_return_t rc;
        rc= world.test.on_error(return_code, world_ptr);

        if (rc != TEST_SUCCESS)
          break;
      }
    }

    if (next->post && world.runner->post)
    {
      (void) world.runner->post(next->post, world_ptr);
    }

    if (! failed && ! skipped)
    {
      stats.collection_success++;
    }
cleanup:

    world.collection.shutdown(world_ptr);
  }

  if (stats.collection_failed || stats.collection_skipped)
  {
    fprintf(stderr, "Some test failures and/or skipped test occurred.\n\n");
  }
  else
  {
    fprintf(stderr, "All tests completed successfully\n\n");
  }

  if (world.destroy)
  {
    test_return_t error;
    error= world.destroy(world_ptr);

    if (error != TEST_SUCCESS)
    {
      fprintf(stderr, "Failure during shutdown.\n");
      stats.failed++; // We do this to make our exit code return 1
    }
  }

  world_stats_print(&stats);

#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
  sasl_done();
#endif

  return stats.failed == 0 ? 0 : 1;
}
