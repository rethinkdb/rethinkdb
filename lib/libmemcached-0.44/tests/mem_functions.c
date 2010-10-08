/* libMemcached Functions Test
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

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "libmemcached/common.h"

#include "server.h"
#include "clients/generator.h"
#include "clients/execute.h"

#define SMALL_STRING_LEN 1024

#include "test.h"


#ifdef HAVE_LIBMEMCACHEDUTIL
#include <pthread.h>
#include "libmemcached/memcached_util.h"
#endif

#include "hash_results.h"

#define GLOBAL_COUNT 10000
#define GLOBAL2_COUNT 100
#define SERVERS_TO_CREATE 5
static uint32_t global_count;

static pairs_st *global_pairs;
static const char *global_keys[GLOBAL_COUNT];
static size_t global_keys_length[GLOBAL_COUNT];

// Prototype
static test_return_t pre_binary(memcached_st *memc);


static test_return_t init_test(memcached_st *not_used __attribute__((unused)))
{
  memcached_st memc;

  (void)memcached_create(&memc);
  memcached_free(&memc);

  return TEST_SUCCESS;
}

static test_return_t server_list_null_test(memcached_st *ptr __attribute__((unused)))
{
  memcached_server_st *server_list;
  memcached_return_t rc;

  server_list= memcached_server_list_append_with_weight(NULL, NULL, 0, 0, NULL);
  test_true(server_list == NULL);

  server_list= memcached_server_list_append_with_weight(NULL, "localhost", 0, 0, NULL);
  test_true(server_list == NULL);

  server_list= memcached_server_list_append_with_weight(NULL, NULL, 0, 0, &rc);
  test_true(server_list == NULL);

  return TEST_SUCCESS;
}

#define TEST_PORT_COUNT 7
in_port_t test_ports[TEST_PORT_COUNT];

static memcached_return_t  server_display_function(const memcached_st *ptr __attribute__((unused)),
                                                   const memcached_server_st *server,
                                                   void *context)
{
  /* Do Nothing */
  size_t bigger= *((size_t *)(context));
  assert(bigger <= memcached_server_port(server));
  *((size_t *)(context))= memcached_server_port(server);

  return MEMCACHED_SUCCESS;
}

static test_return_t server_sort_test(memcached_st *ptr __attribute__((unused)))
{
  size_t bigger= 0; /* Prime the value for the test_true in server_display_function */

  memcached_return_t rc;
  memcached_server_fn callbacks[1];
  memcached_st *local_memc;

  local_memc= memcached_create(NULL);
  test_true(local_memc);
  memcached_behavior_set(local_memc, MEMCACHED_BEHAVIOR_SORT_HOSTS, 1);

  for (size_t x= 0; x < TEST_PORT_COUNT; x++)
  {
    test_ports[x]= (in_port_t)random() % 64000;
    rc= memcached_server_add_with_weight(local_memc, "localhost", test_ports[x], 0);
    test_true(memcached_server_count(local_memc) == x + 1);
#if 0 // Rewrite
    test_true(memcached_server_list_count(memcached_server_list(local_memc)) == x+1);
#endif
    test_true(rc == MEMCACHED_SUCCESS);
  }

  callbacks[0]= server_display_function;
  memcached_server_cursor(local_memc, callbacks, (void *)&bigger,  1);


  memcached_free(local_memc);

  return TEST_SUCCESS;
}

static test_return_t server_sort2_test(memcached_st *ptr __attribute__((unused)))
{
  size_t bigger= 0; /* Prime the value for the test_true in server_display_function */
  memcached_return_t rc;
  memcached_server_fn callbacks[1];
  memcached_st *local_memc;
  memcached_server_instance_st instance;

  local_memc= memcached_create(NULL);
  test_true(local_memc);
  rc= memcached_behavior_set(local_memc, MEMCACHED_BEHAVIOR_SORT_HOSTS, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_server_add_with_weight(local_memc, "MEMCACHED_BEHAVIOR_SORT_HOSTS", 43043, 0);
  test_true(rc == MEMCACHED_SUCCESS);
  instance= memcached_server_instance_by_position(local_memc, 0);
  test_true(memcached_server_port(instance) == 43043);

  rc= memcached_server_add_with_weight(local_memc, "MEMCACHED_BEHAVIOR_SORT_HOSTS", 43042, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  instance= memcached_server_instance_by_position(local_memc, 0);
  test_true(memcached_server_port(instance) == 43042);

  instance= memcached_server_instance_by_position(local_memc, 1);
  test_true(memcached_server_port(instance) == 43043);

  callbacks[0]= server_display_function;
  memcached_server_cursor(local_memc, callbacks, (void *)&bigger,  1);


  memcached_free(local_memc);

  return TEST_SUCCESS;
}

static memcached_return_t server_print_callback(const memcached_st *ptr __attribute__((unused)),
                                                const memcached_server_st *server,
                                                void *context __attribute__((unused)))
{
  (void)server; // Just in case we aren't printing.

#if 0
  fprintf(stderr, "%s(%d)", memcached_server_name(server), memcached_server_port(server));
#endif

  return MEMCACHED_SUCCESS;
}

static test_return_t memcached_server_remove_test(memcached_st *ptr __attribute__((unused)))
{
  memcached_return_t rc;
  memcached_st local_memc;
  memcached_st *memc;
  memcached_server_st *servers;
  memcached_server_fn callbacks[1];

  const char *server_string= "localhost:4444, localhost:4445, localhost:4446, localhost:4447, localhost, memcache1.memcache.bk.sapo.pt:11211, memcache1.memcache.bk.sapo.pt:11212, memcache1.memcache.bk.sapo.pt:11213, memcache1.memcache.bk.sapo.pt:11214, memcache2.memcache.bk.sapo.pt:11211, memcache2.memcache.bk.sapo.pt:11212, memcache2.memcache.bk.sapo.pt:11213, memcache2.memcache.bk.sapo.pt:11214";

  memc= memcached_create(&local_memc);

  servers= memcached_servers_parse(server_string);

  rc= memcached_server_push(memc, servers);
  memcached_server_list_free(servers);

  callbacks[0]= server_print_callback;
  memcached_server_cursor(memc, callbacks, NULL,  1);

  memcached_free(memc);

  return TEST_SUCCESS;
}

static memcached_return_t server_display_unsort_function(const memcached_st *ptr __attribute__((unused)),
                                                         const memcached_server_st *server,
                                                         void *context)
{
  /* Do Nothing */
  uint32_t x= *((uint32_t *)(context));

  assert(test_ports[x] == server->port);
  *((uint32_t *)(context))= ++x;

  return MEMCACHED_SUCCESS;
}

static test_return_t server_unsort_test(memcached_st *ptr __attribute__((unused)))
{
  size_t counter= 0; /* Prime the value for the test_true in server_display_function */
  size_t bigger= 0; /* Prime the value for the test_true in server_display_function */
  memcached_return_t rc;
  memcached_server_fn callbacks[1];
  memcached_st *local_memc;

  local_memc= memcached_create(NULL);
  test_true(local_memc);

  for (size_t x= 0; x < TEST_PORT_COUNT; x++)
  {
    test_ports[x]= (in_port_t)(random() % 64000);
    rc= memcached_server_add_with_weight(local_memc, "localhost", test_ports[x], 0);
    test_true(memcached_server_count(local_memc) == x+1);
#if 0 // Rewrite
    test_true(memcached_server_list_count(memcached_server_list(local_memc)) == x+1);
#endif
    test_true(rc == MEMCACHED_SUCCESS);
  }

  callbacks[0]= server_display_unsort_function;
  memcached_server_cursor(local_memc, callbacks, (void *)&counter,  1);

  /* Now we sort old data! */
  memcached_behavior_set(local_memc, MEMCACHED_BEHAVIOR_SORT_HOSTS, 1);
  callbacks[0]= server_display_function;
  memcached_server_cursor(local_memc, callbacks, (void *)&bigger,  1);


  memcached_free(local_memc);

  return TEST_SUCCESS;
}

static test_return_t allocation_test(memcached_st *not_used __attribute__((unused)))
{
  memcached_st *memc;
  memc= memcached_create(NULL);
  test_true(memc);
  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t clone_test(memcached_st *memc)
{
  /* All null? */
  {
    memcached_st *memc_clone;
    memc_clone= memcached_clone(NULL, NULL);
    test_true(memc_clone);
    memcached_free(memc_clone);
  }

  /* Can we init from null? */
  {
    memcached_st *memc_clone;
    memc_clone= memcached_clone(NULL, memc);
    test_true(memc_clone);

    { // Test allocators
      test_true(memc_clone->allocators.free == memc->allocators.free);
      test_true(memc_clone->allocators.malloc == memc->allocators.malloc);
      test_true(memc_clone->allocators.realloc == memc->allocators.realloc);
      test_true(memc_clone->allocators.calloc == memc->allocators.calloc);
    }

    test_true(memc_clone->connect_timeout == memc->connect_timeout);
    test_true(memc_clone->delete_trigger == memc->delete_trigger);
    test_true(memc_clone->distribution == memc->distribution);
    { // Test all of the flags
      test_true(memc_clone->flags.no_block == memc->flags.no_block);
      test_true(memc_clone->flags.tcp_nodelay == memc->flags.tcp_nodelay);
      test_true(memc_clone->flags.reuse_memory == memc->flags.reuse_memory);
      test_true(memc_clone->flags.use_cache_lookups == memc->flags.use_cache_lookups);
      test_true(memc_clone->flags.support_cas == memc->flags.support_cas);
      test_true(memc_clone->flags.buffer_requests == memc->flags.buffer_requests);
      test_true(memc_clone->flags.use_sort_hosts == memc->flags.use_sort_hosts);
      test_true(memc_clone->flags.verify_key == memc->flags.verify_key);
      test_true(memc_clone->flags.ketama_weighted == memc->flags.ketama_weighted);
      test_true(memc_clone->flags.binary_protocol == memc->flags.binary_protocol);
      test_true(memc_clone->flags.hash_with_prefix_key == memc->flags.hash_with_prefix_key);
      test_true(memc_clone->flags.no_reply == memc->flags.no_reply);
      test_true(memc_clone->flags.use_udp == memc->flags.use_udp);
      test_true(memc_clone->flags.auto_eject_hosts == memc->flags.auto_eject_hosts);
      test_true(memc_clone->flags.randomize_replica_read == memc->flags.randomize_replica_read);
    }
    test_true(memc_clone->get_key_failure == memc->get_key_failure);
    test_true(hashkit_compare(&memc_clone->hashkit, &memc->hashkit));
    test_true(hashkit_compare(&memc_clone->distribution_hashkit, &memc->distribution_hashkit));
    test_true(memc_clone->io_bytes_watermark == memc->io_bytes_watermark);
    test_true(memc_clone->io_msg_watermark == memc->io_msg_watermark);
    test_true(memc_clone->io_key_prefetch == memc->io_key_prefetch);
    test_true(memc_clone->on_cleanup == memc->on_cleanup);
    test_true(memc_clone->on_clone == memc->on_clone);
    test_true(memc_clone->poll_timeout == memc->poll_timeout);
    test_true(memc_clone->rcv_timeout == memc->rcv_timeout);
    test_true(memc_clone->recv_size == memc->recv_size);
    test_true(memc_clone->retry_timeout == memc->retry_timeout);
    test_true(memc_clone->send_size == memc->send_size);
    test_true(memc_clone->server_failure_limit == memc->server_failure_limit);
    test_true(memc_clone->snd_timeout == memc->snd_timeout);
    test_true(memc_clone->user_data == memc->user_data);

    memcached_free(memc_clone);
  }

  /* Can we init from struct? */
  {
    memcached_st declared_clone;
    memcached_st *memc_clone;
    memset(&declared_clone, 0 , sizeof(memcached_st));
    memc_clone= memcached_clone(&declared_clone, NULL);
    test_true(memc_clone);
    memcached_free(memc_clone);
  }

  /* Can we init from struct? */
  {
    memcached_st declared_clone;
    memcached_st *memc_clone;
    memset(&declared_clone, 0 , sizeof(memcached_st));
    memc_clone= memcached_clone(&declared_clone, memc);
    test_true(memc_clone);
    memcached_free(memc_clone);
  }

  return TEST_SUCCESS;
}

static test_return_t userdata_test(memcached_st *memc)
{
  void* foo= NULL;
  test_true(memcached_set_user_data(memc, foo) == NULL);
  test_true(memcached_get_user_data(memc) == foo);
  test_true(memcached_set_user_data(memc, NULL) == foo);

  return TEST_SUCCESS;
}

static test_return_t connection_test(memcached_st *memc)
{
  memcached_return_t rc;

  rc= memcached_server_add_with_weight(memc, "localhost", 0, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  return TEST_SUCCESS;
}

static test_return_t error_test(memcached_st *memc)
{
  memcached_return_t rc;
  uint32_t values[] = { 851992627U, 2337886783U, 3196981036U, 4001849190U,
                        982370485U, 1263635348U, 4242906218U, 3829656100U,
                        1891735253U, 334139633U, 2257084983U, 3088286104U,
                        13199785U, 2542027183U, 1097051614U, 199566778U,
                        2748246961U, 2465192557U, 1664094137U, 2405439045U,
                        1842224848U, 692413798U, 3479807801U, 919913813U,
                        4269430871U, 610793021U, 527273862U, 1437122909U,
                        2300930706U, 2943759320U, 674306647U, 2400528935U,
                        54481931U, 4186304426U, 1741088401U, 2979625118U,
                        4159057246U, 3425930182U, 2593724503U,  1868899624U,
                        1769812374U, 2302537950U, 1110330676U };

  // You have updated the memcache_error messages but not updated docs/tests.
  test_true(MEMCACHED_MAXIMUM_RETURN == 43);
  for (rc= MEMCACHED_SUCCESS; rc < MEMCACHED_MAXIMUM_RETURN; rc++)
  {
    uint32_t hash_val;
    const char *msg=  memcached_strerror(memc, rc);
    hash_val= memcached_generate_hash_value(msg, strlen(msg),
                                            MEMCACHED_HASH_JENKINS);
    if (values[rc] != hash_val)
    {
      fprintf(stderr, "\n\nYou have updated memcached_return_t without updating the error_test\n");
      fprintf(stderr, "%u, %s, (%u)\n\n", (uint32_t)rc, memcached_strerror(memc, rc), hash_val);
    }
    test_true(values[rc] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t set_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "when we sanitize";

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  return TEST_SUCCESS;
}

static test_return_t append_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "fig";
  const char *in_value= "we";
  char *out_value= NULL;
  size_t value_length;
  uint32_t flags;

  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_set(memc, key, strlen(key),
                    in_value, strlen(in_value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_append(memc, key, strlen(key),
                       " the", strlen(" the"),
                       (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_append(memc, key, strlen(key),
                       " people", strlen(" people"),
                       (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  out_value= memcached_get(memc, key, strlen(key),
                       &value_length, &flags, &rc);
  test_true(!memcmp(out_value, "we the people", strlen("we the people")));
  test_true(strlen("we the people") == value_length);
  test_true(rc == MEMCACHED_SUCCESS);
  free(out_value);

  return TEST_SUCCESS;
}

static test_return_t append_binary_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "numbers";
  uint32_t store_list[] = { 23, 56, 499, 98, 32847, 0 };
  uint32_t *value;
  size_t value_length;
  uint32_t flags;
  uint32_t x;

  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_set(memc,
                    key, strlen(key),
                    NULL, 0,
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  for (x= 0; store_list[x] ; x++)
  {
    rc= memcached_append(memc,
                         key, strlen(key),
                         (char *)&store_list[x], sizeof(uint32_t),
                         (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  value= (uint32_t *)memcached_get(memc, key, strlen(key),
                       &value_length, &flags, &rc);
  test_true((value_length == (sizeof(uint32_t) * x)));
  test_true(rc == MEMCACHED_SUCCESS);

  for (uint32_t counter= x, *ptr= value; counter; counter--)
  {
    test_true(*ptr == store_list[x - counter]);
    ptr++;
  }
  free(value);

  return TEST_SUCCESS;
}

static test_return_t cas2_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};
  const char *value= "we the people";
  size_t value_length= strlen("we the people");
  unsigned int x;
  memcached_result_st results_obj;
  memcached_result_st *results;
  unsigned int set= 1;

  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, set);

  for (x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  rc= memcached_mget(memc, keys, key_length, 3);

  results= memcached_result_create(memc, &results_obj);

  results= memcached_fetch_result(memc, &results_obj, &rc);
  test_true(results);
  test_true(results->item_cas);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(memcached_result_cas(results));

  test_true(!memcmp(value, "we the people", strlen("we the people")));
  test_true(strlen("we the people") == value_length);
  test_true(rc == MEMCACHED_SUCCESS);

  memcached_result_free(&results_obj);

  return TEST_SUCCESS;
}

static test_return_t cas_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "fun";
  size_t key_length= strlen(key);
  const char *value= "we the people";
  const char* keys[2] = { key, NULL };
  size_t keylengths[2] = { strlen(key), 0 };
  size_t value_length= strlen(value);
  const char *value2= "change the value";
  size_t value2_length= strlen(value2);

  memcached_result_st results_obj;
  memcached_result_st *results;
  unsigned int set= 1;

  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, set);

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_mget(memc, keys, keylengths, 1);

  results= memcached_result_create(memc, &results_obj);

  results= memcached_fetch_result(memc, &results_obj, &rc);
  test_true(results);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(memcached_result_cas(results));
  test_true(!memcmp(value, memcached_result_value(results), value_length));
  test_true(strlen(memcached_result_value(results)) == value_length);
  test_true(rc == MEMCACHED_SUCCESS);
  uint64_t cas = memcached_result_cas(results);

#if 0
  results= memcached_fetch_result(memc, &results_obj, &rc);
  test_true(rc == MEMCACHED_END);
  test_true(results == NULL);
#endif

  rc= memcached_cas(memc, key, key_length, value2, value2_length, 0, 0, cas);
  test_true(rc == MEMCACHED_SUCCESS);

  /*
   * The item will have a new cas value, so try to set it again with the old
   * value. This should fail!
   */
  rc= memcached_cas(memc, key, key_length, value2, value2_length, 0, 0, cas);
  test_true(rc == MEMCACHED_DATA_EXISTS);

  memcached_result_free(&results_obj);

  return TEST_SUCCESS;
}

static test_return_t prepend_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "fig";
  const char *value= "people";
  char *out_value= NULL;
  size_t value_length;
  uint32_t flags;

  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_prepend(memc, key, strlen(key),
                       "the ", strlen("the "),
                       (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_prepend(memc, key, strlen(key),
                       "we ", strlen("we "),
                       (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  out_value= memcached_get(memc, key, strlen(key),
                       &value_length, &flags, &rc);
  test_true(!memcmp(out_value, "we the people", strlen("we the people")));
  test_true(strlen("we the people") == value_length);
  test_true(rc == MEMCACHED_SUCCESS);
  free(out_value);

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
    test_true(rc == MEMCACHED_NOTSTORED || rc == MEMCACHED_DATA_EXISTS);
  }

  return TEST_SUCCESS;
}

/*
** There was a problem of leaking filedescriptors in the initial release
** of MacOSX 10.5. This test case triggers the problem. On some Solaris
** systems it seems that the kernel is slow on reclaiming the resources
** because the connects starts to time out (the test doesn't do much
** anyway, so just loop 10 iterations)
*/
static test_return_t add_wrapper(memcached_st *memc)
{
  unsigned int max= 10000;
#ifdef __sun
  max= 10;
#endif
#ifdef __APPLE__
  max= 10;
#endif

  for (uint32_t x= 0; x < max; x++)
    add_test(memc);

  return TEST_SUCCESS;
}

static test_return_t replace_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "when we sanitize";
  const char *original= "first we insert some data";

  rc= memcached_set(memc, key, strlen(key),
                    original, strlen(original),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  rc= memcached_replace(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);

  return TEST_SUCCESS;
}

static test_return_t delete_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "when we sanitize";

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  rc= memcached_delete(memc, key, strlen(key), (time_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  return TEST_SUCCESS;
}

static test_return_t flush_test(memcached_st *memc)
{
  memcached_return_t rc;

  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  return TEST_SUCCESS;
}

static memcached_return_t  server_function(const memcached_st *ptr __attribute__((unused)),
                                           const memcached_server_st *server __attribute__((unused)),
                                           void *context __attribute__((unused)))
{
  /* Do Nothing */

  return MEMCACHED_SUCCESS;
}

static test_return_t memcached_server_cursor_test(memcached_st *memc)
{
  char context[8];
  strcpy(context, "foo bad");
  memcached_server_fn callbacks[1];

  callbacks[0]= server_function;
  memcached_server_cursor(memc, callbacks, context,  1);
  return TEST_SUCCESS;
}

static test_return_t bad_key_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo bad";
  char *string;
  size_t string_length;
  uint32_t flags;
  memcached_st *memc_clone;
  unsigned int set= 1;
  size_t max_keylen= 0xffff;

  // Just skip if we are in binary mode.
  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL))
    return TEST_SKIPPED;

  memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  rc= memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_VERIFY_KEY, set);
  test_true(rc == MEMCACHED_SUCCESS);

  /* All keys are valid in the binary protocol (except for length) */
  if (memcached_behavior_get(memc_clone, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) == 0)
  {
    string= memcached_get(memc_clone, key, strlen(key),
                          &string_length, &flags, &rc);
    test_true(rc == MEMCACHED_BAD_KEY_PROVIDED);
    test_true(string_length ==  0);
    test_true(!string);

    set= 0;
    rc= memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_VERIFY_KEY, set);
    test_true(rc == MEMCACHED_SUCCESS);
    string= memcached_get(memc_clone, key, strlen(key),
                          &string_length, &flags, &rc);
    test_true(rc == MEMCACHED_NOTFOUND);
    test_true(string_length ==  0);
    test_true(!string);

    /* Test multi key for bad keys */
    const char *keys[] = { "GoodKey", "Bad Key", "NotMine" };
    size_t key_lengths[] = { 7, 7, 7 };
    set= 1;
    rc= memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_VERIFY_KEY, set);
    test_true(rc == MEMCACHED_SUCCESS);

    rc= memcached_mget(memc_clone, keys, key_lengths, 3);
    test_true(rc == MEMCACHED_BAD_KEY_PROVIDED);

    rc= memcached_mget_by_key(memc_clone, "foo daddy", 9, keys, key_lengths, 1);
    test_true(rc == MEMCACHED_BAD_KEY_PROVIDED);

    max_keylen= 250;

    /* The following test should be moved to the end of this function when the
       memcached server is updated to allow max size length of the keys in the
       binary protocol
    */
    rc= memcached_callback_set(memc_clone, MEMCACHED_CALLBACK_PREFIX_KEY, NULL);
    test_true(rc == MEMCACHED_SUCCESS);

    char *longkey= malloc(max_keylen + 1);
    if (longkey != NULL)
    {
      memset(longkey, 'a', max_keylen + 1);
      string= memcached_get(memc_clone, longkey, max_keylen,
                            &string_length, &flags, &rc);
      test_true(rc == MEMCACHED_NOTFOUND);
      test_true(string_length ==  0);
      test_true(!string);

      string= memcached_get(memc_clone, longkey, max_keylen + 1,
                            &string_length, &flags, &rc);
      test_true(rc == MEMCACHED_BAD_KEY_PROVIDED);
      test_true(string_length ==  0);
      test_true(!string);

      free(longkey);
    }
  }

  /* Make sure zero length keys are marked as bad */
  set= 1;
  rc= memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_VERIFY_KEY, set);
  test_true(rc == MEMCACHED_SUCCESS);
  string= memcached_get(memc_clone, key, 0,
                        &string_length, &flags, &rc);
  test_true(rc == MEMCACHED_BAD_KEY_PROVIDED);
  test_true(string_length ==  0);
  test_true(!string);

  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

#define READ_THROUGH_VALUE "set for me"
static memcached_return_t read_through_trigger(memcached_st *memc __attribute__((unused)),
                                               char *key __attribute__((unused)),
                                               size_t key_length __attribute__((unused)),
                                               memcached_result_st *result)
{

  return memcached_result_set_value(result, READ_THROUGH_VALUE, strlen(READ_THROUGH_VALUE));
}

static test_return_t read_through(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  char *string;
  size_t string_length;
  uint32_t flags;
  memcached_trigger_key_fn cb= (memcached_trigger_key_fn)read_through_trigger;

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_true(rc == MEMCACHED_NOTFOUND);
  test_false(string_length);
  test_false(string);

  rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_GET_FAILURE,
                             *(void **)&cb);
  test_true(rc == MEMCACHED_SUCCESS);

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_true(rc == MEMCACHED_SUCCESS);
  test_true(string_length ==  strlen(READ_THROUGH_VALUE));
  test_strcmp(READ_THROUGH_VALUE, string);
  free(string);

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_true(rc == MEMCACHED_SUCCESS);
  test_true(string_length ==  strlen(READ_THROUGH_VALUE));
  test_true(!strcmp(READ_THROUGH_VALUE, string));
  free(string);

  return TEST_SUCCESS;
}

static memcached_return_t  delete_trigger(memcached_st *ptr __attribute__((unused)),
                                          const char *key,
                                          size_t key_length __attribute__((unused)))
{
  assert(key);

  return MEMCACHED_SUCCESS;
}

static test_return_t delete_through(memcached_st *memc)
{
  memcached_trigger_delete_key_fn callback;
  memcached_return_t rc;

  callback= (memcached_trigger_delete_key_fn)delete_trigger;

  rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_DELETE_TRIGGER, *(void**)&callback);
  test_true(rc == MEMCACHED_SUCCESS);

  return TEST_SUCCESS;
}

static test_return_t get_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  char *string;
  size_t string_length;
  uint32_t flags;

  rc= memcached_delete(memc, key, strlen(key), (time_t)0);
  test_true(rc == MEMCACHED_BUFFERED || rc == MEMCACHED_NOTFOUND);

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_true(rc == MEMCACHED_NOTFOUND);
  test_false(string_length);
  test_false(string);

  return TEST_SUCCESS;
}

static test_return_t get_test2(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "when we sanitize";
  char *string;
  size_t string_length;
  uint32_t flags;

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_true(string);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(string_length == strlen(value));
  test_true(!memcmp(string, value, string_length));

  free(string);

  return TEST_SUCCESS;
}

static test_return_t set_test2(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  const char *value= "train in the brain";
  size_t value_length= strlen(value);
  unsigned int x;

  for (x= 0; x < 10; x++)
  {
    rc= memcached_set(memc, key, strlen(key),
                      value, value_length,
                      (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  return TEST_SUCCESS;
}

static test_return_t set_test3(memcached_st *memc)
{
  memcached_return_t rc;
  char *value;
  size_t value_length= 8191;
  unsigned int x;

  value = (char*)malloc(value_length);
  test_true(value);

  for (x= 0; x < value_length; x++)
    value[x] = (char) (x % 127);

  /* The dump test relies on there being at least 32 items in memcached */
  for (x= 0; x < 32; x++)
  {
    char key[16];

    sprintf(key, "foo%u", x);

    rc= memcached_set(memc, key, strlen(key),
                      value, value_length,
                      (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  free(value);

  return TEST_SUCCESS;
}

static test_return_t get_test3(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  char *value;
  size_t value_length= 8191;
  char *string;
  size_t string_length;
  uint32_t flags;
  uint32_t x;

  value = (char*)malloc(value_length);
  test_true(value);

  for (x= 0; x < value_length; x++)
    value[x] = (char) (x % 127);

  rc= memcached_set(memc, key, strlen(key),
                    value, value_length,
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  test_true(rc == MEMCACHED_SUCCESS);
  test_true(string);
  test_true(string_length == value_length);
  test_true(!memcmp(string, value, string_length));

  free(string);
  free(value);

  return TEST_SUCCESS;
}

static test_return_t get_test4(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  char *value;
  size_t value_length= 8191;
  char *string;
  size_t string_length;
  uint32_t flags;
  uint32_t x;

  value = (char*)malloc(value_length);
  test_true(value);

  for (x= 0; x < value_length; x++)
    value[x] = (char) (x % 127);

  rc= memcached_set(memc, key, strlen(key),
                    value, value_length,
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  for (x= 0; x < 10; x++)
  {
    string= memcached_get(memc, key, strlen(key),
                          &string_length, &flags, &rc);

    test_true(rc == MEMCACHED_SUCCESS);
    test_true(string);
    test_true(string_length == value_length);
    test_true(!memcmp(string, value, string_length));
    free(string);
  }

  free(value);

  return TEST_SUCCESS;
}

/*
 * This test verifies that memcached_read_one_response doesn't try to
 * dereference a NIL-pointer if you issue a multi-get and don't read out all
 * responses before you execute a storage command.
 */
static test_return_t get_test5(memcached_st *memc)
{
  /*
  ** Request the same key twice, to ensure that we hash to the same server
  ** (so that we have multiple response values queued up) ;-)
  */
  const char *keys[]= { "key", "key" };
  size_t lengths[]= { 3, 3 };
  uint32_t flags;
  size_t rlen;

  memcached_return_t rc= memcached_set(memc, keys[0], lengths[0],
                                     keys[0], lengths[0], 0, 0);
  test_true(rc == MEMCACHED_SUCCESS);
  rc= memcached_mget(memc, keys, lengths, 2);

  memcached_result_st results_obj;
  memcached_result_st *results;
  results=memcached_result_create(memc, &results_obj);
  test_true(results);
  results=memcached_fetch_result(memc, &results_obj, &rc);
  test_true(results);
  memcached_result_free(&results_obj);

  /* Don't read out the second result, but issue a set instead.. */
  rc= memcached_set(memc, keys[0], lengths[0], keys[0], lengths[0], 0, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  char *val= memcached_get_by_key(memc, keys[0], lengths[0], "yek", 3,
                                  &rlen, &flags, &rc);
  test_true(val == NULL);
  test_true(rc == MEMCACHED_NOTFOUND);
  val= memcached_get(memc, keys[0], lengths[0], &rlen, &flags, &rc);
  test_true(val != NULL);
  test_true(rc == MEMCACHED_SUCCESS);
  free(val);

  return TEST_SUCCESS;
}

static test_return_t mget_end(memcached_st *memc)
{
  const char *keys[]= { "foo", "foo2" };
  size_t lengths[]= { 3, 4 };
  const char *values[]= { "fjord", "41" };

  memcached_return_t rc;

  // Set foo and foo2
  for (int i= 0; i < 2; i++)
  {
    rc= memcached_set(memc, keys[i], lengths[i], values[i], strlen(values[i]),
		      (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  char *string;
  size_t string_length;
  uint32_t flags;

  // retrieve both via mget
  rc= memcached_mget(memc, keys, lengths, 2);
  test_true(rc == MEMCACHED_SUCCESS);

  char key[MEMCACHED_MAX_KEY];
  size_t key_length;

  // this should get both
  for (int i = 0; i < 2; i++)
  {
    string= memcached_fetch(memc, key, &key_length, &string_length,
                            &flags, &rc);
    test_true(rc == MEMCACHED_SUCCESS);
    int val = 0;
    if (key_length == 4)
      val= 1;
    test_true(string_length == strlen(values[val]));
    test_true(strncmp(values[val], string, string_length) == 0);
    free(string);
  }

  // this should indicate end
  string= memcached_fetch(memc, key, &key_length, &string_length, &flags, &rc);
  test_true(rc == MEMCACHED_END);

  // now get just one
  rc= memcached_mget(memc, keys, lengths, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  string= memcached_fetch(memc, key, &key_length, &string_length, &flags, &rc);
  test_true(key_length == lengths[0]);
  test_true(strncmp(keys[0], key, key_length) == 0);
  test_true(string_length == strlen(values[0]));
  test_true(strncmp(values[0], string, string_length) == 0);
  test_true(rc == MEMCACHED_SUCCESS);
  free(string);

  // this should indicate end
  string= memcached_fetch(memc, key, &key_length, &string_length, &flags, &rc);
  test_true(rc == MEMCACHED_END);

  return TEST_SUCCESS;
}

/* Do not copy the style of this code, I just access hosts to testthis function */
static test_return_t stats_servername_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_stat_st memc_stat;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
  if (memcached_get_sasl_callbacks(memc) != NULL)
    return TEST_SKIPPED;
#endif
  rc= memcached_stat_servername(&memc_stat, NULL,
                                memcached_server_name(instance),
                                memcached_server_port(instance));

  return TEST_SUCCESS;
}

static test_return_t increment_test(memcached_st *memc)
{
  uint64_t new_number;
  memcached_return_t rc;
  const char *key= "number";
  const char *value= "0";

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  rc= memcached_increment(memc, key, strlen(key),
                          1, &new_number);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(new_number == 1);

  rc= memcached_increment(memc, key, strlen(key),
                          1, &new_number);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(new_number == 2);

  return TEST_SUCCESS;
}

static test_return_t increment_with_initial_test(memcached_st *memc)
{
  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) != 0)
  {
    uint64_t new_number;
    memcached_return_t rc;
    const char *key= "number";
    uint64_t initial= 0;

    rc= memcached_increment_with_initial(memc, key, strlen(key),
                                         1, initial, 0, &new_number);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(new_number == initial);

    rc= memcached_increment_with_initial(memc, key, strlen(key),
                                         1, initial, 0, &new_number);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(new_number == (initial + 1));
  }
  return TEST_SUCCESS;
}

static test_return_t decrement_test(memcached_st *memc)
{
  uint64_t new_number;
  memcached_return_t rc;
  const char *key= "number";
  const char *value= "3";

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  rc= memcached_decrement(memc, key, strlen(key),
                          1, &new_number);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(new_number == 2);

  rc= memcached_decrement(memc, key, strlen(key),
                          1, &new_number);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(new_number == 1);

  return TEST_SUCCESS;
}

static test_return_t decrement_with_initial_test(memcached_st *memc)
{
  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) != 0)
  {
    uint64_t new_number;
    memcached_return_t rc;
    const char *key= "number";
    uint64_t initial= 3;

    rc= memcached_decrement_with_initial(memc, key, strlen(key),
                                         1, initial, 0, &new_number);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(new_number == initial);

    rc= memcached_decrement_with_initial(memc, key, strlen(key),
                                         1, initial, 0, &new_number);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(new_number == (initial - 1));
  }
  return TEST_SUCCESS;
}

static test_return_t increment_by_key_test(memcached_st *memc)
{
  uint64_t new_number;
  memcached_return_t rc;
  const char *master_key= "foo";
  const char *key= "number";
  const char *value= "0";

  rc= memcached_set_by_key(memc, master_key, strlen(master_key),
                           key, strlen(key),
                           value, strlen(value),
                           (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  rc= memcached_increment_by_key(memc, master_key, strlen(master_key), key, strlen(key),
                                 1, &new_number);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(new_number == 1);

  rc= memcached_increment_by_key(memc, master_key, strlen(master_key), key, strlen(key),
                                 1, &new_number);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(new_number == 2);

  return TEST_SUCCESS;
}

static test_return_t increment_with_initial_by_key_test(memcached_st *memc)
{
  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) != 0)
  {
    uint64_t new_number;
    memcached_return_t rc;
    const char *master_key= "foo";
    const char *key= "number";
    uint64_t initial= 0;

    rc= memcached_increment_with_initial_by_key(memc, master_key, strlen(master_key),
                                                key, strlen(key),
                                                1, initial, 0, &new_number);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(new_number == initial);

    rc= memcached_increment_with_initial_by_key(memc, master_key, strlen(master_key),
                                                key, strlen(key),
                                                1, initial, 0, &new_number);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(new_number == (initial + 1));
  }
  return TEST_SUCCESS;
}

static test_return_t decrement_by_key_test(memcached_st *memc)
{
  uint64_t new_number;
  memcached_return_t rc;
  const char *master_key= "foo";
  const char *key= "number";
  const char *value= "3";

  rc= memcached_set_by_key(memc, master_key, strlen(master_key),
                           key, strlen(key),
                           value, strlen(value),
                           (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  rc= memcached_decrement_by_key(memc, master_key, strlen(master_key),
                                 key, strlen(key),
                                 1, &new_number);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(new_number == 2);

  rc= memcached_decrement_by_key(memc, master_key, strlen(master_key),
                                 key, strlen(key),
                                 1, &new_number);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(new_number == 1);

  return TEST_SUCCESS;
}

static test_return_t decrement_with_initial_by_key_test(memcached_st *memc)
{
  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) != 0)
  {
    uint64_t new_number;
    memcached_return_t rc;
    const char *master_key= "foo";
    const char *key= "number";
    uint64_t initial= 3;

    rc= memcached_decrement_with_initial_by_key(memc, master_key, strlen(master_key),
                                                key, strlen(key),
                                                1, initial, 0, &new_number);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(new_number == initial);

    rc= memcached_decrement_with_initial_by_key(memc, master_key, strlen(master_key),
                                                key, strlen(key),
                                                1, initial, 0, &new_number);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(new_number == (initial - 1));
  }
  return TEST_SUCCESS;
}

static test_return_t quit_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "fudge";
  const char *value= "sanford and sun";

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)10, (uint32_t)3);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  memcached_quit(memc);

  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)50, (uint32_t)9);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  return TEST_SUCCESS;
}

static test_return_t mget_result_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};
  unsigned int x;

  memcached_result_st results_obj;
  memcached_result_st *results;

  results= memcached_result_create(memc, &results_obj);
  test_true(results);
  test_true(&results_obj == results);

  /* We need to empty the server before continueing test */
  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_SUCCESS);

  while ((results= memcached_fetch_result(memc, &results_obj, &rc)) != NULL)
  {
    test_true(results);
  }

  while ((results= memcached_fetch_result(memc, &results_obj, &rc)) != NULL)
  test_true(!results);
  test_true(rc == MEMCACHED_END);

  for (x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_SUCCESS);

  while ((results= memcached_fetch_result(memc, &results_obj, &rc)))
  {
    test_true(results);
    test_true(&results_obj == results);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(memcached_result_key_length(results) == memcached_result_length(results));
    test_true(!memcmp(memcached_result_key_value(results),
                   memcached_result_value(results),
                   memcached_result_length(results)));
  }

  memcached_result_free(&results_obj);

  return TEST_SUCCESS;
}

static test_return_t mget_result_alloc_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};
  unsigned int x;

  memcached_result_st *results;

  /* We need to empty the server before continueing test */
  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_SUCCESS);

  while ((results= memcached_fetch_result(memc, NULL, &rc)) != NULL)
  {
    test_true(results);
  }
  test_true(!results);
  test_true(rc == MEMCACHED_END);

  for (x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_SUCCESS);

  x= 0;
  while ((results= memcached_fetch_result(memc, NULL, &rc)))
  {
    test_true(results);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(memcached_result_key_length(results) == memcached_result_length(results));
    test_true(!memcmp(memcached_result_key_value(results),
                   memcached_result_value(results),
                   memcached_result_length(results)));
    memcached_result_free(results);
    x++;
  }

  return TEST_SUCCESS;
}

/* Count the results */
static memcached_return_t callback_counter(const memcached_st *ptr __attribute__((unused)),
                                           memcached_result_st *result __attribute__((unused)),
                                           void *context)
{
  size_t *counter= (size_t *)context;

  *counter= *counter + 1;

  return MEMCACHED_SUCCESS;
}

static test_return_t mget_result_function(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};
  unsigned int x;
  size_t counter;
  memcached_execute_fn callbacks[1];

  /* We need to empty the server before continueing test */
  rc= memcached_flush(memc, 0);
  for (x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_SUCCESS);

  callbacks[0]= &callback_counter;
  counter= 0;
  rc= memcached_fetch_execute(memc, callbacks, (void *)&counter, 1);

  test_true(counter == 3);

  return TEST_SUCCESS;
}

