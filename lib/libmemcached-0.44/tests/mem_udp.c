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

#include "libmemcached/common.h"

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

#include "server.h"

#include "test.h"

#define SERVERS_TO_CREATE 5


/**
  @note This should be testing to see if the server really supports the binary protocol.
*/
static test_return_t pre_binary(memcached_st *memc)
{
  memcached_return_t rc= MEMCACHED_FAILURE;
  memcached_st *memc_clone;
  memcached_server_instance_st instance;

  memc_clone= memcached_clone(NULL, memc);
  test_true(memc_clone);
  // The memcached_version needs to be done on a clone, because the server
  // will not toggle protocol on an connection.
  memcached_version(memc_clone);

  instance= memcached_server_instance_by_position(memc_clone, 0);

  if (instance->major_version >= 1 && instance->minor_version > 2)
  {
    rc = memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    test_true(rc == MEMCACHED_SUCCESS);
    test_true(memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL) == 1);
  }

  memcached_free(memc_clone);

  return rc == MEMCACHED_SUCCESS ? TEST_SUCCESS : TEST_SKIPPED;
}

static void increment_request_id(uint16_t *id)
{
  (*id)++;
  if ((*id & UDP_REQUEST_ID_THREAD_MASK) != 0)
    *id= 0;
}

static uint16_t *get_udp_request_ids(memcached_st *memc)
{
  uint16_t *ids= malloc(sizeof(uint16_t) * memcached_server_count(memc));
  assert(ids != NULL);

  for (uint32_t x= 0; x < memcached_server_count(memc); x++)
  {
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, x);

    ids[x]= get_udp_datagram_request_id((struct udp_datagram_header_st *) ((memcached_server_instance_st )instance)->write_buffer);
  }

  return ids;
}

static test_return_t post_udp_op_check(memcached_st *memc, uint16_t *expected_req_ids)
{
  (void)memc;
  (void)expected_req_ids;
#if 0
  memcached_server_st *cur_server = memcached_server_list(memc);
  uint16_t *cur_req_ids = get_udp_request_ids(memc);

  for (size_t x= 0; x < memcached_server_count(memc); x++)
  {
    test_true(cur_server[x].cursor_active == 0);
    test_true(cur_req_ids[x] == expected_req_ids[x]);
  }
  free(expected_req_ids);
  free(cur_req_ids);

#endif
  return TEST_SUCCESS;
}

/*
** There is a little bit of a hack here, instead of removing
** the servers, I just set num host to 0 and them add then new udp servers
**/
static test_return_t init_udp(memcached_st *memc)
{
  memcached_version(memc);
#if 0
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);

  /* For the time being, only support udp test for >= 1.2.6 && < 1.3 */
  if (instance->major_version != 1 || instance->minor_version != 2
          || instance->micro_version < 6)
    return TEST_SKIPPED;

  uint32_t num_hosts= memcached_server_count(memc);
  memcached_server_st servers[num_hosts];
  memcpy(servers, memcached_server_list(memc), sizeof(memcached_server_st) * num_hosts);
  for (uint32_t x= 0; x < num_hosts; x++)
  {
   memcached_server_instance_st set_instance=
     memcached_server_instance_by_position(memc, x);

    memcached_server_free(((memcached_server_write_instance_st)set_instance));
  }

  memc->number_of_hosts= 0;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_USE_UDP, 1);
  for (uint32_t x= 0; x < num_hosts; x++)
  {
    memcached_server_instance_st set_instance=
      memcached_server_instance_by_position(memc, x);

    test_true(memcached_server_add_udp(memc, servers[x].hostname, servers[x].port) == MEMCACHED_SUCCESS);
    test_true(set_instance->write_buffer_offset == UDP_DATAGRAM_HEADER_LENGTH);
  }
#endif

  return TEST_SKIPPED;
}

static test_return_t binary_init_udp(memcached_st *memc)
{
  test_return_t test_rc;
  test_rc= pre_binary(memc);

  if (test_rc != TEST_SUCCESS)
    return test_rc;

  return init_udp(memc);
}

