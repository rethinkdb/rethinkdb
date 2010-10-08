/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: connect to all hosts, and make sure they meet a minimum version
 *
 */

/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#include "libmemcached/common.h"
#include "libmemcached/memcached_util.h"

struct local_context
{
  uint8_t major_version;
  uint8_t minor_version;
  uint8_t micro_version;

  bool truth;
};

static memcached_return_t check_server_version(const memcached_st *ptr __attribute__((unused)),
                                               const memcached_server_st *instance,
                                               void *context)
{
  /* Do Nothing */
  struct local_context *check= (struct local_context *)context;

  if (instance->major_version >= check->major_version &&
      instance->minor_version >= check->minor_version &&
      instance->micro_version >= check->micro_version )
  {
    return MEMCACHED_SUCCESS;
  }

  check->truth= false;

  return MEMCACHED_FAILURE;
}

bool libmemcached_util_version_check(memcached_st *memc, 
                                     uint8_t major_version,
                                     uint8_t minor_version,
                                     uint8_t micro_version)
{
  memcached_server_fn callbacks[1];
  memcached_version(memc);

  struct local_context check= { .major_version= major_version, .minor_version= minor_version, .micro_version= micro_version, .truth= true };
  memcached_version(memc);

  callbacks[0]= check_server_version;
  memcached_server_cursor(memc, callbacks, (void *)&check,  1);

  return check.truth;
}