static test_return_t mget_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};
  unsigned int x;
  uint32_t flags;

  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *return_value;
  size_t return_value_length;

  /* We need to empty the server before continueing test */
  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_SUCCESS);

  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                      &return_value_length, &flags, &rc)) != NULL)
  {
    test_true(return_value);
  }
  test_true(!return_value);
  test_true(return_value_length == 0);
  test_true(rc == MEMCACHED_END);

  for (x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_SUCCESS);

  x= 0;
  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                        &return_value_length, &flags, &rc)))
  {
    test_true(return_value);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(return_key_length == return_value_length);
    test_true(!memcmp(return_value, return_key, return_value_length));
    free(return_value);
    x++;
  }

  return TEST_SUCCESS;
}

static test_return_t mget_execute(memcached_st *memc)
{
  bool binary= false;

  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) != 0)
    binary= true;

  /*
   * I only want to hit _one_ server so I know the number of requests I'm
   * sending in the pipeline.
   */
  uint32_t number_of_hosts= memc->number_of_hosts;
  memc->number_of_hosts= 1;

  size_t max_keys= 20480;


  char **keys= calloc(max_keys, sizeof(char*));
  size_t *key_length=calloc(max_keys, sizeof(size_t));

  /* First add all of the items.. */
  char blob[1024] = {0};
  memcached_return_t rc;

  for (size_t x= 0; x < max_keys; ++x)
  {
    char k[251];

    key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%zu", x);
    keys[x]= strdup(k);
    test_true(keys[x] != NULL);
    rc= memcached_add(memc, keys[x], key_length[x], blob, sizeof(blob), 0, 0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  /* Try to get all of them with a large multiget */
  size_t counter= 0;
  memcached_execute_fn callbacks[1]= { [0]= &callback_counter };
  rc= memcached_mget_execute(memc, (const char**)keys, key_length,
                             max_keys, callbacks, &counter, 1);

  if (rc == MEMCACHED_SUCCESS)
  {
    test_true(binary);
    rc= memcached_fetch_execute(memc, callbacks, (void *)&counter, 1);
    test_true(rc == MEMCACHED_END);

    /* Verify that we got all of the items */
    test_true(counter == max_keys);
  }
  else if (rc == MEMCACHED_NOT_SUPPORTED)
  {
    test_true(counter == 0);
  }
  else
  {
    test_fail("note: this test functions differently when in binary mode");
  }

  /* Release all allocated resources */
  for (size_t x= 0; x < max_keys; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  memc->number_of_hosts= number_of_hosts;
  return TEST_SUCCESS;
}

#define REGRESSION_BINARY_VS_BLOCK_COUNT  20480

static test_return_t key_setup(memcached_st *memc)
{
  (void)memc;

  if (pre_binary(memc) != TEST_SUCCESS)
    return TEST_SKIPPED;

  global_pairs= pairs_generate(REGRESSION_BINARY_VS_BLOCK_COUNT, 0);

  return TEST_SUCCESS;
}

static test_return_t key_teardown(memcached_st *memc)
{
  (void)memc;
  pairs_free(global_pairs);

  return TEST_SUCCESS;
}

static test_return_t block_add_regression(memcached_st *memc)
{
  /* First add all of the items.. */
  for (size_t x= 0; x < REGRESSION_BINARY_VS_BLOCK_COUNT; ++x)
  {
    memcached_return_t rc;
    char blob[1024] = {0};

    rc= memcached_add_by_key(memc, "bob", 3, global_pairs[x].key, global_pairs[x].key_length, blob, sizeof(blob), 0, 0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  return TEST_SUCCESS;
}

static test_return_t binary_add_regression(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
  test_return_t rc= block_add_regression(memc);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
  return rc;
}

static test_return_t get_stats_keys(memcached_st *memc)
{
 char **stat_list;
 char **ptr;
 memcached_stat_st memc_stat;
 memcached_return_t rc;

 stat_list= memcached_stat_get_keys(memc, &memc_stat, &rc);
 test_true(rc == MEMCACHED_SUCCESS);
 for (ptr= stat_list; *ptr; ptr++)
   test_true(*ptr);

 free(stat_list);

 return TEST_SUCCESS;
}

static test_return_t version_string_test(memcached_st *memc __attribute__((unused)))
{
  const char *version_string;

  version_string= memcached_lib_version();

  test_true(!strcmp(version_string, LIBMEMCACHED_VERSION_STRING));

  return TEST_SUCCESS;
}

static test_return_t get_stats(memcached_st *memc)
{
 char **stat_list;
 char **ptr;
 memcached_return_t rc;
 memcached_stat_st *memc_stat;

 memc_stat= memcached_stat(memc, NULL, &rc);
 test_true(rc == MEMCACHED_SUCCESS);

 test_true(rc == MEMCACHED_SUCCESS);
 test_true(memc_stat);

 for (uint32_t x= 0; x < memcached_server_count(memc); x++)
 {
   stat_list= memcached_stat_get_keys(memc, memc_stat+x, &rc);
   test_true(rc == MEMCACHED_SUCCESS);
   for (ptr= stat_list; *ptr; ptr++);

   free(stat_list);
 }

 memcached_stat_free(NULL, memc_stat);

  return TEST_SUCCESS;
}

static test_return_t add_host_test(memcached_st *memc)
{
  unsigned int x;
  memcached_server_st *servers;
  memcached_return_t rc;
  char servername[]= "0.example.com";

  servers= memcached_server_list_append_with_weight(NULL, servername, 400, 0, &rc);
  test_true(servers);
  test_true(1 == memcached_server_list_count(servers));

  for (x= 2; x < 20; x++)
  {
    char buffer[SMALL_STRING_LEN];

    snprintf(buffer, SMALL_STRING_LEN, "%u.example.com", 400+x);
    servers= memcached_server_list_append_with_weight(servers, buffer, 401, 0,
                                     &rc);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(x == memcached_server_list_count(servers));
  }

  rc= memcached_server_push(memc, servers);
  test_true(rc == MEMCACHED_SUCCESS);
  rc= memcached_server_push(memc, servers);
  test_true(rc == MEMCACHED_SUCCESS);

  memcached_server_list_free(servers);

  return TEST_SUCCESS;
}

static memcached_return_t  clone_test_callback(memcached_st *parent __attribute__((unused)), memcached_st *memc_clone __attribute__((unused)))
{
  return MEMCACHED_SUCCESS;
}

static memcached_return_t  cleanup_test_callback(memcached_st *ptr __attribute__((unused)))
{
  return MEMCACHED_SUCCESS;
}

static test_return_t callback_test(memcached_st *memc)
{
  /* Test User Data */
  {
    int x= 5;
    int *test_ptr;
    memcached_return_t rc;

    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_USER_DATA, &x);
    test_true(rc == MEMCACHED_SUCCESS);
    test_ptr= (int *)memcached_callback_get(memc, MEMCACHED_CALLBACK_USER_DATA, &rc);
    test_true(*test_ptr == x);
  }

  /* Test Clone Callback */
  {
    memcached_clone_fn clone_cb= (memcached_clone_fn)clone_test_callback;
    void *clone_cb_ptr= *(void **)&clone_cb;
    void *temp_function= NULL;
    memcached_return_t rc;

    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_CLONE_FUNCTION,
                               clone_cb_ptr);
    test_true(rc == MEMCACHED_SUCCESS);
    temp_function= memcached_callback_get(memc, MEMCACHED_CALLBACK_CLONE_FUNCTION, &rc);
    test_true(temp_function == clone_cb_ptr);
  }

  /* Test Cleanup Callback */
  {
    memcached_cleanup_fn cleanup_cb=
      (memcached_cleanup_fn)cleanup_test_callback;
    void *cleanup_cb_ptr= *(void **)&cleanup_cb;
    void *temp_function= NULL;
    memcached_return_t rc;

    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_CLONE_FUNCTION,
                               cleanup_cb_ptr);
    test_true(rc == MEMCACHED_SUCCESS);
    temp_function= memcached_callback_get(memc, MEMCACHED_CALLBACK_CLONE_FUNCTION, &rc);
    test_true(temp_function == cleanup_cb_ptr);
  }

  return TEST_SUCCESS;
}

/* We don't test the behavior itself, we test the switches */
static test_return_t behavior_test(memcached_st *memc)
{
  uint64_t value;
  uint32_t set= 1;

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, set);
  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NO_BLOCK);
  test_true(value == 1);

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, set);
  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY);
  test_true(value == 1);

  set= MEMCACHED_HASH_MD5;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, set);
  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH);
  test_true(value == MEMCACHED_HASH_MD5);

  set= 0;

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, set);
  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NO_BLOCK);
  test_true(value == 0);

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, set);
  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY);
  test_true(value == 0);

  set= MEMCACHED_HASH_DEFAULT;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, set);
  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH);
  test_true(value == MEMCACHED_HASH_DEFAULT);

  set= MEMCACHED_HASH_CRC;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, set);
  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH);
  test_true(value == MEMCACHED_HASH_CRC);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE);
  test_true(value > 0);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE);
  test_true(value > 0);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, value + 1);
  test_true((value + 1) == memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS));

  return TEST_SUCCESS;
}

