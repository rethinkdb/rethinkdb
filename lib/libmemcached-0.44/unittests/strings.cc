/* libMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <gtest/gtest.h>

#include <libmemcached/common.h>

TEST(memcached_string_st, memcached_create_static)
{
  memcached_string_st string;
  memcached_string_st *string_ptr;

  memcached_st *memc= memcached_create(NULL);
  string_ptr= memcached_string_create(memc, &string, 0);
  ASSERT_TRUE(string.options.is_initialized);
  ASSERT_TRUE(string_ptr);

  /* The following two better be the same! */
  ASSERT_FALSE(memcached_is_allocated(string_ptr));
  ASSERT_FALSE(memcached_is_allocated(&string));
  EXPECT_EQ(&string, string_ptr);

  ASSERT_TRUE(string.options.is_initialized);
  ASSERT_TRUE(memcached_is_initialized(&string));
  memcached_string_free(&string);
  ASSERT_FALSE(memcached_is_initialized(&string));

  memcached_free(memc);
}

TEST(memcached_string_st, memcached_create_null)
{
  memcached_string_st *string;
  memcached_st *memc= memcached_create(NULL);

  string= memcached_string_create(memc, NULL, 0);
  ASSERT_TRUE(string);
  ASSERT_TRUE(memcached_is_allocated(string));
  ASSERT_TRUE(memcached_is_initialized(string));
  memcached_string_free(string);

  memcached_free(memc);
}

TEST(memcached_string_st, string_alloc_with_size)
{
  memcached_string_st *string;
  memcached_st *memc= memcached_create(NULL);

  string= memcached_string_create(memc, NULL, 1024);
  ASSERT_TRUE(string);
  ASSERT_TRUE(memcached_is_allocated(string));
  ASSERT_TRUE(memcached_is_initialized(string));
  memcached_string_free(string);

  memcached_free(memc);
}

TEST(memcached_string_st, string_alloc_with_size_toobig)
{
  memcached_st *memc= memcached_create(NULL);
  memcached_string_st *string;

  string= memcached_string_create(memc, NULL, SIZE_MAX);
  ASSERT_FALSE(string);

  memcached_free(memc);
}

TEST(memcached_string_st, string_alloc_append)
{
  char buffer[SMALL_STRING_LEN];
  memcached_string_st *string;

  memcached_st *memc= memcached_create(NULL);

  /* Ring the bell! */
  memset(buffer, 6, SMALL_STRING_LEN);

  string= memcached_string_create(memc, NULL, 100);
  ASSERT_TRUE(string);
  ASSERT_TRUE(memcached_is_allocated(string));
  ASSERT_TRUE(memcached_is_initialized(string));

  for (uint32_t x= 0; x < 1024; x++)
  {
    memcached_return_t rc;
    rc= memcached_string_append(string, buffer, SMALL_STRING_LEN);
    EXPECT_EQ(rc, MEMCACHED_SUCCESS);
  }
  ASSERT_TRUE(memcached_is_allocated(string));
  memcached_string_free(string);

  memcached_free(memc);
}

TEST(memcached_string_st, string_alloc_append_toobig)
{
  memcached_return_t rc;
  char buffer[SMALL_STRING_LEN];
  memcached_string_st *string;

  memcached_st *memc= memcached_create(NULL);

  /* Ring the bell! */
  memset(buffer, 6, SMALL_STRING_LEN);

  string= memcached_string_create(memc, NULL, 100);
  ASSERT_TRUE(string);
  ASSERT_TRUE(memcached_is_allocated(string));
  ASSERT_TRUE(memcached_is_initialized(string));

  for (uint32_t x= 0; x < 1024; x++)
  {
    rc= memcached_string_append(string, buffer, SMALL_STRING_LEN);
    EXPECT_EQ(rc, MEMCACHED_SUCCESS);
  }
  rc= memcached_string_append(string, buffer, SIZE_MAX);
  EXPECT_EQ(rc, MEMCACHED_MEMORY_ALLOCATION_FAILURE);
  ASSERT_TRUE(memcached_is_allocated(string));
  memcached_string_free(string);

  memcached_free(memc);
}
