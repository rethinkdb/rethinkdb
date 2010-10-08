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

#ifndef CLIENTS_EXECUTE_H
#define CLIENTS_EXECUTE_H

#include <stdio.h>

#include "libmemcached/memcached.h"
#include "generator.h"

unsigned int execute_set(memcached_st *memc, pairs_st *pairs, unsigned int number_of);
unsigned int execute_get(memcached_st *memc, pairs_st *pairs, unsigned int number_of);
unsigned int execute_mget(memcached_st *memc, const char * const *keys, size_t *key_length,
                          unsigned int number_of);
#endif