static test_return_t MEMCACHED_BEHAVIOR_CORK_test(memcached_st *memc)
{
  memcached_return_t rc;
  bool set= true;
  bool value;

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CORK, set);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_NOT_SUPPORTED);

  value= (bool)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_CORK);

  if (rc == MEMCACHED_SUCCESS)
  {
    test_true((bool)value == set);
  }
  else
  {
    test_false((bool)value == set);
  }

  return TEST_SUCCESS;
}


static test_return_t MEMCACHED_BEHAVIOR_TCP_KEEPALIVE_test(memcached_st *memc)
{
  memcached_return_t rc;
  bool set= true;
  bool value;

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE, set);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_NOT_SUPPORTED);

  value= (bool)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE);

  if (rc == MEMCACHED_SUCCESS)
  {
    test_true((bool)value == set);
  }
  else
  {
    test_false((bool)value == set);
  }

  return TEST_SUCCESS;
}


static test_return_t MEMCACHED_BEHAVIOR_TCP_KEEPIDLE_test(memcached_st *memc)
{
  memcached_return_t rc;
  bool set= true;
  bool value;

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_KEEPIDLE, set);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_NOT_SUPPORTED);

  value= (bool)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_TCP_KEEPIDLE);

  if (rc == MEMCACHED_SUCCESS)
  {
    test_true((bool)value == set);
  }
  else
  {
    test_false((bool)value == set);
  }

  return TEST_SUCCESS;
}

static test_return_t fetch_all_results(memcached_st *memc)
{
  memcached_return_t rc= MEMCACHED_SUCCESS;
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *return_value;
  size_t return_value_length;
  uint32_t flags;

  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                        &return_value_length, &flags, &rc)))
  {
    test_true(return_value);
    test_true(rc == MEMCACHED_SUCCESS);
    free(return_value);
  }

  return ((rc == MEMCACHED_END) || (rc == MEMCACHED_SUCCESS)) ? TEST_SUCCESS : TEST_FAILURE;
}

/* Test case provided by Cal Haldenbrand */
static test_return_t user_supplied_bug1(memcached_st *memc)
{
  unsigned int setter= 1;

  unsigned long long total= 0;
  uint32_t size= 0;
  char key[10];
  char randomstuff[6 * 1024];
  memcached_return_t rc;

  memset(randomstuff, 0, 6 * 1024);

  /* We just keep looking at the same values over and over */
  srandom(10);

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, setter);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, setter);


  /* add key */
  for (uint32_t x= 0 ; total < 20 * 1024576 ; x++ )
  {
    unsigned int j= 0;

    size= (uint32_t)(rand() % ( 5 * 1024 ) ) + 400;
    memset(randomstuff, 0, 6 * 1024);
    test_true(size < 6 * 1024); /* Being safe here */

    for (j= 0 ; j < size ;j++)
      randomstuff[j] = (signed char) ((rand() % 26) + 97);

    total += size;
    snprintf(key, sizeof(key), "%u", x);
    rc = memcached_set(memc, key, strlen(key),
                       randomstuff, strlen(randomstuff), 10, 0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
    /* If we fail, lets try again */
    if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_BUFFERED)
      rc = memcached_set(memc, key, strlen(key),
                         randomstuff, strlen(randomstuff), 10, 0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  return TEST_SUCCESS;
}

/* Test case provided by Cal Haldenbrand */
static test_return_t user_supplied_bug2(memcached_st *memc)
{
  unsigned int setter;
  size_t total= 0;

  setter= 1;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, setter);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, setter);
#ifdef NOT_YET
  setter = 20 * 1024576;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE, setter);
  setter = 20 * 1024576;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE, setter);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE);

  for (x= 0, errors= 0; total < 20 * 1024576 ; x++)
#endif

  for (uint32_t x= 0, errors= 0; total < 24576 ; x++)
  {
    memcached_return_t rc= MEMCACHED_SUCCESS;
    char buffer[SMALL_STRING_LEN];
    uint32_t flags= 0;
    size_t val_len= 0;
    char *getval;

    memset(buffer, 0, SMALL_STRING_LEN);

    snprintf(buffer, sizeof(buffer), "%u", x);
    getval= memcached_get(memc, buffer, strlen(buffer),
                           &val_len, &flags, &rc);
    if (rc != MEMCACHED_SUCCESS)
    {
      if (rc == MEMCACHED_NOTFOUND)
        errors++;
      else
      {
        test_true(rc);
      }

      continue;
    }
    total+= val_len;
    errors= 0;
    free(getval);
  }

  return TEST_SUCCESS;
}

/* Do a large mget() over all the keys we think exist */
#define KEY_COUNT 3000 // * 1024576
static test_return_t user_supplied_bug3(memcached_st *memc)
{
  memcached_return_t rc;
  unsigned int setter;
  unsigned int x;
  char **keys;
  size_t key_lengths[KEY_COUNT];

  setter= 1;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, setter);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, setter);
#ifdef NOT_YET
  setter = 20 * 1024576;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE, setter);
  setter = 20 * 1024576;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE, setter);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE);
#endif

  keys= calloc(KEY_COUNT, sizeof(char *));
  test_true(keys);
  for (x= 0; x < KEY_COUNT; x++)
  {
    char buffer[30];

    snprintf(buffer, 30, "%u", x);
    keys[x]= strdup(buffer);
    key_lengths[x]= strlen(keys[x]);
  }

  rc= memcached_mget(memc, (const char **)keys, key_lengths, KEY_COUNT);
  test_true(rc == MEMCACHED_SUCCESS);

  test_true(fetch_all_results(memc) == TEST_SUCCESS);

  for (x= 0; x < KEY_COUNT; x++)
    free(keys[x]);
  free(keys);

  return TEST_SUCCESS;
}

/* Make sure we behave properly if server list has no values */
static test_return_t user_supplied_bug4(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"fudge", "son", "food"};
  size_t key_length[]= {5, 3, 4};
  unsigned int x;
  uint32_t flags;
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *return_value;
  size_t return_value_length;

  /* Here we free everything before running a bunch of mget tests */
  memcached_servers_reset(memc);


  /* We need to empty the server before continueing test */
  rc= memcached_flush(memc, 0);
  test_true(rc == MEMCACHED_NO_SERVERS);

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_NO_SERVERS);

  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                      &return_value_length, &flags, &rc)) != NULL)
  {
    test_true(return_value);
  }
  test_true(!return_value);
  test_true(return_value_length == 0);
  test_true(rc == MEMCACHED_NO_SERVERS);

  for (x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true(rc == MEMCACHED_NO_SERVERS);
  }

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_NO_SERVERS);

  x= 0;
  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                        &return_value_length, &flags, &rc)))
  {
    test_true(return_value);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(return_key_length == return_value_length);
    test_true(!memcmp(return_value, return_key, return_value_length));
    free(return_value);
    x++;
  }

  return TEST_SUCCESS;
}

#define VALUE_SIZE_BUG5 1048064
static test_return_t user_supplied_bug5(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"036790384900", "036790384902", "036790384904", "036790384906"};
  size_t key_length[]=  {strlen("036790384900"), strlen("036790384902"), strlen("036790384904"), strlen("036790384906")};
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *value;
  size_t value_length;
  uint32_t flags;
  unsigned int count;
  unsigned int x;
  char insert_data[VALUE_SIZE_BUG5];

  for (x= 0; x < VALUE_SIZE_BUG5; x++)
    insert_data[x]= (signed char)rand();

  memcached_flush(memc, 0);
  value= memcached_get(memc, keys[0], key_length[0],
                        &value_length, &flags, &rc);
  test_true(value == NULL);
  rc= memcached_mget(memc, keys, key_length, 4);

  count= 0;
  while ((value= memcached_fetch(memc, return_key, &return_key_length,
                                        &value_length, &flags, &rc)))
    count++;
  test_true(count == 0);

  for (x= 0; x < 4; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      insert_data, VALUE_SIZE_BUG5,
                      (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  for (x= 0; x < 10; x++)
  {
    value= memcached_get(memc, keys[0], key_length[0],
                         &value_length, &flags, &rc);
    test_true(value);
    free(value);

    rc= memcached_mget(memc, keys, key_length, 4);
    count= 0;
    while ((value= memcached_fetch(memc, return_key, &return_key_length,
                                          &value_length, &flags, &rc)))
    {
      count++;
      free(value);
    }
    test_true(count == 4);
  }

  return TEST_SUCCESS;
}

static test_return_t user_supplied_bug6(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"036790384900", "036790384902", "036790384904", "036790384906"};
  size_t key_length[]=  {strlen("036790384900"), strlen("036790384902"), strlen("036790384904"), strlen("036790384906")};
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *value;
  size_t value_length;
  uint32_t flags;
  unsigned int count;
  unsigned int x;
  char insert_data[VALUE_SIZE_BUG5];

  for (x= 0; x < VALUE_SIZE_BUG5; x++)
    insert_data[x]= (signed char)rand();

  memcached_flush(memc, 0);
  value= memcached_get(memc, keys[0], key_length[0],
                        &value_length, &flags, &rc);
  test_true(value == NULL);
  test_true(rc == MEMCACHED_NOTFOUND);
  rc= memcached_mget(memc, keys, key_length, 4);
  test_true(rc == MEMCACHED_SUCCESS);

  count= 0;
  while ((value= memcached_fetch(memc, return_key, &return_key_length,
                                        &value_length, &flags, &rc)))
    count++;
  test_true(count == 0);
  test_true(rc == MEMCACHED_END);

  for (x= 0; x < 4; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      insert_data, VALUE_SIZE_BUG5,
                      (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  for (x= 0; x < 2; x++)
  {
    value= memcached_get(memc, keys[0], key_length[0],
                         &value_length, &flags, &rc);
    test_true(value);
    free(value);

    rc= memcached_mget(memc, keys, key_length, 4);
    test_true(rc == MEMCACHED_SUCCESS);
    count= 3;
    /* We test for purge of partial complete fetches */
    for (count= 3; count; count--)
    {
      value= memcached_fetch(memc, return_key, &return_key_length,
                             &value_length, &flags, &rc);
      test_true(rc == MEMCACHED_SUCCESS);
      test_true(!(memcmp(value, insert_data, value_length)));
      test_true(value_length);
      free(value);
    }
  }

  return TEST_SUCCESS;
}

static test_return_t user_supplied_bug8(memcached_st *memc __attribute__((unused)))
{
  memcached_return_t rc;
  memcached_st *mine;
  memcached_st *memc_clone;

  memcached_server_st *servers;
  const char *server_list= "memcache1.memcache.bk.sapo.pt:11211, memcache1.memcache.bk.sapo.pt:11212, memcache1.memcache.bk.sapo.pt:11213, memcache1.memcache.bk.sapo.pt:11214, memcache2.memcache.bk.sapo.pt:11211, memcache2.memcache.bk.sapo.pt:11212, memcache2.memcache.bk.sapo.pt:11213, memcache2.memcache.bk.sapo.pt:11214";

  servers= memcached_servers_parse(server_list);
  test_true(servers);

  mine= memcached_create(NULL);
  rc= memcached_server_push(mine, servers);
  test_true(rc == MEMCACHED_SUCCESS);
  memcached_server_list_free(servers);

  test_true(mine);
  memc_clone= memcached_clone(NULL, mine);

  memcached_quit(mine);
  memcached_quit(memc_clone);


  memcached_free(mine);
  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

/* Test flag store/retrieve */
static test_return_t user_supplied_bug7(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys= "036790384900";
  size_t key_length=  strlen(keys);
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *value;
  size_t value_length;
  uint32_t flags;
  unsigned int x;
  char insert_data[VALUE_SIZE_BUG5];

  for (x= 0; x < VALUE_SIZE_BUG5; x++)
    insert_data[x]= (signed char)rand();

  memcached_flush(memc, 0);

  flags= 245;
  rc= memcached_set(memc, keys, key_length,
                    insert_data, VALUE_SIZE_BUG5,
                    (time_t)0, flags);
  test_true(rc == MEMCACHED_SUCCESS);

  flags= 0;
  value= memcached_get(memc, keys, key_length,
                        &value_length, &flags, &rc);
  test_true(flags == 245);
  test_true(value);
  free(value);

  rc= memcached_mget(memc, &keys, &key_length, 1);

  flags= 0;
  value= memcached_fetch(memc, return_key, &return_key_length,
                         &value_length, &flags, &rc);
  test_true(flags == 245);
  test_true(value);
  free(value);


  return TEST_SUCCESS;
}

static test_return_t user_supplied_bug9(memcached_st *memc)
{
  memcached_return_t rc;
  const char *keys[]= {"UDATA:edevil@sapo.pt", "fudge&*@#", "for^#@&$not"};
  size_t key_length[3];
  unsigned int x;
  uint32_t flags;
  unsigned count= 0;

  char return_key[MEMCACHED_MAX_KEY];
  size_t return_key_length;
  char *return_value;
  size_t return_value_length;


  key_length[0]= strlen("UDATA:edevil@sapo.pt");
  key_length[1]= strlen("fudge&*@#");
  key_length[2]= strlen("for^#@&$not");


  for (x= 0; x < 3; x++)
  {
    rc= memcached_set(memc, keys[x], key_length[x],
                      keys[x], key_length[x],
                      (time_t)50, (uint32_t)9);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  rc= memcached_mget(memc, keys, key_length, 3);
  test_true(rc == MEMCACHED_SUCCESS);

  /* We need to empty the server before continueing test */
  while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                      &return_value_length, &flags, &rc)) != NULL)
  {
    test_true(return_value);
    free(return_value);
    count++;
  }
  test_true(count == 3);

  return TEST_SUCCESS;
}

/* We are testing with aggressive timeout to get failures */
static test_return_t user_supplied_bug10(memcached_st *memc)
{
  const char *key= "foo";
  char *value;
  size_t value_length= 512;
  unsigned int x;
  size_t key_len= 3;
  memcached_return_t rc;
  unsigned int set= 1;
  memcached_st *mclone= memcached_clone(NULL, memc);
  int32_t timeout;

  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_NO_BLOCK, set);
  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_TCP_NODELAY, set);
  timeout= 2;
  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_POLL_TIMEOUT,
                         (uint64_t)timeout);

  value = (char*)malloc(value_length * sizeof(char));

  for (x= 0; x < value_length; x++)
    value[x]= (char) (x % 127);

  for (x= 1; x <= 100000; ++x)
  {
    rc= memcached_set(mclone, key, key_len,value, value_length, 0, 0);

    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_WRITE_FAILURE ||
           rc == MEMCACHED_BUFFERED || rc == MEMCACHED_TIMEOUT);

    if (rc == MEMCACHED_WRITE_FAILURE || rc == MEMCACHED_TIMEOUT)
      x--;
  }

  free(value);
  memcached_free(mclone);

  return TEST_SUCCESS;
}

/*
  We are looking failures in the async protocol
*/
static test_return_t user_supplied_bug11(memcached_st *memc)
{
  const char *key= "foo";
  char *value;
  size_t value_length= 512;
  unsigned int x;
  size_t key_len= 3;
  memcached_return_t rc;
  unsigned int set= 1;
  int32_t timeout;
  memcached_st *mclone= memcached_clone(NULL, memc);

  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_NO_BLOCK, set);
  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_TCP_NODELAY, set);
  timeout= -1;
  memcached_behavior_set(mclone, MEMCACHED_BEHAVIOR_POLL_TIMEOUT,
                         (size_t)timeout);

  timeout= (int32_t)memcached_behavior_get(mclone, MEMCACHED_BEHAVIOR_POLL_TIMEOUT);

  test_true(timeout == -1);

  value = (char*)malloc(value_length * sizeof(char));

  for (x= 0; x < value_length; x++)
    value[x]= (char) (x % 127);

  for (x= 1; x <= 100000; ++x)
  {
    rc= memcached_set(mclone, key, key_len,value, value_length, 0, 0);
  }

  free(value);
  memcached_free(mclone);

  return TEST_SUCCESS;
}

/*
  Bug found where incr was not returning MEMCACHED_NOTFOUND when object did not exist.
*/
static test_return_t user_supplied_bug12(memcached_st *memc)
{
  memcached_return_t rc;
  uint32_t flags;
  size_t value_length;
  char *value;
  uint64_t number_value;

  value= memcached_get(memc, "autoincrement", strlen("autoincrement"),
                        &value_length, &flags, &rc);
  test_true(value == NULL);
  test_true(rc == MEMCACHED_NOTFOUND);

  rc= memcached_increment(memc, "autoincrement", strlen("autoincrement"),
                          1, &number_value);

  test_true(value == NULL);
  /* The binary protocol will set the key if it doesn't exist */
  if (memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) == 1)
  {
    test_true(rc == MEMCACHED_SUCCESS);
  }
  else
  {
    test_true(rc == MEMCACHED_NOTFOUND);
  }

  rc= memcached_set(memc, "autoincrement", strlen("autoincrement"), "1", 1, 0, 0);

  value= memcached_get(memc, "autoincrement", strlen("autoincrement"),
                        &value_length, &flags, &rc);
  test_true(value);
  test_true(rc == MEMCACHED_SUCCESS);
  free(value);

  rc= memcached_increment(memc, "autoincrement", strlen("autoincrement"),
                          1, &number_value);
  test_true(number_value == 2);
  test_true(rc == MEMCACHED_SUCCESS);

  return TEST_SUCCESS;
}

/*
  Bug found where command total one more than MEMCACHED_MAX_BUFFER
  set key34567890 0 0 8169 \r\n is sent followed by buffer of size 8169, followed by 8169
 */
static test_return_t user_supplied_bug13(memcached_st *memc)
{
  char key[] = "key34567890";
  char *overflow;
  memcached_return_t rc;
  size_t overflowSize;

  char commandFirst[]= "set key34567890 0 0 ";
  char commandLast[] = " \r\n"; /* first line of command sent to server */
  size_t commandLength;
  size_t testSize;

  commandLength = strlen(commandFirst) + strlen(commandLast) + 4; /* 4 is number of characters in size, probably 8196 */

  overflowSize = MEMCACHED_MAX_BUFFER - commandLength;

  for (testSize= overflowSize - 1; testSize < overflowSize + 1; testSize++)
  {
    overflow= malloc(testSize);
    test_true(overflow != NULL);

    memset(overflow, 'x', testSize);
    rc= memcached_set(memc, key, strlen(key),
                      overflow, testSize, 0, 0);
    test_true(rc == MEMCACHED_SUCCESS);
    free(overflow);
  }

  return TEST_SUCCESS;
}


/*
  Test values of many different sizes
  Bug found where command total one more than MEMCACHED_MAX_BUFFER
  set key34567890 0 0 8169 \r\n
  is sent followed by buffer of size 8169, followed by 8169
 */