/* Make sure that I cant add a tcp server to a udp client */
static test_return_t add_tcp_server_udp_client_test(memcached_st *memc)
{
  (void)memc;
#if 0
  memcached_server_st server;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);
  memcached_server_clone(&server, &memc->hosts[0]);
  test_true(memcached_server_remove(&(memc->hosts[0])) == MEMCACHED_SUCCESS);
  test_true(memcached_server_add(memc, server.hostname, server.port) == MEMCACHED_INVALID_HOST_PROTOCOL);
#endif
  return TEST_SUCCESS;
}

/* Make sure that I cant add a udp server to a tcp client */
static test_return_t add_udp_server_tcp_client_test(memcached_st *memc)
{
  (void)memc;
#if 0
  memcached_server_st server;
  memcached_server_instance_st instance=
    memcached_server_instance_by_position(memc, 0);
  memcached_server_clone(&server, &memc->hosts[0]);
  test_true(memcached_server_remove(&(memc->hosts[0])) == MEMCACHED_SUCCESS);

  memcached_st tcp_client;
  memcached_create(&tcp_client);
  test_true(memcached_server_add_udp(&tcp_client, server.hostname, server.port) == MEMCACHED_INVALID_HOST_PROTOCOL);
#endif

  return TEST_SUCCESS;
}

static test_return_t set_udp_behavior_test(memcached_st *memc)
{

  memcached_quit(memc);
  memc->number_of_hosts= 0;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_DISTRIBUTION, memc->distribution);
  test_true(memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_USE_UDP, 1) == MEMCACHED_SUCCESS);
  test_true(memc->flags.use_udp);
  test_true(memc->flags.no_reply);

  test_true(memcached_server_count(memc) == 0);

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_USE_UDP,0);
  test_true(! (memc->flags.use_udp));
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY,0);
  test_true(! (memc->flags.no_reply));

  return TEST_SUCCESS;
}

static test_return_t udp_set_test(memcached_st *memc)
{
  unsigned int num_iters= 1025; //request id rolls over at 1024

  for (size_t x= 0; x < num_iters;x++)
  {
    memcached_return_t rc;
    const char *key= "foo";
    const char *value= "when we sanitize";
    uint16_t *expected_ids= get_udp_request_ids(memc);
    unsigned int server_key= memcached_generate_hash(memc, key, strlen(key));
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, server_key);
    size_t init_offset= instance->write_buffer_offset;

    rc= memcached_set(memc, key, strlen(key),
                      value, strlen(value),
                      (time_t)0, (uint32_t)0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
    /** NB, the check below assumes that if new write_ptr is less than
     *  the original write_ptr that we have flushed. For large payloads, this
     *  maybe an invalid assumption, but for the small payload we have it is OK
     */
    if (rc == MEMCACHED_SUCCESS ||
            instance->write_buffer_offset < init_offset)
      increment_request_id(&expected_ids[server_key]);

    if (rc == MEMCACHED_SUCCESS)
    {
      test_true(instance->write_buffer_offset == UDP_DATAGRAM_HEADER_LENGTH);
    }
    else
    {
      test_true(instance->write_buffer_offset != UDP_DATAGRAM_HEADER_LENGTH);
      test_true(instance->write_buffer_offset <= MAX_UDP_DATAGRAM_LENGTH);
    }
    test_true(post_udp_op_check(memc, expected_ids) == TEST_SUCCESS);
  }
  return TEST_SUCCESS;
}

static test_return_t udp_buffered_set_test(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1);
  return udp_set_test(memc);
}

static test_return_t udp_set_too_big_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "bar";
  char value[MAX_UDP_DATAGRAM_LENGTH];
  uint16_t *expected_ids= get_udp_request_ids(memc);
  rc= memcached_set(memc, key, strlen(key),
                    value, MAX_UDP_DATAGRAM_LENGTH,
                    (time_t)0, (uint32_t)0);
  test_true(rc == MEMCACHED_WRITE_FAILURE);

  return post_udp_op_check(memc,expected_ids);
}

