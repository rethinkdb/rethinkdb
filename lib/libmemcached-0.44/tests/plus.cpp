/*
  C++ interface test
*/
#include "libmemcached/memcached.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "server.h"

#include "test.h"

#include <string>
#include <iostream>

using namespace std;
using namespace memcache;

extern "C" {
   test_return_t basic_test(memcached_st *memc);
   test_return_t increment_test(memcached_st *memc);
   test_return_t basic_master_key_test(memcached_st *memc);
   test_return_t mget_result_function(memcached_st *memc);
   test_return_t basic_behavior(memcached_st *memc);
   test_return_t mget_test(memcached_st *memc);
   memcached_return_t callback_counter(const memcached_st *,
                                       memcached_result_st *,
                                       void *context);
}

static void populate_vector(vector<char> &vec, const string &str)
{
  vec.reserve(str.length());
  vec.assign(str.begin(), str.end());
}

static void copy_vec_to_string(vector<char> &vec, string &str)
{
  str.clear();
  if (! vec.empty())
  {
    str.assign(vec.begin(), vec.end());
  }
}

test_return_t basic_test(memcached_st *memc)
{
  Memcache foo(memc);
  const string value_set("This is some data");
  std::vector<char> value;
  std::vector<char> test_value;

  populate_vector(value, value_set);

  foo.set("mine", value, 0, 0);
  foo.get("mine", test_value);

  assert((memcmp(&test_value[0], &value[0], test_value.size()) == 0));

  /*
   * Simple test of the exceptions here...this should throw an exception
   * saying that the key is empty.
   */
  try
  {
    foo.set("", value, 0, 0);
  }
  catch (Error &err)
  {
    return TEST_SUCCESS;
  }

  return TEST_FAILURE;
}

test_return_t increment_test(memcached_st *memc)
{
  Memcache mcach(memc);
  bool rc;
  const string key("blah");
  const string inc_value("1");
  std::vector<char> inc_val;
  vector<char> ret_value;
  string ret_string;
  uint64_t int_inc_value;
  uint64_t int_ret_value;

  populate_vector(inc_val, inc_value);

  rc= mcach.set(key, inc_val, 0, 0);
  if (rc == false)
  {
    return TEST_FAILURE;
  }
  mcach.get(key, ret_value);
  if (ret_value.empty())
  {
    return TEST_FAILURE;
  }
  copy_vec_to_string(ret_value, ret_string);

  int_inc_value= uint64_t(atol(inc_value.c_str()));
  int_ret_value= uint64_t(atol(ret_string.c_str()));
  assert(int_ret_value == int_inc_value);

  rc= mcach.increment(key, 1, &int_ret_value);
  assert(rc == true);
  assert(int_ret_value == 2);

  rc= mcach.increment(key, 1, &int_ret_value);
  assert(rc == true);
  assert(int_ret_value == 3);

  rc= mcach.increment(key, 5, &int_ret_value);
  assert(rc == true);
  assert(int_ret_value == 8);

  return TEST_SUCCESS;
}

test_return_t basic_master_key_test(memcached_st *memc)
{
  Memcache foo(memc);
  const string value_set("Data for server A");
  vector<char> value;
  vector<char> test_value;
  const string master_key_a("server-a");
  const string master_key_b("server-b");
  const string key("xyz");

  populate_vector(value, value_set);

  foo.setByKey(master_key_a, key, value, 0, 0);
  foo.getByKey(master_key_a, key, test_value);

  assert((memcmp(&value[0], &test_value[0], value.size()) == 0));

  test_value.clear();

  foo.getByKey(master_key_b, key, test_value);
  assert((memcmp(&value[0], &test_value[0], value.size()) == 0));

  return TEST_SUCCESS;
}

/* Count the results */
memcached_return_t callback_counter(const memcached_st *,
                                    memcached_result_st *,
                                    void *context)
{
  unsigned int *counter= static_cast<unsigned int *>(context);

  *counter= *counter + 1;

  return MEMCACHED_SUCCESS;
}