static test_return_t user_supplied_bug14(memcached_st *memc)
{
  size_t setter= 1;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, setter);
  memcached_return_t rc;
  const char *key= "foo";
  char *value;
  size_t value_length= 18000;
  char *string;
  size_t string_length;
  uint32_t flags;
  unsigned int x;
  size_t current_length;

  value = (char*)malloc(value_length);
  test_true(value);

  for (x= 0; x < value_length; x++)
    value[x] = (char) (x % 127);

  for (current_length= 0; current_length < value_length; current_length++)
  {
    rc= memcached_set(memc, key, strlen(key),
                      value, current_length,
                      (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

    string= memcached_get(memc, key, strlen(key),
                          &string_length, &flags, &rc);

    test_true(rc == MEMCACHED_SUCCESS);
    test_true(string_length == current_length);
    test_true(!memcmp(string, value, string_length));

    free(string);
  }

  free(value);

  return TEST_SUCCESS;
}

/*
  Look for zero length value problems
  */
static test_return_t user_supplied_bug15(memcached_st *memc)
{
  uint32_t x;
  memcached_return_t rc;
  const char *key= "mykey";
  char *value;
  size_t length;
  uint32_t flags;

  for (x= 0; x < 2; x++)
  {
    rc= memcached_set(memc, key, strlen(key),
                      NULL, 0,
                      (time_t)0, (uint32_t)0);

    test_true(rc == MEMCACHED_SUCCESS);

    value= memcached_get(memc, key, strlen(key),
                         &length, &flags, &rc);

    test_true(rc == MEMCACHED_SUCCESS);
    test_true(value == NULL);
    test_true(length == 0);
    test_true(flags == 0);

    value= memcached_get(memc, key, strlen(key),
                         &length, &flags, &rc);

    test_true(rc == MEMCACHED_SUCCESS);
    test_true(value == NULL);
    test_true(length == 0);
    test_true(flags == 0);
  }

  return TEST_SUCCESS;
}

/* Check the return sizes on FLAGS to make sure it stores 32bit unsigned values correctly */
static test_return_t user_supplied_bug16(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "mykey";
  char *value;
  size_t length;
  uint32_t flags;

  rc= memcached_set(memc, key, strlen(key),
                    NULL, 0,
                    (time_t)0, UINT32_MAX);

  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_get(memc, key, strlen(key),
                       &length, &flags, &rc);

  test_true(rc == MEMCACHED_SUCCESS);
  test_true(value == NULL);
  test_true(length == 0);
  test_true(flags == UINT32_MAX);

  return TEST_SUCCESS;
}

#if !defined(__sun) && !defined(__OpenBSD__)
/* Check the validity of chinese key*/
static test_return_t user_supplied_bug17(memcached_st *memc)
{
    memcached_return_t rc;
    const char *key= "豆瓣";
    const char *value="我们在炎热抑郁的夏天无法停止豆瓣";
    char *value2;
    size_t length;
    uint32_t flags;

    rc= memcached_set(memc, key, strlen(key),
            value, strlen(value),
            (time_t)0, 0);

    test_true(rc == MEMCACHED_SUCCESS);

    value2= memcached_get(memc, key, strlen(key),
            &length, &flags, &rc);

    test_true(length==strlen(value));
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(memcmp(value, value2, length)==0);
    free(value2);

    return TEST_SUCCESS;
}
#endif

/*
  From Andrei on IRC
*/

static test_return_t user_supplied_bug19(memcached_st *not_used)
{
  memcached_st *memc;
  const memcached_server_st *server;
  memcached_return_t res;

  (void)not_used;

  memc= memcached_create(NULL);
  memcached_server_add_with_weight(memc, "localhost", 11311, 100);
  memcached_server_add_with_weight(memc, "localhost", 11312, 100);

  server= memcached_server_by_key(memc, "a", 1, &res);

  memcached_free(memc);

  return TEST_SUCCESS;
}

/* CAS test from Andei */
static test_return_t user_supplied_bug20(memcached_st *memc)
{
  memcached_return_t status;
  memcached_result_st *result, result_obj;
  const char *key = "abc";
  size_t key_len = strlen("abc");
  const char *value = "foobar";
  size_t value_len = strlen(value);

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1);

  status = memcached_set(memc, key, key_len, value, value_len, (time_t)0, (uint32_t)0);
  test_true(status == MEMCACHED_SUCCESS);

  status = memcached_mget(memc, &key, &key_len, 1);
  test_true(status == MEMCACHED_SUCCESS);

  result= memcached_result_create(memc, &result_obj);
  test_true(result);

  memcached_result_create(memc, &result_obj);
  result= memcached_fetch_result(memc, &result_obj, &status);

  test_true(result);
  test_true(status == MEMCACHED_SUCCESS);

  memcached_result_free(result);

  return TEST_SUCCESS;
}

#include "ketama_test_cases.h"
static test_return_t user_supplied_bug18(memcached_st *trash)
{
  memcached_return_t rc;
  uint64_t value;
  int x;
  memcached_server_st *server_pool;
  memcached_st *memc;

  (void)trash;

  memc= memcached_create(NULL);
  test_true(memc);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  test_true(value == 1);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH, MEMCACHED_HASH_MD5);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH);
  test_true(value == MEMCACHED_HASH_MD5);

  server_pool = memcached_servers_parse("10.0.1.1:11211 600,10.0.1.2:11211 300,10.0.1.3:11211 200,10.0.1.4:11211 350,10.0.1.5:11211 1000,10.0.1.6:11211 800,10.0.1.7:11211 950,10.0.1.8:11211 100");
  memcached_server_push(memc, server_pool);

  /* verify that the server list was parsed okay. */
  test_true(memcached_server_count(memc) == 8);
  test_true(strcmp(server_pool[0].hostname, "10.0.1.1") == 0);
  test_true(server_pool[0].port == 11211);
  test_true(server_pool[0].weight == 600);
  test_true(strcmp(server_pool[2].hostname, "10.0.1.3") == 0);
  test_true(server_pool[2].port == 11211);
  test_true(server_pool[2].weight == 200);
  test_true(strcmp(server_pool[7].hostname, "10.0.1.8") == 0);
  test_true(server_pool[7].port == 11211);
  test_true(server_pool[7].weight == 100);

  /* VDEAAAAA hashes to fffcd1b5, after the last continuum point, and lets
   * us test the boundary wraparound.
   */
  test_true(memcached_generate_hash(memc, (char *)"VDEAAAAA", 8) == memc->continuum[0].index);

  /* verify the standard ketama set. */
  for (x= 0; x < 99; x++)
  {
    uint32_t server_idx = memcached_generate_hash(memc, ketama_test_cases[x].key, strlen(ketama_test_cases[x].key));

    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, server_idx);

    const char *hostname = memcached_server_name(instance);
    test_strcmp(hostname, ketama_test_cases[x].server);
  }

  memcached_server_list_free(server_pool);
  memcached_free(memc);

  return TEST_SUCCESS;
}

/* Large mget() of missing keys with binary proto
 *
 * If many binary quiet commands (such as getq's in an mget) fill the output
 * buffer and the server chooses not to respond, memcached_flush hangs. See
 * http://lists.tangent.org/pipermail/libmemcached/2009-August/000918.html
 */

/* sighandler_t function that always asserts false */
static void fail(int unused __attribute__((unused)))
{
  assert(0);
}


static test_return_t _user_supplied_bug21(memcached_st* memc, size_t key_count)
{
#ifdef WIN32
  (void)memc;
  (void)key_count;
  return TEST_SKIPPED;
#else
  memcached_return_t rc;
  unsigned int x;
  char **keys;
  size_t* key_lengths;
  void (*oldalarm)(int);
  memcached_st *memc_clone;

  memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  /* only binproto uses getq for mget */
  memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);

  /* empty the cache to ensure misses (hence non-responses) */
  rc= memcached_flush(memc_clone, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  key_lengths= calloc(key_count, sizeof(size_t));
  keys= calloc(key_count, sizeof(char *));
  test_true(keys);
  for (x= 0; x < key_count; x++)
  {
    char buffer[30];

    snprintf(buffer, 30, "%u", x);
    keys[x]= strdup(buffer);
    key_lengths[x]= strlen(keys[x]);
  }

  oldalarm= signal(SIGALRM, fail);
  alarm(5);

  rc= memcached_mget(memc_clone, (const char **)keys, key_lengths, key_count);
  test_true(rc == MEMCACHED_SUCCESS);

  alarm(0);
  signal(SIGALRM, oldalarm);

  test_true(fetch_all_results(memc) == TEST_SUCCESS);

  for (x= 0; x < key_count; x++)
    free(keys[x]);
  free(keys);
  free(key_lengths);

  memcached_free(memc_clone);

  return TEST_SUCCESS;
#endif
}

static test_return_t user_supplied_bug21(memcached_st *memc)
{
  test_return_t test_rc;
  test_rc= pre_binary(memc);

  if (test_rc != TEST_SUCCESS)
    return test_rc;

  test_return_t rc;

  /* should work as of r580 */
  rc= _user_supplied_bug21(memc, 10);
  test_true(rc == TEST_SUCCESS);

  /* should fail as of r580 */
  rc= _user_supplied_bug21(memc, 1000);
  test_true(rc == TEST_SUCCESS);

  return TEST_SUCCESS;
}

static test_return_t auto_eject_hosts(memcached_st *trash)
{
  (void) trash;
  memcached_server_instance_st instance;

  memcached_return_t rc;
  memcached_st *memc= memcached_create(NULL);
  test_true(memc);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  uint64_t value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  test_true(value == 1);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH, MEMCACHED_HASH_MD5);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH);
  test_true(value == MEMCACHED_HASH_MD5);

    /* server should be removed when in delay */
  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS);
  test_true(value == 1);

  memcached_server_st *server_pool;
  server_pool = memcached_servers_parse("10.0.1.1:11211 600,10.0.1.2:11211 300,10.0.1.3:11211 200,10.0.1.4:11211 350,10.0.1.5:11211 1000,10.0.1.6:11211 800,10.0.1.7:11211 950,10.0.1.8:11211 100");
  memcached_server_push(memc, server_pool);

  /* verify that the server list was parsed okay. */
  test_true(memcached_server_count(memc) == 8);
  test_true(strcmp(server_pool[0].hostname, "10.0.1.1") == 0);
  test_true(server_pool[0].port == 11211);
  test_true(server_pool[0].weight == 600);
  test_true(strcmp(server_pool[2].hostname, "10.0.1.3") == 0);
  test_true(server_pool[2].port == 11211);
  test_true(server_pool[2].weight == 200);
  test_true(strcmp(server_pool[7].hostname, "10.0.1.8") == 0);
  test_true(server_pool[7].port == 11211);
  test_true(server_pool[7].weight == 100);

  instance= memcached_server_instance_by_position(memc, 2);
  ((memcached_server_write_instance_st)instance)->next_retry = time(NULL) + 15;
  memc->next_distribution_rebuild= time(NULL) - 1;

  /*
    This would not work if there were only two hosts.
  */
  for (size_t x= 0; x < 99; x++)
  {
    memcached_autoeject(memc);
    uint32_t server_idx= memcached_generate_hash(memc, ketama_test_cases[x].key, strlen(ketama_test_cases[x].key));
    test_true(server_idx != 2);
  }

  /* and re-added when it's back. */
  ((memcached_server_write_instance_st)instance)->next_retry = time(NULL) - 1;
  memc->next_distribution_rebuild= time(NULL) - 1;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION,
                         memc->distribution);
  for (size_t x= 0; x < 99; x++)
  {
    uint32_t server_idx = memcached_generate_hash(memc, ketama_test_cases[x].key, strlen(ketama_test_cases[x].key));
    // We re-use instance from above.
    instance=
      memcached_server_instance_by_position(memc, server_idx);
    const char *hostname = memcached_server_name(instance);
    test_true(strcmp(hostname, ketama_test_cases[x].server) == 0);
  }

  memcached_server_list_free(server_pool);
  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t output_ketama_weighted_keys(memcached_st *trash)
{
  (void) trash;

  memcached_return_t rc;
  memcached_st *memc= memcached_create(NULL);
  test_true(memc);


  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  uint64_t value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  test_true(value == 1);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH, MEMCACHED_HASH_MD5);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH);
  test_true(value == MEMCACHED_HASH_MD5);


  test_true(memcached_behavior_set_distribution(memc, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY) == MEMCACHED_SUCCESS);

  memcached_server_st *server_pool;
  server_pool = memcached_servers_parse("10.0.1.1:11211,10.0.1.2:11211,10.0.1.3:11211,10.0.1.4:11211,10.0.1.5:11211,10.0.1.6:11211,10.0.1.7:11211,10.0.1.8:11211,192.168.1.1:11211,192.168.100.1:11211");
  memcached_server_push(memc, server_pool);

  // @todo this needs to be refactored to actually test something.
#if 0
  FILE *fp;
  if ((fp = fopen("ketama_keys.txt", "w")))
  {
    // noop
  } else {
    printf("cannot write to file ketama_keys.txt");
    return TEST_FAILURE;
  }

  for (int x= 0; x < 10000; x++)
  {
    char key[10];
    sprintf(key, "%d", x);

    uint32_t server_idx = memcached_generate_hash(memc, key, strlen(key));
    char *hostname = memc->hosts[server_idx].hostname;
    in_port_t port = memc->hosts[server_idx].port;
    fprintf(fp, "key %s is on host /%s:%u\n", key, hostname, port);
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, host_index);
  }
  fclose(fp);
#endif
  memcached_server_list_free(server_pool);
  memcached_free(memc);

  return TEST_SUCCESS;
}


static test_return_t result_static(memcached_st *memc)
{
  memcached_result_st result;
  memcached_result_st *result_ptr;

  result_ptr= memcached_result_create(memc, &result);
  test_true(result.options.is_allocated == false);
  test_true(memcached_is_initialized(&result) == true);
  test_true(result_ptr);
  test_true(result_ptr == &result);

  memcached_result_free(&result);

  test_true(result.options.is_allocated == false);
  test_true(memcached_is_initialized(&result) == false);

  return TEST_SUCCESS;
}

static test_return_t result_alloc(memcached_st *memc)
{
  memcached_result_st *result_ptr;

  result_ptr= memcached_result_create(memc, NULL);
  test_true(result_ptr);
  test_true(result_ptr->options.is_allocated == true);
  test_true(memcached_is_initialized(result_ptr) == true);
  memcached_result_free(result_ptr);

  return TEST_SUCCESS;
}

static test_return_t string_static_null(memcached_st *memc)
{
  memcached_string_st string;
  memcached_string_st *string_ptr;

  string_ptr= memcached_string_create(memc, &string, 0);
  test_true(string.options.is_initialized == true);
  test_true(string_ptr);

  /* The following two better be the same! */
  test_true(memcached_is_allocated(string_ptr) == false);
  test_true(memcached_is_allocated(&string) == false);
  test_true(&string == string_ptr);

  test_true(string.options.is_initialized == true);
  test_true(memcached_is_initialized(&string) == true);
  memcached_string_free(&string);
  test_true(memcached_is_initialized(&string) == false);

  return TEST_SUCCESS;
}

static test_return_t string_alloc_null(memcached_st *memc)
{
  memcached_string_st *string;

  string= memcached_string_create(memc, NULL, 0);
  test_true(string);
  test_true(memcached_is_allocated(string) == true);
  test_true(memcached_is_initialized(string) == true);
  memcached_string_free(string);

  return TEST_SUCCESS;
}

static test_return_t string_alloc_with_size(memcached_st *memc)
{
  memcached_string_st *string;

  string= memcached_string_create(memc, NULL, 1024);
  test_true(string);
  test_true(memcached_is_allocated(string) == true);
  test_true(memcached_is_initialized(string) == true);
  memcached_string_free(string);

  return TEST_SUCCESS;
}

static test_return_t string_alloc_with_size_toobig(memcached_st *memc)
{
  memcached_string_st *string;

  string= memcached_string_create(memc, NULL, SIZE_MAX);
  test_true(string == NULL);

  return TEST_SUCCESS;
}

static test_return_t string_alloc_append(memcached_st *memc)
{
  unsigned int x;
  char buffer[SMALL_STRING_LEN];
  memcached_string_st *string;

  /* Ring the bell! */
  memset(buffer, 6, SMALL_STRING_LEN);

  string= memcached_string_create(memc, NULL, 100);
  test_true(string);
  test_true(memcached_is_allocated(string) == true);
  test_true(memcached_is_initialized(string) == true);

  for (x= 0; x < 1024; x++)
  {
    memcached_return_t rc;
    rc= memcached_string_append(string, buffer, SMALL_STRING_LEN);
    test_true(rc == MEMCACHED_SUCCESS);
  }
  test_true(memcached_is_allocated(string) == true);
  memcached_string_free(string);

  return TEST_SUCCESS;
}

static test_return_t string_alloc_append_toobig(memcached_st *memc)
{
  memcached_return_t rc;
  unsigned int x;
  char buffer[SMALL_STRING_LEN];
  memcached_string_st *string;

  /* Ring the bell! */
  memset(buffer, 6, SMALL_STRING_LEN);

  string= memcached_string_create(memc, NULL, 100);
  test_true(string);
  test_true(memcached_is_allocated(string) == true);
  test_true(memcached_is_initialized(string) == true);

  for (x= 0; x < 1024; x++)
  {
    rc= memcached_string_append(string, buffer, SMALL_STRING_LEN);
    test_true(rc == MEMCACHED_SUCCESS);
  }
  rc= memcached_string_append(string, buffer, SIZE_MAX);
  test_true(rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE);
  test_true(memcached_is_allocated(string) == true);
  memcached_string_free(string);

  return TEST_SUCCESS;
}

static test_return_t cleanup_pairs(memcached_st *memc __attribute__((unused)))
{
  pairs_free(global_pairs);

  return TEST_SUCCESS;
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

  return TEST_SUCCESS;
}

static test_return_t generate_large_pairs(memcached_st *memc __attribute__((unused)))
{
  global_pairs= pairs_generate(GLOBAL2_COUNT, MEMCACHED_MAX_BUFFER+10);
  global_count= GLOBAL2_COUNT;

  for (size_t x= 0; x < global_count; x++)
  {
    global_keys[x]= global_pairs[x].key;
    global_keys_length[x]=  global_pairs[x].key_length;
  }

  return TEST_SUCCESS;
}

static test_return_t generate_data(memcached_st *memc)
{
  execute_set(memc, global_pairs, global_count);

  return TEST_SUCCESS;
}

static test_return_t generate_data_with_stats(memcached_st *memc)
{
  memcached_stat_st *stat_p;
  memcached_return_t rc;
  uint32_t host_index= 0;
  execute_set(memc, global_pairs, global_count);

  //TODO: hosts used size stats
  stat_p= memcached_stat(memc, NULL, &rc);
  test_true(stat_p);

  for (host_index= 0; host_index < SERVERS_TO_CREATE; host_index++)
  {
    /* This test was changes so that "make test" would work properlly */
#ifdef DEBUG
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, host_index);

    printf("\nserver %u|%s|%u bytes: %llu\n", host_index, instance->hostname, instance->port, (unsigned long long)(stat_p + host_index)->bytes);
#endif
    test_true((unsigned long long)(stat_p + host_index)->bytes);
  }

  memcached_stat_free(NULL, stat_p);

  return TEST_SUCCESS;
}
static test_return_t generate_buffer_data(memcached_st *memc)
{
  size_t latch= 0;

  latch= 1;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, latch);
  generate_data(memc);

  return TEST_SUCCESS;
}

static test_return_t get_read_count(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_st *memc_clone;

  memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);

  memcached_server_add_with_weight(memc_clone, "localhost", 6666, 0);

  {
    char *return_value;
    size_t return_value_length;
    uint32_t flags;
    uint32_t count;

    for (size_t x= count= 0; x < global_count; x++)
    {
      return_value= memcached_get(memc_clone, global_keys[x], global_keys_length[x],
                                  &return_value_length, &flags, &rc);
      if (rc == MEMCACHED_SUCCESS)
      {
        count++;
        if (return_value)
          free(return_value);
      }
    }
  }

  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

static test_return_t get_read(memcached_st *memc)
{
  memcached_return_t rc;

  {
    char *return_value;
    size_t return_value_length;
    uint32_t flags;

    for (size_t x= 0; x < global_count; x++)
    {
      return_value= memcached_get(memc, global_keys[x], global_keys_length[x],
                                  &return_value_length, &flags, &rc);
      /*
      test_true(return_value);
      test_true(rc == MEMCACHED_SUCCESS);
    */
      if (rc == MEMCACHED_SUCCESS && return_value)
        free(return_value);
    }
  }

  return TEST_SUCCESS;
}

static test_return_t mget_read(memcached_st *memc)
{
  memcached_return_t rc;

  rc= memcached_mget(memc, global_keys, global_keys_length, global_count);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(fetch_all_results(memc) == TEST_SUCCESS);

  return TEST_SUCCESS;
}

static test_return_t mget_read_result(memcached_st *memc)
{
  memcached_return_t rc;

  rc= memcached_mget(memc, global_keys, global_keys_length, global_count);
  test_true(rc == MEMCACHED_SUCCESS);
  /* Turn this into a help function */
  {
    memcached_result_st results_obj;
    memcached_result_st *results;

    results= memcached_result_create(memc, &results_obj);

    while ((results= memcached_fetch_result(memc, &results_obj, &rc)))
    {
      test_true(results);
      test_true(rc == MEMCACHED_SUCCESS);
    }

    memcached_result_free(&results_obj);
  }

  return TEST_SUCCESS;
}

static test_return_t mget_read_function(memcached_st *memc)
{
  memcached_return_t rc;
  size_t counter;
  memcached_execute_fn callbacks[1];

  rc= memcached_mget(memc, global_keys, global_keys_length, global_count);
  test_true(rc == MEMCACHED_SUCCESS);

  callbacks[0]= &callback_counter;
  counter= 0;
  rc= memcached_fetch_execute(memc, callbacks, (void *)&counter, 1);

  return TEST_SUCCESS;
}

static test_return_t delete_generate(memcached_st *memc)
{
  for (size_t x= 0; x < global_count; x++)
  {
    (void)memcached_delete(memc, global_keys[x], global_keys_length[x], (time_t)0);
  }

  return TEST_SUCCESS;
}

static test_return_t delete_buffer_generate(memcached_st *memc)
{
  uint64_t latch= 0;

  latch= 1;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, latch);

  for (size_t x= 0; x < global_count; x++)
  {
    (void)memcached_delete(memc, global_keys[x], global_keys_length[x], (time_t)0);
  }

  return TEST_SUCCESS;
}

