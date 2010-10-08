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

#include "libmemcached/common.h"
#include "libmemcached/memcached_util.h"


bool libmemcached_util_ping(const char *hostname, in_port_t port, memcached_return_t *ret)
{
  memcached_return_t rc;
  memcached_st memc, *memc_ptr;

  memc_ptr= memcached_create(&memc);

  rc= memcached_server_add(memc_ptr, hostname, port);

  if (rc == MEMCACHED_SUCCESS)
  {
    rc= memcached_version(memc_ptr);
  }

  memcached_free(memc_ptr);

  if (ret)
  {
    *ret= rc;
  }

  return rc == MEMCACHED_SUCCESS;
}