test_return_t mget_result_function(memcached_st *memc)
{
  Memcache mc(memc);
  bool rc;
  string key1("fudge");
  string key2("son");
  string key3("food");
  vector<string> keys;
  vector< vector<char> *> values;
  vector<char> val1;
  vector<char> val2;
  vector<char> val3;
  populate_vector(val1, key1);
  populate_vector(val2, key2);
  populate_vector(val3, key3);
  keys.reserve(3);
  keys.push_back(key1);
  keys.push_back(key2);
  keys.push_back(key3);
  values.reserve(3);
  values.push_back(&val1);
  values.push_back(&val2);
  values.push_back(&val3);
  unsigned int counter;
  memcached_execute_fn callbacks[1];

  /* We need to empty the server before we continue the test */
  rc= mc.flush(0);
  rc= mc.setAll(keys, values, 50, 9);
  assert(rc == true);

  rc= mc.mget(keys);
  assert(rc == true);

  callbacks[0]= &callback_counter;
  counter= 0;
  rc= mc.fetchExecute(callbacks, static_cast<void *>(&counter), 1);

  assert(counter == 3);

  return TEST_SUCCESS;
}

test_return_t mget_test(memcached_st *memc)
{
  Memcache mc(memc);
  bool rc;
  memcached_return_t mc_rc;
  vector<string> keys;
  vector< vector<char> *> values;
  keys.reserve(3);
  keys.push_back("fudge");
  keys.push_back("son");
  keys.push_back("food");
  vector<char> val1;
  vector<char> val2;
  vector<char> val3;
  populate_vector(val1, "fudge");
  populate_vector(val2, "son");
  populate_vector(val3, "food");
  values.reserve(3);
  values.push_back(&val1);
  values.push_back(&val2);
  values.push_back(&val3);

  string return_key;
  vector<char> return_value;

  /* We need to empty the server before we continue the test */
  rc= mc.flush(0);
  assert(rc == true);

  rc= mc.mget(keys);
  assert(rc == true);

  while ((mc_rc= mc.fetch(return_key, return_value)) != MEMCACHED_END)
  {
    assert(return_value.size() != 0);
    return_value.clear();
  }
  assert(mc_rc == MEMCACHED_END);

  rc= mc.setAll(keys, values, 50, 9);
  assert(rc == true);

  rc= mc.mget(keys);
  assert(rc == true);

  while ((mc_rc= mc.fetch(return_key, return_value)) != MEMCACHED_END)
  {
    assert(return_key.length() == return_value.size());
    assert(!memcmp(&return_value[0], return_key.c_str(), return_value.size()));
  }

  return TEST_SUCCESS;
}

test_return_t basic_behavior(memcached_st *memc)
{
  Memcache mc(memc);
  bool rc;
  uint64_t value = 1;
  rc = mc.setBehavior(MEMCACHED_BEHAVIOR_VERIFY_KEY, value);
  assert(rc);
  uint64_t behavior = mc.getBehavior(MEMCACHED_BEHAVIOR_VERIFY_KEY);
  assert(behavior == value);

  return TEST_SUCCESS;
}

test_st tests[] ={
  { "basic", 0,
    reinterpret_cast<test_callback_fn>(basic_test) },
  { "basic_master_key", 0,
    reinterpret_cast<test_callback_fn>(basic_master_key_test) },
  { "increment_test", 0,
    reinterpret_cast<test_callback_fn>(increment_test) },
  { "mget", 1,
    reinterpret_cast<test_callback_fn>(mget_test) },
  { "mget_result_function", 1,
    reinterpret_cast<test_callback_fn>(mget_result_function) },
  { "basic_behavior", 0,
    reinterpret_cast<test_callback_fn>(basic_behavior) },
  {0, 0, 0}
};

collection_st collection[] ={
  {"block", 0, 0, tests},
  {0, 0, 0, 0}
};

#define SERVERS_TO_CREATE 5

#include "libmemcached_world.h"

void get_world(world_st *world)
{
  world->collections= collection;

  world->create= reinterpret_cast<test_callback_create_fn>(world_create);
  world->destroy= reinterpret_cast<test_callback_fn>(world_destroy);

  world->test.startup= reinterpret_cast<test_callback_fn>(world_test_startup);
  world->test.flush= reinterpret_cast<test_callback_fn>(world_flush);
  world->test.pre_run= reinterpret_cast<test_callback_fn>(world_pre_run);
  world->test.post_run= reinterpret_cast<test_callback_fn>(world_post_run);
  world->test.on_error= reinterpret_cast<test_callback_error_fn>(world_on_error);

  world->collection.startup= reinterpret_cast<test_callback_fn>(world_container_startup);
  world->collection.shutdown= reinterpret_cast<test_callback_fn>(world_container_shutdown);

  world->runner= &defualt_libmemcached_runner;
}