static test_return_t add_host_test1(memcached_st *memc)
{
  memcached_return_t rc;
  char servername[]= "0.example.com";
  memcached_server_st *servers;

  servers= memcached_server_list_append_with_weight(NULL, servername, 400, 0, &rc);
  test_true(servers);
  test_true(1 == memcached_server_list_count(servers));

  for (size_t x= 2; x < 20; x++)
  {
    char buffer[SMALL_STRING_LEN];

    snprintf(buffer, SMALL_STRING_LEN, "%zu.example.com", 400+x);
    servers= memcached_server_list_append_with_weight(servers, buffer, 401, 0,
                                     &rc);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(x == memcached_server_list_count(servers));
  }

  rc= memcached_server_push(memc, servers);
  test_true(rc == MEMCACHED_SUCCESS);
  rc= memcached_server_push(memc, servers);
  test_true(rc == MEMCACHED_SUCCESS);

  memcached_server_list_free(servers);

  return TEST_SUCCESS;
}

static test_return_t pre_nonblock(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);

  return TEST_SUCCESS;
}

static test_return_t pre_cork(memcached_st *memc)
{
  memcached_return_t rc;
  bool set= true;

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CORK, set);

#ifdef __APPLE__
  return TEST_SKIPPED;
#endif

  if (rc == MEMCACHED_SUCCESS)
    return TEST_SUCCESS;

  return TEST_SKIPPED;
}

static test_return_t pre_cork_and_nonblock(memcached_st *memc)
{
  test_return_t rc;

  rc= pre_cork(memc);

#ifdef __APPLE__
  return TEST_SKIPPED;
#endif

  if (rc != TEST_SUCCESS)
    return rc;

  return pre_nonblock(memc);
}

static test_return_t pre_nonblock_binary(memcached_st *memc)
{
  memcached_return_t rc= MEMCACHED_FAILURE;
  memcached_st *memc_clone;

  memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);
  // The memcached_version needs to be done on a clone, because the server
  // will not toggle protocol on an connection.
  memcached_version(memc_clone);

  if (libmemcached_util_version_check(memc_clone, 1, 3, 0))
  {
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);
    rc = memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) == 1);
  }
  else
  {
    return TEST_SKIPPED;
  }

  memcached_free(memc_clone);

  return rc == MEMCACHED_SUCCESS ? TEST_SUCCESS : TEST_SKIPPED;
}

static test_return_t pre_murmur(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_MURMUR);

  return TEST_SUCCESS;
}

static test_return_t pre_jenkins(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_JENKINS);

  return TEST_SUCCESS;
}


static test_return_t pre_md5(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_MD5);

  return TEST_SUCCESS;
}

static test_return_t pre_crc(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_CRC);

  return TEST_SUCCESS;
}

static test_return_t pre_hsieh(memcached_st *memc)
{
#ifdef HAVE_HSIEH_HASH
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_HSIEH);
  return TEST_SUCCESS;
#else
  (void) memc;
  return TEST_SKIPPED;
#endif
}

static test_return_t pre_hash_fnv1_64(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_MURMUR);

  return TEST_SUCCESS;
}

static test_return_t pre_hash_fnv1a_64(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_FNV1A_64);

  return TEST_SUCCESS;
}

static test_return_t pre_hash_fnv1_32(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_FNV1_32);

  return TEST_SUCCESS;
}

static test_return_t pre_hash_fnv1a_32(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, (uint64_t)MEMCACHED_HASH_FNV1A_32);

  return TEST_SUCCESS;
}

static test_return_t pre_behavior_ketama(memcached_st *memc)
{
  memcached_return_t rc;
  uint64_t value;

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA);
  test_true(value == 1);

  return TEST_SUCCESS;
}

static test_return_t pre_behavior_ketama_weighted(memcached_st *memc)
{
  memcached_return_t rc;
  uint64_t value;

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  test_true(value == 1);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH, MEMCACHED_HASH_MD5);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_HASH);
  test_true(value == MEMCACHED_HASH_MD5);

  return TEST_SUCCESS;
}

/**
  @note This should be testing to see if the server really supports the binary protocol.
*/
static test_return_t pre_binary(memcached_st *memc)
{
  memcached_return_t rc= MEMCACHED_FAILURE;

  if (libmemcached_util_version_check(memc, 1, 3, 0))
  {
    rc = memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) == 1);
  }

  return rc == MEMCACHED_SUCCESS ? TEST_SUCCESS : TEST_SKIPPED;
}

static test_return_t pre_sasl(memcached_st *memc)
{
  memcached_return_t rc= MEMCACHED_FAILURE;

#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
  const char *server= getenv("LIBMEMCACHED_TEST_SASL_SERVER");
  const char *user= getenv("LIBMEMCACHED_TEST_SASL_USERNAME");
  const char *pass= getenv("LIBMEMCACHED_TEST_SASL_PASSWORD");

  if (server != NULL && user != NULL && pass != NULL)
  {
    memcached_server_st *servers= memcached_servers_parse(server);
    test_true(servers != NULL);
    memcached_servers_reset(memc);
    test_true(memcached_server_push(memc, servers) == MEMCACHED_SUCCESS);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    rc= memcached_set_sasl_auth_data(memc, user, pass);
    test_true(rc == MEMCACHED_SUCCESS);
  }
#else
  (void)memc;
#endif

  return rc == MEMCACHED_SUCCESS ? TEST_SUCCESS : TEST_SKIPPED;
}

static test_return_t pre_replication(memcached_st *memc)
{
  test_return_t test_rc;
  test_rc= pre_binary(memc);

  if (test_rc != TEST_SUCCESS)
    return test_rc;

  /*
   * Make sure that we store the item on all servers
   * (master + replicas == number of servers)
   */
  memcached_return_t rc;
  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS,
                             memcached_server_count(memc) - 1);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS) == memcached_server_count(memc) - 1);

  return rc == MEMCACHED_SUCCESS ? TEST_SUCCESS : TEST_SKIPPED;
}


static test_return_t pre_replication_noblock(memcached_st *memc)
{
  test_return_t rc;

  rc= pre_replication(memc);
  if (rc != TEST_SUCCESS)
    return rc;

  rc= pre_nonblock(memc);

  return rc;
}


static void my_free(const memcached_st *ptr __attribute__((unused)), void *mem, void *context)
{
  (void) context;
#ifdef HARD_MALLOC_TESTS
  void *real_ptr= (mem == NULL) ? mem : (void*)((caddr_t)mem - 8);
  free(real_ptr);
#else
  free(mem);
#endif
}


static void *my_malloc(const memcached_st *ptr __attribute__((unused)), const size_t size, void *context)
{
  (void)context;
#ifdef HARD_MALLOC_TESTS
  void *ret= malloc(size + 8);
  if (ret != NULL)
  {
    ret= (void*)((caddr_t)ret + 8);
  }
#else
  void *ret= malloc(size);
#endif

  if (ret != NULL)
  {
    memset(ret, 0xff, size);
  }

  return ret;
}


static void *my_realloc(const memcached_st *ptr __attribute__((unused)), void *mem, const size_t size, void *context)
{
  (void)context;
#ifdef HARD_MALLOC_TESTS
  void *real_ptr= (mem == NULL) ? NULL : (void*)((caddr_t)mem - 8);
  void *nmem= realloc(real_ptr, size + 8);

  void *ret= NULL;
  if (nmem != NULL)
  {
    ret= (void*)((caddr_t)nmem + 8);
  }

  return ret;
#else
  return realloc(mem, size);
#endif
}


static void *my_calloc(const memcached_st *ptr __attribute__((unused)), size_t nelem, const size_t size, void *context)
{
  (void)context;
#ifdef HARD_MALLOC_TESTS
  void *mem= my_malloc(ptr, nelem * size);
  if (mem)
  {
    memset(mem, 0, nelem * size);
  }

  return mem;
#else
  return calloc(nelem, size);
#endif
}


static test_return_t set_prefix(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "mine";
  char *value;

  /* Make sure be default none exists */
  value= memcached_callback_get(memc, MEMCACHED_CALLBACK_PREFIX_KEY, &rc);
  test_true(rc == MEMCACHED_FAILURE);

  /* Test a clean set */
  rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_PREFIX_KEY, (void *)key);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_callback_get(memc, MEMCACHED_CALLBACK_PREFIX_KEY, &rc);
  test_true(memcmp(value, key, 4) == 0);
  test_true(rc == MEMCACHED_SUCCESS);

  /* Test that we can turn it off */
  rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_PREFIX_KEY, NULL);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_callback_get(memc, MEMCACHED_CALLBACK_PREFIX_KEY, &rc);
  test_true(rc == MEMCACHED_FAILURE);

  /* Now setup for main test */
  rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_PREFIX_KEY, (void *)key);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_callback_get(memc, MEMCACHED_CALLBACK_PREFIX_KEY, &rc);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(memcmp(value, key, 4) == 0);

  /* Set to Zero, and then Set to something too large */
  {
    char long_key[255];
    memset(long_key, 0, 255);

    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_PREFIX_KEY, NULL);
    test_true(rc == MEMCACHED_SUCCESS);

    value= memcached_callback_get(memc, MEMCACHED_CALLBACK_PREFIX_KEY, &rc);
    test_true(rc == MEMCACHED_FAILURE);
    test_true(value == NULL);

    /* Test a long key for failure */
    /* TODO, extend test to determine based on setting, what result should be */
    strcpy(long_key, "Thisismorethentheallottednumberofcharacters");
    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_PREFIX_KEY, long_key);
    //test_true(rc == MEMCACHED_BAD_KEY_PROVIDED);
    test_true(rc == MEMCACHED_SUCCESS);

    /* Now test a key with spaces (which will fail from long key, since bad key is not set) */
    strcpy(long_key, "This is more then the allotted number of characters");
    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_PREFIX_KEY, long_key);
    test_true(rc == MEMCACHED_BAD_KEY_PROVIDED);

    /* Test for a bad prefix, but with a short key */
    rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_VERIFY_KEY, 1);
    test_true(rc == MEMCACHED_SUCCESS);

    strcpy(long_key, "dog cat");
    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_PREFIX_KEY, long_key);
    test_true(rc == MEMCACHED_BAD_KEY_PROVIDED);
  }

  return TEST_SUCCESS;
}


#ifdef MEMCACHED_ENABLE_DEPRECATED
static test_return_t deprecated_set_memory_alloc(memcached_st *memc)
{
  void *test_ptr= NULL;
  void *cb_ptr= NULL;
  {
    memcached_malloc_fn malloc_cb=
      (memcached_malloc_fn)my_malloc;
    cb_ptr= *(void **)&malloc_cb;
    memcached_return_t rc;

    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_MALLOC_FUNCTION, cb_ptr);
    test_true(rc == MEMCACHED_SUCCESS);
    test_ptr= memcached_callback_get(memc, MEMCACHED_CALLBACK_MALLOC_FUNCTION, &rc);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(test_ptr == cb_ptr);
  }

  {
    memcached_realloc_fn realloc_cb=
      (memcached_realloc_fn)my_realloc;
    cb_ptr= *(void **)&realloc_cb;
    memcached_return_t rc;

    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_REALLOC_FUNCTION, cb_ptr);
    test_true(rc == MEMCACHED_SUCCESS);
    test_ptr= memcached_callback_get(memc, MEMCACHED_CALLBACK_REALLOC_FUNCTION, &rc);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(test_ptr == cb_ptr);
  }

  {
    memcached_free_fn free_cb=
      (memcached_free_fn)my_free;
    cb_ptr= *(void **)&free_cb;
    memcached_return_t rc;

    rc= memcached_callback_set(memc, MEMCACHED_CALLBACK_FREE_FUNCTION, cb_ptr);
    test_true(rc == MEMCACHED_SUCCESS);
    test_ptr= memcached_callback_get(memc, MEMCACHED_CALLBACK_FREE_FUNCTION, &rc);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(test_ptr == cb_ptr);
  }

  return TEST_SUCCESS;
}
#endif


static test_return_t set_memory_alloc(memcached_st *memc)
{
  memcached_return_t rc;
  rc= memcached_set_memory_allocators(memc, NULL, my_free,
                                      my_realloc, my_calloc, NULL);
  test_true(rc == MEMCACHED_FAILURE);

  rc= memcached_set_memory_allocators(memc, my_malloc, my_free,
                                      my_realloc, my_calloc, NULL);

  memcached_malloc_fn mem_malloc;
  memcached_free_fn mem_free;
  memcached_realloc_fn mem_realloc;
  memcached_calloc_fn mem_calloc;
  memcached_get_memory_allocators(memc, &mem_malloc, &mem_free,
                                  &mem_realloc, &mem_calloc);

  test_true(mem_malloc == my_malloc);
  test_true(mem_realloc == my_realloc);
  test_true(mem_calloc == my_calloc);
  test_true(mem_free == my_free);

  return TEST_SUCCESS;
}

static test_return_t enable_consistent_crc(memcached_st *memc)
{
  test_return_t rc;
  memcached_server_distribution_t value= MEMCACHED_DISTRIBUTION_CONSISTENT;
  memcached_hash_t hash;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION, value);
  if ((rc= pre_crc(memc)) != TEST_SUCCESS)
    return rc;

  value= (memcached_server_distribution_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION);
  test_true(value == MEMCACHED_DISTRIBUTION_CONSISTENT);

  hash= (memcached_hash_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH);

  if (hash != MEMCACHED_HASH_CRC)
    return TEST_SKIPPED;

  return TEST_SUCCESS;
}

static test_return_t enable_consistent_hsieh(memcached_st *memc)
{
  test_return_t rc;
  memcached_server_distribution_t value= MEMCACHED_DISTRIBUTION_CONSISTENT;
  memcached_hash_t hash;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION, value);
  if ((rc= pre_hsieh(memc)) != TEST_SUCCESS)
    return rc;

  value= (memcached_server_distribution_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION);
  test_true(value == MEMCACHED_DISTRIBUTION_CONSISTENT);

  hash= (memcached_hash_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_HASH);

  if (hash != MEMCACHED_HASH_HSIEH)
    return TEST_SKIPPED;


  return TEST_SUCCESS;
}

static test_return_t enable_cas(memcached_st *memc)
{
  unsigned int set= 1;

  if (libmemcached_util_version_check(memc, 1, 2, 4))
  {
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, set);

    return TEST_SUCCESS;
  }

  return TEST_SKIPPED;
}

static test_return_t check_for_1_2_3(memcached_st *memc)
{
  memcached_version(memc);

  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  if ((instance->major_version >= 1 && (instance->minor_version == 2 && instance->micro_version >= 4))
      || instance->minor_version > 2)
  {
    return TEST_SUCCESS;
  }

  return TEST_SKIPPED;
}

static test_return_t pre_unix_socket(memcached_st *memc)
{
  memcached_return_t rc;
  struct stat buf;

  memcached_servers_reset(memc);

  if (stat("/tmp/memcached.socket", &buf))
    return TEST_SKIPPED;

  rc= memcached_server_add_unix_socket_with_weight(memc, "/tmp/memcached.socket", 0);

  return ( rc == MEMCACHED_SUCCESS ? TEST_SUCCESS : TEST_FAILURE );
}

static test_return_t pre_nodelay(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, 0);

  return TEST_SUCCESS;
}

static test_return_t pre_settimer(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SND_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 1000);

  return TEST_SUCCESS;
}

static test_return_t poll_timeout(memcached_st *memc)
{
  size_t timeout;

  timeout= 100;

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, timeout);

  timeout= (size_t)memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT);

  test_true(timeout == 100);

  return TEST_SUCCESS;
}

static test_return_t noreply_test(memcached_st *memc)
{
  memcached_return_t ret;
  ret= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 1);
  test_true(ret == MEMCACHED_SUCCESS);
  ret= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1);
  test_true(ret == MEMCACHED_SUCCESS);
  ret= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1);
  test_true(ret == MEMCACHED_SUCCESS);
  test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_NOREPLY) == 1);
  test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS) == 1);
  test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS) == 1);

  for (int count=0; count < 5; ++count)
  {
    for (size_t x= 0; x < 100; ++x)
    {
      char key[10];
      size_t len= (size_t)sprintf(key, "%zu", x);
      switch (count)
      {
      case 0:
        ret= memcached_add(memc, key, len, key, len, 0, 0);
        break;
      case 1:
        ret= memcached_replace(memc, key, len, key, len, 0, 0);
        break;
      case 2:
        ret= memcached_set(memc, key, len, key, len, 0, 0);
        break;
      case 3:
        ret= memcached_append(memc, key, len, key, len, 0, 0);
        break;
      case 4:
        ret= memcached_prepend(memc, key, len, key, len, 0, 0);
        break;
      default:
        test_true(count);
        break;
      }
      test_true(ret == MEMCACHED_SUCCESS || ret == MEMCACHED_BUFFERED);
    }

    /*
    ** NOTE: Don't ever do this in your code! this is not a supported use of the
    ** API and is _ONLY_ done this way to verify that the library works the
    ** way it is supposed to do!!!!
    */
    int no_msg=0;
    for (uint32_t x= 0; x < memcached_server_count(memc); ++x)
    {
      memcached_server_instance_st instance=
        memcached_server_instance_by_position(memc, x);
      no_msg+=(int)(instance->cursor_active);
    }

    test_true(no_msg == 0);
    test_true(memcached_flush_buffers(memc) == MEMCACHED_SUCCESS);

    /*
     ** Now validate that all items was set properly!
     */
    for (size_t x= 0; x < 100; ++x)
    {
      char key[10];

      size_t len= (size_t)sprintf(key, "%zu", x);
      size_t length;
      uint32_t flags;
      char* value=memcached_get(memc, key, strlen(key),
                                &length, &flags, &ret);
      test_true(ret == MEMCACHED_SUCCESS && value != NULL);
      switch (count)
      {
      case 0: /* FALLTHROUGH */
      case 1: /* FALLTHROUGH */
      case 2:
        test_true(strncmp(value, key, len) == 0);
        test_true(len == length);
        break;
      case 3:
        test_true(length == len * 2);
        break;
      case 4:
        test_true(length == len * 3);
        break;
      default:
        test_true(count);
        break;
      }
      free(value);
    }
  }

  /* Try setting an illegal cas value (should not return an error to
   * the caller (because we don't expect a return message from the server)
   */
  const char* keys[]= {"0"};
  size_t lengths[]= {1};
  size_t length;
  uint32_t flags;
  memcached_result_st results_obj;
  memcached_result_st *results;
  ret= memcached_mget(memc, keys, lengths, 1);
  test_true(ret == MEMCACHED_SUCCESS);

  results= memcached_result_create(memc, &results_obj);
  test_true(results);
  results= memcached_fetch_result(memc, &results_obj, &ret);
  test_true(results);
  test_true(ret == MEMCACHED_SUCCESS);
  uint64_t cas= memcached_result_cas(results);
  memcached_result_free(&results_obj);

  ret= memcached_cas(memc, keys[0], lengths[0], keys[0], lengths[0], 0, 0, cas);
  test_true(ret == MEMCACHED_SUCCESS);

  /*
   * The item will have a new cas value, so try to set it again with the old
   * value. This should fail!
   */
  ret= memcached_cas(memc, keys[0], lengths[0], keys[0], lengths[0], 0, 0, cas);
  test_true(ret == MEMCACHED_SUCCESS);
  test_true(memcached_flush_buffers(memc) == MEMCACHED_SUCCESS);
  char* value=memcached_get(memc, keys[0], lengths[0], &length, &flags, &ret);
  test_true(ret == MEMCACHED_SUCCESS && value != NULL);
  free(value);

  return TEST_SUCCESS;
}

static test_return_t analyzer_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_stat_st *memc_stat;
  memcached_analysis_st *report;

  memc_stat= memcached_stat(memc, NULL, &rc);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(memc_stat);

  report= memcached_analyze(memc, memc_stat, &rc);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(report);

  free(report);
  memcached_stat_free(NULL, memc_stat);

  return TEST_SUCCESS;
}

/* Count the objects */
static memcached_return_t callback_dump_counter(const memcached_st *ptr __attribute__((unused)),
                                                const char *key __attribute__((unused)),
                                                size_t key_length __attribute__((unused)),
                                                void *context)
{
  size_t *counter= (size_t *)context;

  *counter= *counter + 1;

  return MEMCACHED_SUCCESS;
}

