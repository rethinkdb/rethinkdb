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

#include <getopt.h>
#include <libmemcached/memcached.h>
#include "libmemcached/watchpoint.h"
#include "client_options.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef __sun
  /* For some odd reason the option struct on solaris defines the argument
   * as char* and not const char*
   */
#define OPTIONSTRING char*
#else
#define OPTIONSTRING const char*
#endif

typedef struct memcached_programs_help_st memcached_programs_help_st;

struct memcached_programs_help_st
{
  char *not_used_yet;
};

char *strdup_cleanup(const char *str);
void cleanup(void);
long int timedif(struct timeval a, struct timeval b);
void version_command(const char *command_name);
void help_command(const char *command_name, const char *description,
                  const struct option *long_options,
                  memcached_programs_help_st *options);
void process_hash_option(memcached_st *memc, char *opt_hash);
bool initialize_sasl(memcached_st *memc, char *user, char *password);
void shutdown_sasl(void);
void initialize_sockets(void);
