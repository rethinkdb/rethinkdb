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

#ifndef __CLIENT_OPTIONS_H__
#define __CLIENT_OPTIONS_H__

typedef struct memcached_help_text_st memcached_help_text_st;

typedef enum {
  OPT_SERVERS= 's',
  OPT_VERSION= 'V',
  OPT_HELP= 'h',
  OPT_VERBOSE= 'v',
  OPT_DEBUG= 'd',
  OPT_ANALYZE= 'a',
  OPT_FLAG= 257,
  OPT_EXPIRE,
  OPT_SET,
  OPT_REPLACE,
  OPT_ADD,
  OPT_SLAP_EXECUTE_NUMBER,
  OPT_SLAP_INITIAL_LOAD,
  OPT_SLAP_TEST,
  OPT_SLAP_CONCURRENCY,
  OPT_SLAP_NON_BLOCK,
  OPT_SLAP_TCP_NODELAY,
  OPT_FLUSH,
  OPT_HASH,
  OPT_BINARY,
  OPT_UDP,
  OPT_USERNAME,
  OPT_PASSWD,
  OPT_STAT_ARGS,
  OPT_FILE= 'f'
} memcached_options;

#endif /* CLIENT_OPTIONS */