static test_return_t dump_test(memcached_st *memc)
{
  memcached_return_t rc;
  size_t counter= 0;
  memcached_dump_fn callbacks[1];
  test_return_t main_rc;

  callbacks[0]= &callback_dump_counter;

  /* No support for Binary protocol yet */
  if (memc->flags.binary_protocol)
    return TEST_SUCCESS;

  main_rc= set_test3(memc);

  test_true (main_rc == TEST_SUCCESS);

  rc= memcached_dump(memc, callbacks, (void *)&counter, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  /* We may have more then 32 if our previous flush has not completed */
  test_true(counter >= 32);

  return TEST_SUCCESS;
}

#ifdef HAVE_LIBMEMCACHEDUTIL
static void* connection_release(void *arg)
{
  struct {
    memcached_pool_st* pool;
    memcached_st* mmc;
  } *resource= arg;

  usleep(250);
  assert(memcached_pool_push(resource->pool, resource->mmc) == MEMCACHED_SUCCESS);
  return arg;
}

static test_return_t connection_pool_test(memcached_st *memc)
{
  memcached_pool_st* pool= memcached_pool_create(memc, 5, 10);
  test_true(pool != NULL);
  memcached_st* mmc[10];
  memcached_return_t rc;

  for (size_t x= 0; x < 10; ++x)
  {
    mmc[x]= memcached_pool_pop(pool, false, &rc);
    test_true(mmc[x] != NULL);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  test_true(memcached_pool_pop(pool, false, &rc) == NULL);
  test_true(rc == MEMCACHED_SUCCESS);

  pthread_t tid;
  struct {
    memcached_pool_st* pool;
    memcached_st* mmc;
  } item= { .pool = pool, .mmc = mmc[9] };
  pthread_create(&tid, NULL, connection_release, &item);
  mmc[9]= memcached_pool_pop(pool, true, &rc);
  test_true(rc == MEMCACHED_SUCCESS);
  pthread_join(tid, NULL);
  test_true(mmc[9] == item.mmc);
  const char *key= "key";
  size_t keylen= strlen(key);

  // verify that I can do ops with all connections
  rc= memcached_set(mmc[0], key, keylen, "0", 1, 0, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  for (size_t x= 0; x < 10; ++x)
  {
    uint64_t number_value;
    rc= memcached_increment(mmc[x], key, keylen, 1, &number_value);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(number_value == (x+1));
  }

  // Release them..
  for (size_t x= 0; x < 10; ++x)
  {
    test_true(memcached_pool_push(pool, mmc[x]) == MEMCACHED_SUCCESS);
  }


  /* verify that I can set behaviors on the pool when I don't have all
   * of the connections in the pool. It should however be enabled
   * when I push the item into the pool
   */
  mmc[0]= memcached_pool_pop(pool, false, &rc);
  test_true(mmc[0] != NULL);

  rc= memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK, 9999);
  test_true(rc == MEMCACHED_SUCCESS);

  mmc[1]= memcached_pool_pop(pool, false, &rc);
  test_true(mmc[1] != NULL);

  test_true(memcached_behavior_get(mmc[1], MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK) == 9999);
  test_true(memcached_pool_push(pool, mmc[1]) == MEMCACHED_SUCCESS);
  test_true(memcached_pool_push(pool, mmc[0]) == MEMCACHED_SUCCESS);

  mmc[0]= memcached_pool_pop(pool, false, &rc);
  test_true(memcached_behavior_get(mmc[0], MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK) == 9999);
  test_true(memcached_pool_push(pool, mmc[0]) == MEMCACHED_SUCCESS);


  test_true(memcached_pool_destroy(pool) == memc);
  return TEST_SUCCESS;
}

static test_return_t util_version_test(memcached_st *memc)
{
  bool if_successful;

  if_successful= libmemcached_util_version_check(memc, 0, 0, 0);
  test_true(if_successful == true);

  if_successful= libmemcached_util_version_check(memc, 9, 9, 9);
  test_true(if_successful == false);

  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  memcached_version(memc);

  // We only use one binary when we test, so this should be just fine.
  if_successful= libmemcached_util_version_check(memc, instance->major_version, instance->minor_version, instance->micro_version);
  test_true(if_successful == true);

  if (instance->micro_version > 0)
    if_successful= libmemcached_util_version_check(memc, instance->major_version, instance->minor_version, (uint8_t)(instance->micro_version -1));
  else if (instance->minor_version > 0)
    if_successful= libmemcached_util_version_check(memc, instance->major_version, (uint8_t)(instance->minor_version - 1), instance->micro_version);
  else if (instance->major_version > 0)
    if_successful= libmemcached_util_version_check(memc, (uint8_t)(instance->major_version -1), instance->minor_version, instance->micro_version);

  test_true(if_successful == true);

  if (instance->micro_version > 0)
    if_successful= libmemcached_util_version_check(memc, instance->major_version, instance->minor_version, (uint8_t)(instance->micro_version +1));
  else if (instance->minor_version > 0)
    if_successful= libmemcached_util_version_check(memc, instance->major_version, (uint8_t)(instance->minor_version +1), instance->micro_version);
  else if (instance->major_version > 0)
    if_successful= libmemcached_util_version_check(memc, (uint8_t)(instance->major_version +1), instance->minor_version, instance->micro_version);

  test_true(if_successful == false);

  return TEST_SUCCESS;
}

static test_return_t ping_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  // Test both the version that returns a code, and the one that does not.
  test_true(libmemcached_util_ping(memcached_server_name(instance),
                                   memcached_server_port(instance), NULL));

  test_true(libmemcached_util_ping(memcached_server_name(instance),
                                   memcached_server_port(instance), &rc));

  test_true(rc == MEMCACHED_SUCCESS);

  return TEST_SUCCESS;
}
#endif

static test_return_t replication_set_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_st *memc_clone= memcached_clone(NULL, memc);
  memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, 0);

  rc= memcached_set(memc, "bubba", 5, "0", 1, 0, 0);
  test_true(rc == MEMCACHED_SUCCESS);

  /*
  ** We are using the quiet commands to store the replicas, so we need
  ** to ensure that all of them are processed before we can continue.
  ** In the test we go directly from storing the object to trying to
  ** receive the object from all of the different servers, so we
  ** could end up in a race condition (the memcached server hasn't yet
  ** processed the quiet command from the replication set when it process
  ** the request from the other client (created by the clone)). As a
  ** workaround for that we call memcached_quit to send the quit command
  ** to the server and wait for the response ;-) If you use the test code
  ** as an example for your own code, please note that you shouldn't need
  ** to do this ;-)
  */
  memcached_quit(memc);

  /*
  ** "bubba" should now be stored on all of our servers. We don't have an
  ** easy to use API to address each individual server, so I'll just iterate
  ** through a bunch of "master keys" and I should most likely hit all of the
  ** servers...
  */
  for (int x= 'a'; x <= 'z'; ++x)
  {
    char key[2]= { [0]= (char)x };
    size_t len;
    uint32_t flags;
    char *val= memcached_get_by_key(memc_clone, key, 1, "bubba", 5,
                                    &len, &flags, &rc);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(val != NULL);
    free(val);
  }

  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

static test_return_t replication_get_test(memcached_st *memc)
{
  memcached_return_t rc;

  /*
   * Don't do the following in your code. I am abusing the internal details
   * within the library, and this is not a supported interface.
   * This is to verify correct behavior in the library
   */
  for (uint32_t host= 0; host < memcached_server_count(memc); ++host)
  {
    memcached_st *memc_clone= memcached_clone(NULL, memc);
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc_clone, host);

    ((memcached_server_write_instance_st)instance)->port= 0;

    for (int x= 'a'; x <= 'z'; ++x)
    {
      char key[2]= { [0]= (char)x };
      size_t len;
      uint32_t flags;
      char *val= memcached_get_by_key(memc_clone, key, 1, "bubba", 5,
                                      &len, &flags, &rc);
      test_true(rc == MEMCACHED_SUCCESS);
      test_true(val != NULL);
      free(val);
    }

    memcached_free(memc_clone);
  }

  return TEST_SUCCESS;
}

static test_return_t replication_mget_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_st *memc_clone= memcached_clone(NULL, memc);
  memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, 0);

  const char *keys[]= { "bubba", "key1", "key2", "key3" };
  size_t len[]= { 5, 4, 4, 4 };

  for (size_t x= 0; x< 4; ++x)
  {
    rc= memcached_set(memc, keys[x], len[x], "0", 1, 0, 0);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  /*
  ** We are using the quiet commands to store the replicas, so we need
  ** to ensure that all of them are processed before we can continue.
  ** In the test we go directly from storing the object to trying to
  ** receive the object from all of the different servers, so we
  ** could end up in a race condition (the memcached server hasn't yet
  ** processed the quiet command from the replication set when it process
  ** the request from the other client (created by the clone)). As a
  ** workaround for that we call memcached_quit to send the quit command
  ** to the server and wait for the response ;-) If you use the test code
  ** as an example for your own code, please note that you shouldn't need
  ** to do this ;-)
  */
  memcached_quit(memc);

  /*
   * Don't do the following in your code. I am abusing the internal details
   * within the library, and this is not a supported interface.
   * This is to verify correct behavior in the library
   */
  memcached_result_st result_obj;
  for (uint32_t host= 0; host < memc_clone->number_of_hosts; host++)
  {
    memcached_st *new_clone= memcached_clone(NULL, memc);
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(new_clone, host);
    ((memcached_server_write_instance_st)instance)->port= 0;

    for (int x= 'a'; x <= 'z'; ++x)
    {
      char key[2]= { [0]= (char)x, [1]= 0 };

      rc= memcached_mget_by_key(new_clone, key, 1, keys, len, 4);
      test_true(rc == MEMCACHED_SUCCESS);

      memcached_result_st *results= memcached_result_create(new_clone, &result_obj);
      test_true(results);

      int hits= 0;
      while ((results= memcached_fetch_result(new_clone, &result_obj, &rc)) != NULL)
      {
        hits++;
      }
      test_true(hits == 4);
      memcached_result_free(&result_obj);
    }

    memcached_free(new_clone);
  }

  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

static test_return_t replication_randomize_mget_test(memcached_st *memc)
{
  memcached_result_st result_obj;
  memcached_return_t rc;
  memcached_st *memc_clone= memcached_clone(NULL, memc);
  memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, 3);
  memcached_behavior_set(memc_clone, MEMCACHED_BEHAVIOR_RANDOMIZE_REPLICA_READ, 1);

  const char *keys[]= { "key1", "key2", "key3", "key4", "key5", "key6", "key7" };
  size_t len[]= { 4, 4, 4, 4, 4, 4, 4 };

  for (size_t x= 0; x< 7; ++x)
  {
    rc= memcached_set(memc, keys[x], len[x], "1", 1, 0, 0);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  memcached_quit(memc);

  for (size_t x= 0; x< 7; ++x)
  {
    const char key[2]= { [0]= (const char)x };

    rc= memcached_mget_by_key(memc_clone, key, 1, keys, len, 7);
    test_true(rc == MEMCACHED_SUCCESS);

    memcached_result_st *results= memcached_result_create(memc_clone, &result_obj);
    test_true(results);

    int hits= 0;
    while ((results= memcached_fetch_result(memc_clone, &result_obj, &rc)) != NULL)
    {
      ++hits;
    }
    test_true(hits == 7);
    memcached_result_free(&result_obj);
  }
  memcached_free(memc_clone);
  return TEST_SUCCESS;
}

static test_return_t replication_delete_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_st *memc_clone= memcached_clone(NULL, memc);
  /* Delete the items from all of the servers except 1 */
  uint64_t repl= memcached_behavior_get(memc,
                                        MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, --repl);

  const char *keys[]= { "bubba", "key1", "key2", "key3" };
  size_t len[]= { 5, 4, 4, 4 };

  for (size_t x= 0; x< 4; ++x)
  {
    rc= memcached_delete_by_key(memc, keys[0], len[0], keys[x], len[x], 0);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  /*
   * Don't do the following in your code. I am abusing the internal details
   * within the library, and this is not a supported interface.
   * This is to verify correct behavior in the library
   */
  uint32_t hash= memcached_generate_hash(memc, keys[0], len[0]);
  for (uint32_t x= 0; x < (repl + 1); ++x)
  {
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc_clone, x);

    ((memcached_server_write_instance_st)instance)->port= 0;
    if (++hash == memc_clone->number_of_hosts)
      hash= 0;
  }

  memcached_result_st result_obj;
  for (uint32_t host= 0; host < memc_clone->number_of_hosts; ++host)
  {
    for (size_t x= 'a'; x <= 'z'; ++x)
    {
      const char key[2]= { [0]= (const char)x };

      rc= memcached_mget_by_key(memc_clone, key, 1, keys, len, 4);
      test_true(rc == MEMCACHED_SUCCESS);

      memcached_result_st *results= memcached_result_create(memc_clone, &result_obj);
      test_true(results);

      int hits= 0;
      while ((results= memcached_fetch_result(memc_clone, &result_obj, &rc)) != NULL)
      {
        ++hits;
      }
      test_true(hits == 4);
      memcached_result_free(&result_obj);
    }
  }
  memcached_free(memc_clone);

  return TEST_SUCCESS;
}

#if 0
static test_return_t hash_sanity_test (memcached_st *memc)
{
  (void)memc;

  assert(MEMCACHED_HASH_DEFAULT == MEMCACHED_HASH_DEFAULT);
  assert(MEMCACHED_HASH_MD5 == MEMCACHED_HASH_MD5);
  assert(MEMCACHED_HASH_CRC == MEMCACHED_HASH_CRC);
  assert(MEMCACHED_HASH_FNV1_64 == MEMCACHED_HASH_FNV1_64);
  assert(MEMCACHED_HASH_FNV1A_64 == MEMCACHED_HASH_FNV1A_64);
  assert(MEMCACHED_HASH_FNV1_32 == MEMCACHED_HASH_FNV1_32);
  assert(MEMCACHED_HASH_FNV1A_32 == MEMCACHED_HASH_FNV1A_32);
#ifdef HAVE_HSIEH_HASH
  assert(MEMCACHED_HASH_HSIEH == MEMCACHED_HASH_HSIEH);
#endif
  assert(MEMCACHED_HASH_MURMUR == MEMCACHED_HASH_MURMUR);
  assert(MEMCACHED_HASH_JENKINS == MEMCACHED_HASH_JENKINS);
  assert(MEMCACHED_HASH_MAX == MEMCACHED_HASH_MAX);

  return TEST_SUCCESS;
}
#endif

static test_return_t hsieh_avaibility_test (memcached_st *memc)
{
  memcached_return_t expected_rc= MEMCACHED_FAILURE;
#ifdef HAVE_HSIEH_HASH
  expected_rc= MEMCACHED_SUCCESS;
#endif
  memcached_return_t rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH,
                                                (uint64_t)MEMCACHED_HASH_HSIEH);
  test_true(rc == expected_rc);

  return TEST_SUCCESS;
}

static test_return_t one_at_a_time_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_DEFAULT);
    test_true(one_at_a_time_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t md5_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_MD5);
    test_true(md5_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t crc_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_CRC);
    test_true(crc_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t fnv1_64_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_FNV1_64);
    test_true(fnv1_64_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t fnv1a_64_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_FNV1A_64);
    test_true(fnv1a_64_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t fnv1_32_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;


  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_FNV1_32);
    test_true(fnv1_32_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t fnv1a_32_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_FNV1A_32);
    test_true(fnv1a_32_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t hsieh_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_HSIEH);
    test_true(hsieh_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static test_return_t murmur_run (memcached_st *memc __attribute__((unused)))
{
#ifdef WORDS_BIGENDIAN
  return TEST_SKIPPED;
#else
  uint32_t x;
  const char **ptr;

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_MURMUR);
    test_true(murmur_values[x] == hash_val);
  }

  return TEST_SUCCESS;
#endif
}

static test_return_t jenkins_run (memcached_st *memc __attribute__((unused)))
{
  uint32_t x;
  const char **ptr;


  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash_value(*ptr, strlen(*ptr), MEMCACHED_HASH_JENKINS);
    test_true(jenkins_values[x] == hash_val);
  }

  return TEST_SUCCESS;
}

static uint32_t hash_md5_test_function(const char *string, size_t string_length, void *context)
{
  (void)context;
  return libhashkit_md5(string, string_length);
}

static uint32_t hash_crc_test_function(const char *string, size_t string_length, void *context)
{
  (void)context;
  return libhashkit_crc32(string, string_length);
}

static test_return_t memcached_get_hashkit_test (memcached_st *memc)
{
  uint32_t x;
  const char **ptr;
  const hashkit_st *kit;
  hashkit_st new_kit;
  hashkit_return_t hash_rc;

  uint32_t md5_hosts[]= {4U, 1U, 0U, 1U, 4U, 2U, 0U, 3U, 0U, 0U, 3U, 1U, 0U, 0U, 1U, 3U, 0U, 0U, 0U, 3U, 1U, 0U, 4U, 4U, 3U};
  uint32_t crc_hosts[]= {2U, 4U, 1U, 0U, 2U, 4U, 4U, 4U, 1U, 2U, 3U, 4U, 3U, 4U, 1U, 3U, 3U, 2U, 0U, 0U, 0U, 1U, 2U, 4U, 0U};

  kit= memcached_get_hashkit(memc);

  hashkit_clone(&new_kit, kit);
  hash_rc= hashkit_set_custom_function(&new_kit, hash_md5_test_function, NULL);
  test_true(hash_rc == HASHKIT_SUCCESS);

  memcached_set_hashkit(memc, &new_kit);

  /*
    Verify Setting the hash.
  */
  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= hashkit_digest(kit, *ptr, strlen(*ptr));
    test_true(md5_values[x] == hash_val);
  }


  /*
    Now check memcached_st.
  */
  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash(memc, *ptr, strlen(*ptr));
    test_true(md5_hosts[x] == hash_val);
  }

  hash_rc= hashkit_set_custom_function(&new_kit, hash_crc_test_function, NULL);
  test_true(hash_rc == HASHKIT_SUCCESS);

  memcached_set_hashkit(memc, &new_kit);

  /*
    Verify Setting the hash.
  */
  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= hashkit_digest(kit, *ptr, strlen(*ptr));
    test_true(crc_values[x] == hash_val);
  }

  for (ptr= list_to_hash, x= 0; *ptr; ptr++, x++)
  {
    uint32_t hash_val;

    hash_val= memcached_generate_hash(memc, *ptr, strlen(*ptr));
    test_true(crc_hosts[x] == hash_val);
  }

  return TEST_SUCCESS;
}

/*
  Test case adapted from John Gorman <johngorman2@gmail.com> 

  We are testing the error condition when we connect to a server via memcached_get() 
  but find that the server is not available.
*/
static test_return_t memcached_get_MEMCACHED_ERRNO(memcached_st *memc)
{
  (void)memc;
  memcached_st *tl_memc_h;
  memcached_server_st *servers;

  const char *key= "MemcachedLives";
  size_t len;
  uint32_t flags;
  memcached_return rc;
  char *value;

  // Create a handle.
  tl_memc_h= memcached_create(NULL);
  servers= memcached_servers_parse("localhost:9898,localhost:9899"); // This server should not exist
  memcached_server_push(tl_memc_h, servers);
  memcached_server_list_free(servers);

  // See if memcached is reachable.
  value= memcached_get(tl_memc_h, key, strlen(key), &len, &flags, &rc);

  test_false(value);
  test_true(len == 0);
  test_true(rc == MEMCACHED_ERRNO);

  memcached_free(tl_memc_h);

  return TEST_SUCCESS;
}

/* 
  We connect to a server which exists, but search for a key that does not exist.
*/
static test_return_t memcached_get_MEMCACHED_NOTFOUND(memcached_st *memc)
{
  const char *key= "MemcachedKeyNotEXIST";
  size_t len;
  uint32_t flags;
  memcached_return rc;
  char *value;

  // See if memcached is reachable.
  value= memcached_get(memc, key, strlen(key), &len, &flags, &rc);

  test_false(value);
  test_true(len == 0);
  test_true(rc == MEMCACHED_NOTFOUND);

  return TEST_SUCCESS;
}

/*
  Test case adapted from John Gorman <johngorman2@gmail.com> 

  We are testing the error condition when we connect to a server via memcached_get_by_key() 
  but find that the server is not available.
*/
static test_return_t memcached_get_by_key_MEMCACHED_ERRNO(memcached_st *memc)
{
  (void)memc;
  memcached_st *tl_memc_h;
  memcached_server_st *servers;

  const char *key= "MemcachedLives";
  size_t len;
  uint32_t flags;
  memcached_return rc;
  char *value;

  // Create a handle.
  tl_memc_h= memcached_create(NULL);
  servers= memcached_servers_parse("localhost:9898,localhost:9899"); // This server should not exist
  memcached_server_push(tl_memc_h, servers);
  memcached_server_list_free(servers);

  // See if memcached is reachable.
  value= memcached_get_by_key(tl_memc_h, key, strlen(key), key, strlen(key), &len, &flags, &rc);

  test_false(value);
  test_true(len == 0);
  test_true(rc == MEMCACHED_ERRNO);

  memcached_free(tl_memc_h);

  return TEST_SUCCESS;
}

/* 
  We connect to a server which exists, but search for a key that does not exist.
*/
static test_return_t memcached_get_by_key_MEMCACHED_NOTFOUND(memcached_st *memc)
{
  const char *key= "MemcachedKeyNotEXIST";
  size_t len;
  uint32_t flags;
  memcached_return rc;
  char *value;

  // See if memcached is reachable.
  value= memcached_get_by_key(memc, key, strlen(key), key, strlen(key), &len, &flags, &rc);

  test_false(value);
  test_true(len == 0);
  test_true(rc == MEMCACHED_NOTFOUND);

  return TEST_SUCCESS;
}


static test_return_t ketama_compatibility_libmemcached(memcached_st *trash)
{
  memcached_return_t rc;
  uint64_t value;
  int x;
  memcached_server_st *server_pool;
  memcached_st *memc;

  (void)trash;

  memc= memcached_create(NULL);
  test_true(memc);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  test_true(value == 1);

  test_true(memcached_behavior_set_distribution(memc, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA) == MEMCACHED_SUCCESS);
  test_true(memcached_behavior_get_distribution(memc) == MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA);


  server_pool = memcached_servers_parse("10.0.1.1:11211 600,10.0.1.2:11211 300,10.0.1.3:11211 200,10.0.1.4:11211 350,10.0.1.5:11211 1000,10.0.1.6:11211 800,10.0.1.7:11211 950,10.0.1.8:11211 100");
  memcached_server_push(memc, server_pool);

  /* verify that the server list was parsed okay. */
  test_true(memcached_server_count(memc) == 8);
  test_strcmp(server_pool[0].hostname, "10.0.1.1");
  test_true(server_pool[0].port == 11211);
  test_true(server_pool[0].weight == 600);
  test_strcmp(server_pool[2].hostname, "10.0.1.3");
  test_true(server_pool[2].port == 11211);
  test_true(server_pool[2].weight == 200);
  test_strcmp(server_pool[7].hostname, "10.0.1.8");
  test_true(server_pool[7].port == 11211);
  test_true(server_pool[7].weight == 100);

  /* VDEAAAAA hashes to fffcd1b5, after the last continuum point, and lets
   * us test the boundary wraparound.
   */
  test_true(memcached_generate_hash(memc, (char *)"VDEAAAAA", 8) == memc->continuum[0].index);

  /* verify the standard ketama set. */
  for (x= 0; x < 99; x++)
  {
    uint32_t server_idx = memcached_generate_hash(memc, ketama_test_cases[x].key, strlen(ketama_test_cases[x].key));
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, server_idx);
    const char *hostname = memcached_server_name(instance);

    test_strcmp(hostname, ketama_test_cases[x].server);
  }

  memcached_server_list_free(server_pool);
  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t ketama_compatibility_spymemcached(memcached_st *trash)
{
  memcached_return_t rc;
  uint64_t value;
  int x;
  memcached_server_st *server_pool;
  memcached_st *memc;

  (void)trash;

  memc= memcached_create(NULL);
  test_true(memc);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  value= memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  test_true(value == 1);

  test_true(memcached_behavior_set_distribution(memc, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY) == MEMCACHED_SUCCESS);
  test_true(memcached_behavior_get_distribution(memc) == MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY);

  server_pool = memcached_servers_parse("10.0.1.1:11211 600,10.0.1.2:11211 300,10.0.1.3:11211 200,10.0.1.4:11211 350,10.0.1.5:11211 1000,10.0.1.6:11211 800,10.0.1.7:11211 950,10.0.1.8:11211 100");
  memcached_server_push(memc, server_pool);

  /* verify that the server list was parsed okay. */
  test_true(memcached_server_count(memc) == 8);
  test_strcmp(server_pool[0].hostname, "10.0.1.1");
  test_true(server_pool[0].port == 11211);
  test_true(server_pool[0].weight == 600);
  test_strcmp(server_pool[2].hostname, "10.0.1.3");
  test_true(server_pool[2].port == 11211);
  test_true(server_pool[2].weight == 200);
  test_strcmp(server_pool[7].hostname, "10.0.1.8");
  test_true(server_pool[7].port == 11211);
  test_true(server_pool[7].weight == 100);

  /* VDEAAAAA hashes to fffcd1b5, after the last continuum point, and lets
   * us test the boundary wraparound.
   */
  test_true(memcached_generate_hash(memc, (char *)"VDEAAAAA", 8) == memc->continuum[0].index);

  /* verify the standard ketama set. */
  for (x= 0; x < 99; x++)
  {
    uint32_t server_idx= memcached_generate_hash(memc, ketama_test_cases_spy[x].key, strlen(ketama_test_cases_spy[x].key));

    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, server_idx);

    const char *hostname= memcached_server_name(instance);

    test_strcmp(hostname, ketama_test_cases_spy[x].server);
  }

  memcached_server_list_free(server_pool);
  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t regression_bug_434484(memcached_st *memc)
{
  test_return_t test_rc;
  test_rc= pre_binary(memc);

  if (test_rc != TEST_SUCCESS)
    return test_rc;

  memcached_return_t ret;
  const char *key= "regression_bug_434484";
  size_t keylen= strlen(key);

  ret= memcached_append(memc, key, keylen, key, keylen, 0, 0);
  test_true(ret == MEMCACHED_NOTSTORED);

  size_t size= 2048 * 1024;
  void *data= calloc(1, size);
  test_true(data != NULL);
  ret= memcached_set(memc, key, keylen, data, size, 0, 0);
  test_true(ret == MEMCACHED_E2BIG);
  free(data);

  return TEST_SUCCESS;
}