static test_return_t udp_delete_test(memcached_st *memc)
{
  unsigned int num_iters= 1025; //request id rolls over at 1024

  for (size_t x= 0; x < num_iters;x++)
  {
    memcached_return_t rc;
    const char *key= "foo";
    uint16_t *expected_ids=get_udp_request_ids(memc);
    unsigned int server_key= memcached_generate_hash(memc, key, strlen(key));
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(memc, server_key);
    size_t init_offset= instance->write_buffer_offset;

    rc= memcached_delete(memc, key, strlen(key), 0);
    test_true(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);

    if (rc == MEMCACHED_SUCCESS || instance->write_buffer_offset < init_offset)
      increment_request_id(&expected_ids[server_key]);
    if (rc == MEMCACHED_SUCCESS)
    {
      test_true(instance->write_buffer_offset == UDP_DATAGRAM_HEADER_LENGTH);
    }
    else
    {
      test_true(instance->write_buffer_offset != UDP_DATAGRAM_HEADER_LENGTH);
      test_true(instance->write_buffer_offset <= MAX_UDP_DATAGRAM_LENGTH);
    }
    test_true(post_udp_op_check(memc,expected_ids) == TEST_SUCCESS);
  }
  return TEST_SUCCESS;
}

static test_return_t udp_buffered_delete_test(memcached_st *memc)
{
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1);
  return udp_delete_test(memc);
}

static test_return_t udp_verbosity_test(memcached_st *memc)
{
  memcached_return_t rc;
  uint16_t *expected_ids= get_udp_request_ids(memc);

  for (size_t x= 0; x < memcached_server_count(memc); x++)
  {
    increment_request_id(&expected_ids[x]);
  }

  rc= memcached_verbosity(memc,3);
  test_true(rc == MEMCACHED_SUCCESS);
  return post_udp_op_check(memc,expected_ids);
}

static test_return_t udp_quit_test(memcached_st *memc)
{
  uint16_t *expected_ids= get_udp_request_ids(memc);
  memcached_quit(memc);
  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_flush_test(memcached_st *memc)
{
  memcached_return_t rc;
  uint16_t *expected_ids= get_udp_request_ids(memc);

  for (size_t x= 0; x < memcached_server_count(memc); x++)
  {
    increment_request_id(&expected_ids[x]);
  }

  rc= memcached_flush(memc,0);
  test_true(rc == MEMCACHED_SUCCESS);
  return post_udp_op_check(memc,expected_ids);
}

static test_return_t udp_incr_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "incr";
  const char *value= "1";
  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);

  test_true(rc == MEMCACHED_SUCCESS);
  uint16_t *expected_ids= get_udp_request_ids(memc);
  unsigned int server_key= memcached_generate_hash(memc, key, strlen(key));
  increment_request_id(&expected_ids[server_key]);
  uint64_t newvalue;
  rc= memcached_increment(memc, key, strlen(key), 1, &newvalue);
  test_true(rc == MEMCACHED_SUCCESS);
  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_decr_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "decr";
  const char *value= "1";
  rc= memcached_set(memc, key, strlen(key),
                    value, strlen(value),
                    (time_t)0, (uint32_t)0);

  test_true(rc == MEMCACHED_SUCCESS);
  uint16_t *expected_ids= get_udp_request_ids(memc);
  unsigned int server_key= memcached_generate_hash(memc, key, strlen(key));
  increment_request_id(&expected_ids[server_key]);
  uint64_t newvalue;
  rc= memcached_decrement(memc, key, strlen(key), 1, &newvalue);
  test_true(rc == MEMCACHED_SUCCESS);
  return post_udp_op_check(memc, expected_ids);
}


