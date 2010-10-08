/*
  C++ to libmemcit
*/
#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libmemcached/memcached.h>

static test_return_t exists_test(void *obj)
{
  Memcached memc;
  (void)obj;
  (void)memc;

  return TEST_SUCCESS;
}

static test_return_t new_test(void *obj)
{
  Memcached *memc= new Memcached;
  (void)obj;

  (void)memc;

  delete memc;

  return TEST_SUCCESS;
}

static test_return_t copy_test(void *obj)
{
  Memcached *memc= new Memcached;
  Memcached *copy(memc);
  (void)obj;

  (void)copy;

  delete memc;

  return TEST_SUCCESS;
}

static test_return_t assign_test(void *obj)
{
  Memcached memc;
  Memcached copy;
  (void)obj;

  copy= memc;

  (void)copy;

  return TEST_SUCCESS;
}

test_st basic[] ={
  { "exists", 0, reinterpret_cast<test_callback_fn>(exists_test) },
  { "new", 0, reinterpret_cast<test_callback_fn>(new_test) },
  { "copy", 0, reinterpret_cast<test_callback_fn>(copy_test) },
  { "assign", 0, reinterpret_cast<test_callback_fn>(assign_test) },
  { 0, 0, 0}
};

collection_st collection[] ={
  {"basic", 0, 0, basic},
  {0, 0, 0, 0}
};

void get_world(world_st *world)
{
  world->collections= collection;
}