static test_return_t regression_bug_434843(memcached_st *memc)
{
  test_return_t test_rc;
  test_rc= pre_binary(memc);

  if (test_rc != TEST_SUCCESS)
    return test_rc;

  memcached_return_t rc;
  size_t counter= 0;
  memcached_execute_fn callbacks[1]= { [0]= &callback_counter };

  /*
   * I only want to hit only _one_ server so I know the number of requests I'm
   * sending in the pipleine to the server. Let's try to do a multiget of
   * 1024 (that should satisfy most users don't you think?). Future versions
   * will include a mget_execute function call if you need a higher number.
   */
  uint32_t number_of_hosts= memcached_server_count(memc);
  memc->number_of_hosts= 1;
  const size_t max_keys= 1024;
  char **keys= calloc(max_keys, sizeof(char*));
  size_t *key_length=calloc(max_keys, sizeof(size_t));

  for (size_t x= 0; x < max_keys; ++x)
  {
     char k[251];

     key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%zu", x);
     keys[x]= strdup(k);
     test_true(keys[x] != NULL);
  }

  /*
   * Run two times.. the first time we should have 100% cache miss,
   * and the second time we should have 100% cache hits
   */
  for (size_t y= 0; y < 2; y++)
  {
    rc= memcached_mget(memc, (const char**)keys, key_length, max_keys);
    test_true(rc == MEMCACHED_SUCCESS);
    rc= memcached_fetch_execute(memc, callbacks, (void *)&counter, 1);

    if (y == 0)
    {
      /* The first iteration should give me a 100% cache miss. verify that*/
      char blob[1024]= { 0 };

      test_true(counter == 0);

      for (size_t x= 0; x < max_keys; ++x)
      {
        rc= memcached_add(memc, keys[x], key_length[x],
                          blob, sizeof(blob), 0, 0);
        test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
      }
    }
    else
    {
      /* Verify that we received all of the key/value pairs */
       test_true(counter == max_keys);
    }
  }

  /* Release allocated resources */
  for (size_t x= 0; x < max_keys; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  memc->number_of_hosts= number_of_hosts;

  return TEST_SUCCESS;
}

static test_return_t regression_bug_434843_buffered(memcached_st *memc)
{
  memcached_return_t rc;
  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1);
  test_true(rc == MEMCACHED_SUCCESS);

  return regression_bug_434843(memc);
}

static test_return_t regression_bug_421108(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_stat_st *memc_stat= memcached_stat(memc, NULL, &rc);
  test_true(rc == MEMCACHED_SUCCESS);

  char *bytes= memcached_stat_get_value(memc, memc_stat, "bytes", &rc);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(bytes != NULL);
  char *bytes_read= memcached_stat_get_value(memc, memc_stat,
                                             "bytes_read", &rc);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(bytes_read != NULL);

  char *bytes_written= memcached_stat_get_value(memc, memc_stat,
                                                "bytes_written", &rc);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true(bytes_written != NULL);

  test_true(strcmp(bytes, bytes_read) != 0);
  test_true(strcmp(bytes, bytes_written) != 0);

  /* Release allocated resources */
  free(bytes);
  free(bytes_read);
  free(bytes_written);
  memcached_stat_free(NULL, memc_stat);

  return TEST_SUCCESS;
}

/*
 * The test case isn't obvious so I should probably document why
 * it works the way it does. Bug 442914 was caused by a bug
 * in the logic in memcached_purge (it did not handle the case
 * where the number of bytes sent was equal to the watermark).
 * In this test case, create messages so that we hit that case
 * and then disable noreply mode and issue a new command to
 * verify that it isn't stuck. If we change the format for the
 * delete command or the watermarks, we need to update this
 * test....
 */
static test_return_t regression_bug_442914(memcached_st *memc)
{
  memcached_return_t rc;
  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 1);
  test_true(rc == MEMCACHED_SUCCESS);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);

  uint32_t number_of_hosts= memcached_server_count(memc);
  memc->number_of_hosts= 1;

  char k[250];
  size_t len;

  for (uint32_t x= 0; x < 250; ++x)
  {
     len= (size_t)snprintf(k, sizeof(k), "%0250u", x);
     rc= memcached_delete(memc, k, len, 0);
     test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  (void)snprintf(k, sizeof(k), "%037u", 251U);
  len= strlen(k);

  rc= memcached_delete(memc, k, len, 0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 0);
  test_true(rc == MEMCACHED_SUCCESS);
  rc= memcached_delete(memc, k, len, 0);
  test_true(rc == MEMCACHED_NOTFOUND);

  memc->number_of_hosts= number_of_hosts;

  return TEST_SUCCESS;
}

static test_return_t regression_bug_447342(memcached_st *memc)
{
  memcached_server_instance_st instance_one;
  memcached_server_instance_st instance_two;

  if (memcached_server_count(memc) < 3 || pre_replication(memc) != MEMCACHED_SUCCESS)
    return TEST_SKIPPED;

  memcached_return_t rc;

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS, 2);
  test_true(rc == MEMCACHED_SUCCESS);

  const size_t max_keys= 100;
  char **keys= calloc(max_keys, sizeof(char*));
  size_t *key_length= calloc(max_keys, sizeof(size_t));

  for (size_t x= 0; x < max_keys; ++x)
  {
    char k[251];

    key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%zu", x);
    keys[x]= strdup(k);
    test_true(keys[x] != NULL);
    rc= memcached_set(memc, k, key_length[x], k, key_length[x], 0, 0);
    test_true(rc == MEMCACHED_SUCCESS);
  }

  /*
  ** We are using the quiet commands to store the replicas, so we need
  ** to ensure that all of them are processed before we can continue.
  ** In the test we go directly from storing the object to trying to
  ** receive the object from all of the different servers, so we
  ** could end up in a race condition (the memcached server hasn't yet
  ** processed the quiet command from the replication set when it process
  ** the request from the other client (created by the clone)). As a
  ** workaround for that we call memcached_quit to send the quit command
  ** to the server and wait for the response ;-) If you use the test code
  ** as an example for your own code, please note that you shouldn't need
  ** to do this ;-)
  */
  memcached_quit(memc);

  /* Verify that all messages are stored, and we didn't stuff too much
   * into the servers
   */
  rc= memcached_mget(memc, (const char* const *)keys, key_length, max_keys);
  test_true(rc == MEMCACHED_SUCCESS);

  size_t counter= 0;
  memcached_execute_fn callbacks[1]= { [0]= &callback_counter };
  rc= memcached_fetch_execute(memc, callbacks, (void *)&counter, 1);
  /* Verify that we received all of the key/value pairs */
  test_true(counter == max_keys);

  memcached_quit(memc);
  /*
   * Don't do the following in your code. I am abusing the internal details
   * within the library, and this is not a supported interface.
   * This is to verify correct behavior in the library. Fake that two servers
   * are dead..
   */
  instance_one= memcached_server_instance_by_position(memc, 0);
  instance_two= memcached_server_instance_by_position(memc, 2);
  in_port_t port0= instance_one->port;
  in_port_t port2= instance_two->port;

  ((memcached_server_write_instance_st)instance_one)->port= 0;
  ((memcached_server_write_instance_st)instance_two)->port= 0;

  rc= memcached_mget(memc, (const char* const *)keys, key_length, max_keys);
  test_true(rc == MEMCACHED_SUCCESS);

  counter= 0;
  rc= memcached_fetch_execute(memc, callbacks, (void *)&counter, 1);
  test_true(counter == (unsigned int)max_keys);

  /* restore the memc handle */
  ((memcached_server_write_instance_st)instance_one)->port= port0;
  ((memcached_server_write_instance_st)instance_two)->port= port2;

  memcached_quit(memc);

  /* Remove half of the objects */
  for (size_t x= 0; x < max_keys; ++x)
  {
    if (x & 1)
    {
      rc= memcached_delete(memc, keys[x], key_length[x], 0);
      test_true(rc == MEMCACHED_SUCCESS);
    }
  }

  memcached_quit(memc);
  ((memcached_server_write_instance_st)instance_one)->port= 0;
  ((memcached_server_write_instance_st)instance_two)->port= 0;

  /* now retry the command, this time we should have cache misses */
  rc= memcached_mget(memc, (const char* const *)keys, key_length, max_keys);
  test_true(rc == MEMCACHED_SUCCESS);

  counter= 0;
  rc= memcached_fetch_execute(memc, callbacks, (void *)&counter, 1);
  test_true(counter == (unsigned int)(max_keys >> 1));

  /* Release allocated resources */
  for (size_t x= 0; x < max_keys; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  /* restore the memc handle */
  ((memcached_server_write_instance_st)instance_one)->port= port0;
  ((memcached_server_write_instance_st)instance_two)->port= port2;

  return TEST_SUCCESS;
}

static test_return_t regression_bug_463297(memcached_st *memc)
{
  memcached_st *memc_clone= memcached_clone(NULL, memc); 
  test_true(memc_clone != NULL); 
  test_true(memcached_version(memc_clone) == MEMCACHED_SUCCESS); 

  memcached_server_instance_st instance= 
    memcached_server_instance_by_position(memc_clone, 0); 

  if (instance->major_version > 1 || 
      (instance->major_version == 1 && 
       instance->minor_version > 2)) 
  {
    /* Binary protocol doesn't support deferred delete */
    memcached_st *bin_clone= memcached_clone(NULL, memc);
    test_true(bin_clone != NULL);
    test_true(memcached_behavior_set(bin_clone, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1) == MEMCACHED_SUCCESS);
    test_true(memcached_delete(bin_clone, "foo", 3, 1) == MEMCACHED_INVALID_ARGUMENTS);
    memcached_free(bin_clone);

    memcached_quit(memc_clone);

    /* If we know the server version, deferred delete should fail
     * with invalid arguments */
    test_true(memcached_delete(memc_clone, "foo", 3, 1) == MEMCACHED_INVALID_ARGUMENTS);

    /* If we don't know the server version, we should get a protocol error */
    memcached_return_t rc= memcached_delete(memc, "foo", 3, 1);

    /* but there is a bug in some of the memcached servers (1.4) that treats
     * the counter as noreply so it doesn't send the proper error message
   */
    test_true(rc == MEMCACHED_PROTOCOL_ERROR || rc == MEMCACHED_NOTFOUND || rc == MEMCACHED_CLIENT_ERROR);

    /* And buffered mode should be disabled and we should get protocol error */
    test_true(memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1) == MEMCACHED_SUCCESS);
    rc= memcached_delete(memc, "foo", 3, 1);
    test_true(rc == MEMCACHED_PROTOCOL_ERROR || rc == MEMCACHED_NOTFOUND || rc == MEMCACHED_CLIENT_ERROR);

    /* Same goes for noreply... */
    test_true(memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 1) == MEMCACHED_SUCCESS);
    rc= memcached_delete(memc, "foo", 3, 1);
    test_true(rc == MEMCACHED_PROTOCOL_ERROR || rc == MEMCACHED_NOTFOUND || rc == MEMCACHED_CLIENT_ERROR);

    /* but a normal request should go through (and be buffered) */
    test_true((rc= memcached_delete(memc, "foo", 3, 0)) == MEMCACHED_BUFFERED);
    test_true(memcached_flush_buffers(memc) == MEMCACHED_SUCCESS);

    test_true(memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 0) == MEMCACHED_SUCCESS);
    /* unbuffered noreply should be success */
    test_true(memcached_delete(memc, "foo", 3, 0) == MEMCACHED_SUCCESS);
    /* unbuffered with reply should be not found... */
    test_true(memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 0) == MEMCACHED_SUCCESS);
    test_true(memcached_delete(memc, "foo", 3, 0) == MEMCACHED_NOTFOUND);
  }

  memcached_free(memc_clone);
  return TEST_SUCCESS;
}


/* Test memcached_server_get_last_disconnect
 * For a working server set, shall be NULL
 * For a set of non existing server, shall not be NULL
 */
static test_return_t test_get_last_disconnect(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_server_instance_st disconnected_server;

  /* With the working set of server */
  const char *key= "marmotte";
  const char *value= "milka";

  memcached_reset_last_disconnected_server(memc);
  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

  disconnected_server = memcached_server_get_last_disconnect(memc);
  test_true(disconnected_server == NULL);

  /* With a non existing server */
  memcached_st *mine;
  memcached_server_st *servers;

  const char *server_list= "localhost:9";

  servers= memcached_servers_parse(server_list);
  test_true(servers);
  mine= memcached_create(NULL);
  rc= memcached_server_push(mine, servers);
  test_true(rc == MEMCACHED_SUCCESS);
  memcached_server_list_free(servers);
  test_true(mine);

  rc= memcached_set(mine, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc != MEMCACHED_SUCCESS);

  disconnected_server= memcached_server_get_last_disconnect(mine);
  if (disconnected_server == NULL)
  {
    fprintf(stderr, "RC %s\n", memcached_strerror(mine, rc));
    abort();
  }
  test_true(disconnected_server != NULL);
  test_true(memcached_server_port(disconnected_server)== 9);
  test_true(strncmp(memcached_server_name(disconnected_server),"localhost",9) == 0);

  memcached_quit(mine);
  memcached_free(mine);

  return TEST_SUCCESS;
}

static test_return_t test_verbosity(memcached_st *memc)
{
  memcached_verbosity(memc, 3);

  return TEST_SUCCESS;
}

static test_return_t test_server_failure(memcached_st *memc)
{
  memcached_st *local_memc;
  memcached_server_instance_st instance= memcached_server_instance_by_position(memc, 0);

  local_memc= memcached_create(NULL);

  memcached_server_add(local_memc, memcached_server_name(instance), memcached_server_port(instance));
  memcached_behavior_set(local_memc, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 2);

  uint32_t server_count= memcached_server_count(local_memc);

  test_true(server_count == 1);

  // Disable the server
  instance= memcached_server_instance_by_position(local_memc, 0);
  ((memcached_server_write_instance_st)instance)->server_failure_counter= 2;

  memcached_return_t rc;
  rc= memcached_set(local_memc, "foo", strlen("foo"),
                    NULL, 0,
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SERVER_MARKED_DEAD);

  ((memcached_server_write_instance_st)instance)->server_failure_counter= 0;
  rc= memcached_set(local_memc, "foo", strlen("foo"),
                    NULL, 0,
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);


  memcached_free(local_memc);

  return TEST_SUCCESS;
}

static test_return_t test_cull_servers(memcached_st *memc)
{
  uint32_t count = memcached_server_count(memc);

  // Do not do this in your code, it is not supported.
  memc->servers[1].state.is_dead= true;
  memc->state.is_time_for_rebuild= true;

  uint32_t new_count= memcached_server_count(memc);
  test_true(count == new_count);

#if 0
  test_true(count == new_count + 1 );
#endif

  return TEST_SUCCESS;
}


static memcached_return_t stat_printer(memcached_server_instance_st server,
                                       const char *key, size_t key_length,
                                       const char *value, size_t value_length,
                                       void *context)
{
  (void)server;
  (void)context;
  (void)key;
  (void)key_length;
  (void)value;
  (void)value_length;

  return MEMCACHED_SUCCESS;
}

static test_return_t memcached_stat_execute_test(memcached_st *memc)
{
  memcached_return_t rc= memcached_stat_execute(memc, NULL, stat_printer, NULL);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_stat_execute(memc, "slabs", stat_printer, NULL);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_stat_execute(memc, "items", stat_printer, NULL);
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_stat_execute(memc, "sizes", stat_printer, NULL);
  test_true(rc == MEMCACHED_SUCCESS);

  return TEST_SUCCESS;
}

/*
 * This test ensures that the failure counter isn't incremented during
 * normal termination of the memcached instance.
 */
static test_return_t wrong_failure_counter_test(memcached_st *memc)
{
  memcached_return_t rc;
  memcached_server_instance_st instance;

  /* Set value to force connection to the server */
  const char *key= "marmotte";
  const char *value= "milka";

  /*
   * Please note that I'm abusing the internal structures in libmemcached
   * in a non-portable way and you shouldn't be doing this. I'm only
   * doing this in order to verify that the library works the way it should
   */
  uint32_t number_of_hosts= memcached_server_count(memc);
  memc->number_of_hosts= 1;

  /* Ensure that we are connected to the server by setting a value */
  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);


  instance= memcached_server_instance_by_position(memc, 0);
  /* The test is to see that the memcached_quit doesn't increase the
   * the server failure conter, so let's ensure that it is zero
   * before sending quit
   */
  ((memcached_server_write_instance_st)instance)->server_failure_counter= 0;

  memcached_quit(memc);

  /* Verify that it memcached_quit didn't increment the failure counter
   * Please note that this isn't bullet proof, because an error could
   * occur...
   */
  test_true(instance->server_failure_counter == 0);

  /* restore the instance */
  memc->number_of_hosts= number_of_hosts;

  return TEST_SUCCESS;
}




/*
 * Test that ensures mget_execute does not end into recursive calls that finally fails
 */
static test_return_t regression_bug_490486(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, 1000);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 1);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 3600);

#ifdef __APPLE__
  return TEST_SKIPPED; // My MAC can't handle this test
#endif

  /*
   * I only want to hit _one_ server so I know the number of requests I'm
   * sending in the pipeline.
   */
  uint32_t number_of_hosts= memc->number_of_hosts;
  memc->number_of_hosts= 1;
  size_t max_keys= 20480;


  char **keys= calloc(max_keys, sizeof(char*));
  size_t *key_length=calloc(max_keys, sizeof(size_t));

  /* First add all of the items.. */
  bool slept= false;
  char blob[1024]= { 0 };
  memcached_return rc;
  for (size_t x= 0; x < max_keys; ++x)
  {
    char k[251];
    key_length[x]= (size_t)snprintf(k, sizeof(k), "0200%zu", x);
    keys[x]= strdup(k);
    assert(keys[x] != NULL);
    rc= memcached_set(memc, keys[x], key_length[x], blob, sizeof(blob), 0, 0);
#ifdef __APPLE__
    if (rc == MEMCACHED_SERVER_MARKED_DEAD)
    {
      break; // We are out of business
    }
#endif
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED || rc == MEMCACHED_TIMEOUT); // MEMCACHED_TIMEOUT <-- only observed on OSX

    if (rc == MEMCACHED_TIMEOUT && slept == false)
    {
      x++;
      sleep(1);// We will try to sleep
      slept= true;
    }
    else if (rc == MEMCACHED_TIMEOUT && slept == true)
    {
      // We failed to send everything.
      break;
    }
  }

  if (rc != MEMCACHED_SERVER_MARKED_DEAD)
  {

    /* Try to get all of them with a large multiget */
    size_t counter= 0;
    memcached_execute_function callbacks[1]= { [0]= &callback_counter };
    rc= memcached_mget_execute(memc, (const char**)keys, key_length,
                               (size_t)max_keys, callbacks, &counter, 1);

    assert(rc == MEMCACHED_SUCCESS);
    char* the_value= NULL;
    char the_key[MEMCACHED_MAX_KEY];
    size_t the_key_length;
    size_t the_value_length;
    uint32_t the_flags;

    do {
      the_value= memcached_fetch(memc, the_key, &the_key_length, &the_value_length, &the_flags, &rc);

      if ((the_value!= NULL) && (rc == MEMCACHED_SUCCESS))
      {
        ++counter;
        free(the_value);
      }

    } while ( (the_value!= NULL) && (rc == MEMCACHED_SUCCESS));


    assert(rc == MEMCACHED_END);

    /* Verify that we got all of the items */
    assert(counter == max_keys);
  }

  /* Release all allocated resources */
  for (size_t x= 0; x < max_keys; ++x)
  {
    free(keys[x]);
  }
  free(keys);
  free(key_length);

  memc->number_of_hosts= number_of_hosts;

  return TEST_SUCCESS;
}

static test_return_t regression_bug_583031(memcached_st *unused)
{
  (void)unused;

    memcached_st *memc= memcached_create(NULL);
    assert(memc);
    memcached_server_add(memc, "10.2.3.4", 11211);

    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 1000);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 1000);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SND_TIMEOUT, 1000);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 1000);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, 1000);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 3);

    memcached_return_t rc;
    size_t length;
    uint32_t flags;

    (void)memcached_get(memc, "dsf", 3, &length, &flags, &rc);

    test_true(rc == MEMCACHED_TIMEOUT);

    memcached_free(memc);

    return TEST_SUCCESS;
}

static void memcached_die(memcached_st* mc, memcached_return error, const char* what, uint32_t it)
{
  fprintf(stderr, "Iteration #%u: ", it);

  if(error == MEMCACHED_ERRNO)
  {
    fprintf(stderr, "system error %d from %s: %s\n",
            errno, what, strerror(errno));
  }
  else
  {
    fprintf(stderr, "error %d from %s: %s\n", error, what,
            memcached_strerror(mc, error));
  }
}

#define TEST_CONSTANT_CREATION 200