static test_return_t udp_stat_test(memcached_st *memc)
{
  memcached_stat_st * rv= NULL;
  memcached_return_t rc;
  char args[]= "";
  uint16_t *expected_ids = get_udp_request_ids(memc);
  rv = memcached_stat(memc, args, &rc);
  free(rv);
  test_true(rc == MEMCACHED_NOT_SUPPORTED);
  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_version_test(memcached_st *memc)
{
  memcached_return_t rc;
  uint16_t *expected_ids = get_udp_request_ids(memc);
  rc = memcached_version(memc);
  test_true(rc == MEMCACHED_NOT_SUPPORTED);
  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_get_test(memcached_st *memc)
{
  memcached_return_t rc;
  const char *key= "foo";
  size_t vlen;
  uint16_t *expected_ids = get_udp_request_ids(memc);
  char *val= memcached_get(memc, key, strlen(key), &vlen, (uint32_t)0, &rc);
  test_true(rc == MEMCACHED_NOT_SUPPORTED);
  test_true(val == NULL);
  return post_udp_op_check(memc, expected_ids);
}

static test_return_t udp_mixed_io_test(memcached_st *memc)
{
  test_st current_op;
  test_st mixed_io_ops [] ={
    {"udp_set_test", 0,
      (test_callback_fn)udp_set_test},
    {"udp_set_too_big_test", 0,
      (test_callback_fn)udp_set_too_big_test},
    {"udp_delete_test", 0,
      (test_callback_fn)udp_delete_test},
    {"udp_verbosity_test", 0,
      (test_callback_fn)udp_verbosity_test},
    {"udp_quit_test", 0,
      (test_callback_fn)udp_quit_test},
    {"udp_flush_test", 0,
      (test_callback_fn)udp_flush_test},
    {"udp_incr_test", 0,
      (test_callback_fn)udp_incr_test},
    {"udp_decr_test", 0,
      (test_callback_fn)udp_decr_test},
    {"udp_version_test", 0,
      (test_callback_fn)udp_version_test}
  };

  for (size_t x= 0; x < 500; x++)
  {
    current_op= mixed_io_ops[random() % 9];
    test_true(current_op.test_fn(memc) == TEST_SUCCESS);
  }
  return TEST_SUCCESS;
}

test_st udp_setup_server_tests[] ={
  {"set_udp_behavior_test", 0, (test_callback_fn)set_udp_behavior_test},
  {"add_tcp_server_udp_client_test", 0, (test_callback_fn)add_tcp_server_udp_client_test},
  {"add_udp_server_tcp_client_test", 0, (test_callback_fn)add_udp_server_tcp_client_test},
  {0, 0, 0}
};

test_st upd_io_tests[] ={
  {"udp_set_test", 0, (test_callback_fn)udp_set_test},
  {"udp_buffered_set_test", 0, (test_callback_fn)udp_buffered_set_test},
  {"udp_set_too_big_test", 0, (test_callback_fn)udp_set_too_big_test},
  {"udp_delete_test", 0, (test_callback_fn)udp_delete_test},
  {"udp_buffered_delete_test", 0, (test_callback_fn)udp_buffered_delete_test},
  {"udp_verbosity_test", 0, (test_callback_fn)udp_verbosity_test},
  {"udp_quit_test", 0, (test_callback_fn)udp_quit_test},
  {"udp_flush_test", 0, (test_callback_fn)udp_flush_test},
  {"udp_incr_test", 0, (test_callback_fn)udp_incr_test},
  {"udp_decr_test", 0, (test_callback_fn)udp_decr_test},
  {"udp_stat_test", 0, (test_callback_fn)udp_stat_test},
  {"udp_version_test", 0, (test_callback_fn)udp_version_test},
  {"udp_get_test", 0, (test_callback_fn)udp_get_test},
  {"udp_mixed_io_test", 0, (test_callback_fn)udp_mixed_io_test},
  {0, 0, 0}
};

collection_st collection[] ={
  {"udp_setup", (test_callback_fn)init_udp, 0, udp_setup_server_tests},
  {"udp_io", (test_callback_fn)init_udp, 0, upd_io_tests},
  {"udp_binary_io", (test_callback_fn)binary_init_udp, 0, upd_io_tests},
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