static test_return_t regression_bug_(memcached_st *memc)
{
  const char *remote_server;
  (void)memc;

  if (! (remote_server= getenv("LIBMEMCACHED_REMOTE_SERVER")))
  {
    return TEST_SKIPPED;
  }

  for (uint32_t x= 0; x < TEST_CONSTANT_CREATION; x++) 
  {
    memcached_st* mc= memcached_create(NULL);
    memcached_return rc;

    rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    if (rc != MEMCACHED_SUCCESS)
    {
      memcached_die(mc, rc, "memcached_behavior_set", x);
    }

    rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_CACHE_LOOKUPS, 1);
    if (rc != MEMCACHED_SUCCESS)
    {
      memcached_die(mc, rc, "memcached_behavior_set", x);
    }

    rc= memcached_server_add(mc, remote_server, 0);
    if (rc != MEMCACHED_SUCCESS)
    {
      memcached_die(mc, rc, "memcached_server_add", x);
    }

    const char *set_key= "akey";
    const size_t set_key_len= strlen(set_key);
    const char *set_value= "a value";
    const size_t set_value_len= strlen(set_value);

    if (rc == MEMCACHED_SUCCESS)
    {
      if (x > 0) 
      {
        size_t get_value_len;
        char *get_value;
        uint32_t get_value_flags;

        get_value= memcached_get(mc, set_key, set_key_len, &get_value_len,
                                 &get_value_flags, &rc);
        if (rc != MEMCACHED_SUCCESS)
        {
          memcached_die(mc, rc, "memcached_get", x);
        }
        else
        {

          if (x != 0 &&
              (get_value_len != set_value_len
               || 0!=strncmp(get_value, set_value, get_value_len)))
          {
            fprintf(stderr, "Values don't match?\n");
            rc= MEMCACHED_FAILURE;
          }
          free(get_value);
        }
      }

      rc= memcached_set(mc,
                        set_key, set_key_len,
                        set_value, set_value_len,
                        0, /* time */
                        0  /* flags */
                       );
      if (rc != MEMCACHED_SUCCESS)
      {
        memcached_die(mc, rc, "memcached_set", x);
      }
    }

    memcached_quit(mc);
    memcached_free(mc);

    if (rc != MEMCACHED_SUCCESS)
    {
      break;
    }
  }

  return TEST_SUCCESS;
}

/*
 * Test that the sasl authentication works. We cannot use the default
 * pool of servers, because that would require that all servers we want
 * to test supports SASL authentication, and that they use the default
 * creds.
 */
static test_return_t sasl_auth_test(memcached_st *memc)
{
#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
  memcached_return_t rc;

  rc= memcached_set(memc, "foo", 3, "bar", 3, (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_SUCCESS);
  test_true((rc= memcached_delete(memc, "foo", 3, 0)) == MEMCACHED_SUCCESS);
  test_true((rc= memcached_destroy_sasl_auth_data(memc)) == MEMCACHED_SUCCESS);
  test_true((rc= memcached_destroy_sasl_auth_data(memc)) == MEMCACHED_FAILURE);
  test_true((rc= memcached_destroy_sasl_auth_data(NULL)) == MEMCACHED_FAILURE);
  memcached_quit(memc);

  rc= memcached_set_sasl_auth_data(memc,
                                   getenv("LIBMEMCACHED_TEST_SASL_USERNAME"),
                                   getenv("LIBMEMCACHED_TEST_SASL_SERVER"));
  test_true(rc == MEMCACHED_SUCCESS);

  rc= memcached_set(memc, "foo", 3, "bar", 3, (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_AUTH_FAILURE);
  test_true(memcached_destroy_sasl_auth_data(memc) == MEMCACHED_SUCCESS);

  memcached_quit(memc);
  return TEST_SUCCESS;
#else
  (void)memc;
  return TEST_FAILURE;
#endif
}

/* Clean the server before beginning testing */
test_st tests[] ={
  {"util_version", 1, (test_callback_fn)util_version_test },
  {"flush", 0, (test_callback_fn)flush_test },
  {"init", 0, (test_callback_fn)init_test },
  {"allocation", 0, (test_callback_fn)allocation_test },
  {"server_list_null_test", 0, (test_callback_fn)server_list_null_test},
  {"server_unsort", 0, (test_callback_fn)server_unsort_test},
  {"server_sort", 0, (test_callback_fn)server_sort_test},
  {"server_sort2", 0, (test_callback_fn)server_sort2_test},
  {"memcached_server_remove", 0, (test_callback_fn)memcached_server_remove_test},
  {"clone_test", 0, (test_callback_fn)clone_test },
  {"connection_test", 0, (test_callback_fn)connection_test},
  {"callback_test", 0, (test_callback_fn)callback_test},
  {"userdata_test", 0, (test_callback_fn)userdata_test},
  {"error", 0, (test_callback_fn)error_test },
  {"set", 0, (test_callback_fn)set_test },
  {"set2", 0, (test_callback_fn)set_test2 },
  {"set3", 0, (test_callback_fn)set_test3 },
  {"dump", 1, (test_callback_fn)dump_test},
  {"add", 1, (test_callback_fn)add_test },
  {"replace", 1, (test_callback_fn)replace_test },
  {"delete", 1, (test_callback_fn)delete_test },
  {"get", 1, (test_callback_fn)get_test },
  {"get2", 0, (test_callback_fn)get_test2 },
  {"get3", 0, (test_callback_fn)get_test3 },
  {"get4", 0, (test_callback_fn)get_test4 },
  {"partial mget", 0, (test_callback_fn)get_test5 },
  {"stats_servername", 0, (test_callback_fn)stats_servername_test },
  {"increment", 0, (test_callback_fn)increment_test },
  {"increment_with_initial", 1, (test_callback_fn)increment_with_initial_test },
  {"decrement", 0, (test_callback_fn)decrement_test },
  {"decrement_with_initial", 1, (test_callback_fn)decrement_with_initial_test },
  {"increment_by_key", 0, (test_callback_fn)increment_by_key_test },
  {"increment_with_initial_by_key", 1, (test_callback_fn)increment_with_initial_by_key_test },
  {"decrement_by_key", 0, (test_callback_fn)decrement_by_key_test },
  {"decrement_with_initial_by_key", 1, (test_callback_fn)decrement_with_initial_by_key_test },
  {"quit", 0, (test_callback_fn)quit_test },
  {"mget", 1, (test_callback_fn)mget_test },
  {"mget_result", 1, (test_callback_fn)mget_result_test },
  {"mget_result_alloc", 1, (test_callback_fn)mget_result_alloc_test },
  {"mget_result_function", 1, (test_callback_fn)mget_result_function },
  {"mget_execute", 1, (test_callback_fn)mget_execute },
  {"mget_end", 0, (test_callback_fn)mget_end },
  {"get_stats", 0, (test_callback_fn)get_stats },
  {"add_host_test", 0, (test_callback_fn)add_host_test },
  {"add_host_test_1", 0, (test_callback_fn)add_host_test1 },
  {"get_stats_keys", 0, (test_callback_fn)get_stats_keys },
  {"version_string_test", 0, (test_callback_fn)version_string_test},
  {"bad_key", 1, (test_callback_fn)bad_key_test },
  {"memcached_server_cursor", 1, (test_callback_fn)memcached_server_cursor_test },
  {"read_through", 1, (test_callback_fn)read_through },
  {"delete_through", 1, (test_callback_fn)delete_through },
  {"noreply", 1, (test_callback_fn)noreply_test},
  {"analyzer", 1, (test_callback_fn)analyzer_test},
#ifdef HAVE_LIBMEMCACHEDUTIL
  {"connectionpool", 1, (test_callback_fn)connection_pool_test },
  {"ping", 1, (test_callback_fn)ping_test },
#endif
  {"test_get_last_disconnect", 1, (test_callback_fn)test_get_last_disconnect},
  {"verbosity", 1, (test_callback_fn)test_verbosity},
  {"test_server_failure", 1, (test_callback_fn)test_server_failure},
  {"cull_servers", 1, (test_callback_fn)test_cull_servers},
  {"memcached_stat_execute", 1, (test_callback_fn)memcached_stat_execute_test},
  {0, 0, 0}
};

test_st behavior_tests[] ={
  {"behavior_test", 0, (test_callback_fn)behavior_test},
  {"MEMCACHED_BEHAVIOR_CORK", 0, (test_callback_fn)MEMCACHED_BEHAVIOR_CORK_test},
  {"MEMCACHED_BEHAVIOR_TCP_KEEPALIVE", 0, (test_callback_fn)MEMCACHED_BEHAVIOR_TCP_KEEPALIVE_test},
  {"MEMCACHED_BEHAVIOR_TCP_KEEPIDLE", 0, (test_callback_fn)MEMCACHED_BEHAVIOR_TCP_KEEPIDLE_test},
  {0, 0, 0}
};

test_st regression_binary_vs_block[] ={
  {"block add", 1, (test_callback_fn)block_add_regression},
  {"binary add", 1, (test_callback_fn)binary_add_regression},
  {0, 0, 0}
};

test_st async_tests[] ={
  {"add", 1, (test_callback_fn)add_wrapper },
  {0, 0, 0}
};

test_st string_tests[] ={
  {"string static with null", 0, (test_callback_fn)string_static_null },
  {"string alloc with null", 0, (test_callback_fn)string_alloc_null },
  {"string alloc with 1K", 0, (test_callback_fn)string_alloc_with_size },
  {"string alloc with malloc failure", 0, (test_callback_fn)string_alloc_with_size_toobig },
  {"string append", 0, (test_callback_fn)string_alloc_append },
  {"string append failure (too big)", 0, (test_callback_fn)string_alloc_append_toobig },
  {0, 0, (test_callback_fn)0}
};

test_st result_tests[] ={
  {"result static", 0, (test_callback_fn)result_static},
  {"result alloc", 0, (test_callback_fn)result_alloc},
  {0, 0, (test_callback_fn)0}
};

test_st version_1_2_3[] ={
  {"append", 0, (test_callback_fn)append_test },
  {"prepend", 0, (test_callback_fn)prepend_test },
  {"cas", 0, (test_callback_fn)cas_test },
  {"cas2", 0, (test_callback_fn)cas2_test },
  {"append_binary", 0, (test_callback_fn)append_binary_test },
  {0, 0, (test_callback_fn)0}
};

test_st user_tests[] ={
  {"user_supplied_bug1", 0, (test_callback_fn)user_supplied_bug1 },
  {"user_supplied_bug2", 0, (test_callback_fn)user_supplied_bug2 },
  {"user_supplied_bug3", 0, (test_callback_fn)user_supplied_bug3 },
  {"user_supplied_bug4", 0, (test_callback_fn)user_supplied_bug4 },
  {"user_supplied_bug5", 1, (test_callback_fn)user_supplied_bug5 },
  {"user_supplied_bug6", 1, (test_callback_fn)user_supplied_bug6 },
  {"user_supplied_bug7", 1, (test_callback_fn)user_supplied_bug7 },
  {"user_supplied_bug8", 1, (test_callback_fn)user_supplied_bug8 },
  {"user_supplied_bug9", 1, (test_callback_fn)user_supplied_bug9 },
  {"user_supplied_bug10", 1, (test_callback_fn)user_supplied_bug10 },
  {"user_supplied_bug11", 1, (test_callback_fn)user_supplied_bug11 },
  {"user_supplied_bug12", 1, (test_callback_fn)user_supplied_bug12 },
  {"user_supplied_bug13", 1, (test_callback_fn)user_supplied_bug13 },
  {"user_supplied_bug14", 1, (test_callback_fn)user_supplied_bug14 },
  {"user_supplied_bug15", 1, (test_callback_fn)user_supplied_bug15 },
  {"user_supplied_bug16", 1, (test_callback_fn)user_supplied_bug16 },
#if !defined(__sun) && !defined(__OpenBSD__)
  /*
  ** It seems to be something weird with the character sets..
  ** value_fetch is unable to parse the value line (iscntrl "fails"), so I
  ** guess I need to find out how this is supposed to work.. Perhaps I need
  ** to run the test in a specific locale (I tried zh_CN.UTF-8 without success,
  ** so just disable the code for now...).
  */
  {"user_supplied_bug17", 1, (test_callback_fn)user_supplied_bug17 },
#endif
  {"user_supplied_bug18", 1, (test_callback_fn)user_supplied_bug18 },
  {"user_supplied_bug19", 1, (test_callback_fn)user_supplied_bug19 },
  {"user_supplied_bug20", 1, (test_callback_fn)user_supplied_bug20 },
  {"user_supplied_bug21", 1, (test_callback_fn)user_supplied_bug21 },
  {"wrong_failure_counter_test", 1, (test_callback_fn)wrong_failure_counter_test},
  {0, 0, (test_callback_fn)0}
};

test_st replication_tests[]= {
  {"set", 1, (test_callback_fn)replication_set_test },
  {"get", 0, (test_callback_fn)replication_get_test },
  {"mget", 0, (test_callback_fn)replication_mget_test },
  {"delete", 0, (test_callback_fn)replication_delete_test },
  {"rand_mget", 0, (test_callback_fn)replication_randomize_mget_test },
  {0, 0, (test_callback_fn)0}
};

/*
 * The following test suite is used to verify that we don't introduce
 * regression bugs. If you want more information about the bug / test,
 * you should look in the bug report at
 *   http://bugs.launchpad.net/libmemcached
 */
test_st regression_tests[]= {
  {"lp:434484", 1, (test_callback_fn)regression_bug_434484 },
  {"lp:434843", 1, (test_callback_fn)regression_bug_434843 },
  {"lp:434843-buffered", 1, (test_callback_fn)regression_bug_434843_buffered },
  {"lp:421108", 1, (test_callback_fn)regression_bug_421108 },
  {"lp:442914", 1, (test_callback_fn)regression_bug_442914 },
  {"lp:447342", 1, (test_callback_fn)regression_bug_447342 },
  {"lp:463297", 1, (test_callback_fn)regression_bug_463297 },
  {"lp:490486", 1, (test_callback_fn)regression_bug_490486 },
  {"lp:583031", 1, (test_callback_fn)regression_bug_583031 },
  {"lp:?", 1, (test_callback_fn)regression_bug_ },
  {0, 0, (test_callback_fn)0}
};

test_st sasl_auth_tests[]= {
  {"sasl_auth", 1, (test_callback_fn)sasl_auth_test },
  {0, 0, (test_callback_fn)0}
};

test_st ketama_compatibility[]= {
  {"libmemcached", 1, (test_callback_fn)ketama_compatibility_libmemcached },
  {"spymemcached", 1, (test_callback_fn)ketama_compatibility_spymemcached },
  {0, 0, (test_callback_fn)0}
};

test_st generate_tests[] ={
  {"generate_pairs", 1, (test_callback_fn)generate_pairs },
  {"generate_data", 1, (test_callback_fn)generate_data },
  {"get_read", 0, (test_callback_fn)get_read },
  {"delete_generate", 0, (test_callback_fn)delete_generate },
  {"generate_buffer_data", 1, (test_callback_fn)generate_buffer_data },
  {"delete_buffer", 0, (test_callback_fn)delete_buffer_generate},
  {"generate_data", 1, (test_callback_fn)generate_data },
  {"mget_read", 0, (test_callback_fn)mget_read },
  {"mget_read_result", 0, (test_callback_fn)mget_read_result },
  {"mget_read_function", 0, (test_callback_fn)mget_read_function },
  {"cleanup", 1, (test_callback_fn)cleanup_pairs },
  {"generate_large_pairs", 1, (test_callback_fn)generate_large_pairs },
  {"generate_data", 1, (test_callback_fn)generate_data },
  {"generate_buffer_data", 1, (test_callback_fn)generate_buffer_data },
  {"cleanup", 1, (test_callback_fn)cleanup_pairs },
  {0, 0, (test_callback_fn)0}
};

test_st consistent_tests[] ={
  {"generate_pairs", 1, (test_callback_fn)generate_pairs },
  {"generate_data", 1, (test_callback_fn)generate_data },
  {"get_read", 0, (test_callback_fn)get_read_count },
  {"cleanup", 1, (test_callback_fn)cleanup_pairs },
  {0, 0, (test_callback_fn)0}
};

test_st consistent_weighted_tests[] ={
  {"generate_pairs", 1, (test_callback_fn)generate_pairs },
  {"generate_data", 1, (test_callback_fn)generate_data_with_stats },
  {"get_read", 0, (test_callback_fn)get_read_count },
  {"cleanup", 1, (test_callback_fn)cleanup_pairs },
  {0, 0, (test_callback_fn)0}
};

test_st hsieh_availability[] ={
  {"hsieh_avaibility_test", 0, (test_callback_fn)hsieh_avaibility_test},
  {0, 0, (test_callback_fn)0}
};

#if 0
test_st hash_sanity[] ={
  {"hash sanity", 0, (test_callback_fn)hash_sanity_test},
  {0, 0, (test_callback_fn)0}
};
#endif

test_st ketama_auto_eject_hosts[] ={
  {"auto_eject_hosts", 1, (test_callback_fn)auto_eject_hosts },
  {"output_ketama_weighted_keys", 1, (test_callback_fn)output_ketama_weighted_keys },
  {0, 0, (test_callback_fn)0}
};

test_st hash_tests[] ={
  {"one_at_a_time_run", 0, (test_callback_fn)one_at_a_time_run },
  {"md5", 0, (test_callback_fn)md5_run },
  {"crc", 0, (test_callback_fn)crc_run },
  {"fnv1_64", 0, (test_callback_fn)fnv1_64_run },
  {"fnv1a_64", 0, (test_callback_fn)fnv1a_64_run },
  {"fnv1_32", 0, (test_callback_fn)fnv1_32_run },
  {"fnv1a_32", 0, (test_callback_fn)fnv1a_32_run },
  {"hsieh", 0, (test_callback_fn)hsieh_run },
  {"murmur", 0, (test_callback_fn)murmur_run },
  {"jenkis", 0, (test_callback_fn)jenkins_run },
  {"memcached_get_hashkit", 0, (test_callback_fn)memcached_get_hashkit_test },
  {0, 0, (test_callback_fn)0}
};

test_st error_conditions[] ={
  {"memcached_get_MEMCACHED_ERRNO", 0, (test_callback_fn)memcached_get_MEMCACHED_ERRNO },
  {"memcached_get_MEMCACHED_NOTFOUND", 0, (test_callback_fn)memcached_get_MEMCACHED_NOTFOUND },
  {"memcached_get_by_key_MEMCACHED_ERRNO", 0, (test_callback_fn)memcached_get_by_key_MEMCACHED_ERRNO },
  {"memcached_get_by_key_MEMCACHED_NOTFOUND", 0, (test_callback_fn)memcached_get_by_key_MEMCACHED_NOTFOUND },
  {0, 0, (test_callback_fn)0}
};

collection_st collection[] ={
#if 0
  {"hash_sanity", 0, 0, hash_sanity},
#endif
  {"hsieh_availability", 0, 0, hsieh_availability},
  {"block", 0, 0, tests},
  {"binary", (test_callback_fn)pre_binary, 0, tests},
  {"nonblock", (test_callback_fn)pre_nonblock, 0, tests},
  {"nodelay", (test_callback_fn)pre_nodelay, 0, tests},
  {"settimer", (test_callback_fn)pre_settimer, 0, tests},
  {"md5", (test_callback_fn)pre_md5, 0, tests},
  {"crc", (test_callback_fn)pre_crc, 0, tests},
  {"hsieh", (test_callback_fn)pre_hsieh, 0, tests},
  {"jenkins", (test_callback_fn)pre_jenkins, 0, tests},
  {"fnv1_64", (test_callback_fn)pre_hash_fnv1_64, 0, tests},
  {"fnv1a_64", (test_callback_fn)pre_hash_fnv1a_64, 0, tests},
  {"fnv1_32", (test_callback_fn)pre_hash_fnv1_32, 0, tests},
  {"fnv1a_32", (test_callback_fn)pre_hash_fnv1a_32, 0, tests},
  {"ketama", (test_callback_fn)pre_behavior_ketama, 0, tests},
  {"ketama_auto_eject_hosts", (test_callback_fn)pre_behavior_ketama, 0, ketama_auto_eject_hosts},
  {"unix_socket", (test_callback_fn)pre_unix_socket, 0, tests},
  {"unix_socket_nodelay", (test_callback_fn)pre_nodelay, 0, tests},
  {"poll_timeout", (test_callback_fn)poll_timeout, 0, tests},
  {"gets", (test_callback_fn)enable_cas, 0, tests},
  {"consistent_crc", (test_callback_fn)enable_consistent_crc, 0, tests},
  {"consistent_hsieh", (test_callback_fn)enable_consistent_hsieh, 0, tests},
#ifdef MEMCACHED_ENABLE_DEPRECATED
  {"deprecated_memory_allocators", (test_callback_fn)deprecated_set_memory_alloc, 0, tests},
#endif
  {"memory_allocators", (test_callback_fn)set_memory_alloc, 0, tests},
  {"prefix", (test_callback_fn)set_prefix, 0, tests},
  {"sasl_auth", (test_callback_fn)pre_sasl, 0, sasl_auth_tests },
  {"sasl", (test_callback_fn)pre_sasl, 0, tests },
  {"version_1_2_3", (test_callback_fn)check_for_1_2_3, 0, version_1_2_3},
  {"string", 0, 0, string_tests},
  {"result", 0, 0, result_tests},
  {"async", (test_callback_fn)pre_nonblock, 0, async_tests},
  {"async_binary", (test_callback_fn)pre_nonblock_binary, 0, async_tests},
  {"user", 0, 0, user_tests},
  {"generate", 0, 0, generate_tests},
  {"generate_hsieh", (test_callback_fn)pre_hsieh, 0, generate_tests},
  {"generate_ketama", (test_callback_fn)pre_behavior_ketama, 0, generate_tests},
  {"generate_hsieh_consistent", (test_callback_fn)enable_consistent_hsieh, 0, generate_tests},
  {"generate_md5", (test_callback_fn)pre_md5, 0, generate_tests},
  {"generate_murmur", (test_callback_fn)pre_murmur, 0, generate_tests},
  {"generate_jenkins", (test_callback_fn)pre_jenkins, 0, generate_tests},
  {"generate_nonblock", (test_callback_fn)pre_nonblock, 0, generate_tests},
  // Too slow
  {"generate_corked", (test_callback_fn)pre_cork, 0, generate_tests},
  {"generate_corked_and_nonblock", (test_callback_fn)pre_cork_and_nonblock, 0, generate_tests},
  {"consistent_not", 0, 0, consistent_tests},
  {"consistent_ketama", (test_callback_fn)pre_behavior_ketama, 0, consistent_tests},
  {"consistent_ketama_weighted", (test_callback_fn)pre_behavior_ketama_weighted, 0, consistent_weighted_tests},
  {"ketama_compat", 0, 0, ketama_compatibility},
  {"test_hashes", 0, 0, hash_tests},
  {"replication", (test_callback_fn)pre_replication, 0, replication_tests},
  {"replication_noblock", (test_callback_fn)pre_replication_noblock, 0, replication_tests},
  {"regression", 0, 0, regression_tests},
  {"behaviors", 0, 0, behavior_tests},
  {"regression_binary_vs_block", (test_callback_fn)key_setup, (test_callback_fn)key_teardown, regression_binary_vs_block},
  {"error_conditions", 0, 0, error_conditions},
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
