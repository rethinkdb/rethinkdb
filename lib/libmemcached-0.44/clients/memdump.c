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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <string.h>

#include <libmemcached/memcached.h>

#include "client_options.h"
#include "utilities.h"

#define PROGRAM_NAME "memdump"
#define PROGRAM_DESCRIPTION "Dump all values from one or many servers."

/* Prototypes */
static void options_parse(int argc, char *argv[]);

static int opt_binary=0;
static int opt_verbose= 0;
static char *opt_servers= NULL;
static char *opt_hash= NULL;
static char *opt_username;
static char *opt_passwd;

/* Print the keys and counter how many were found */
static memcached_return_t key_printer(const memcached_st *ptr __attribute__((unused)),
                                      const char *key, size_t key_length,
                                      void *context __attribute__((unused)))
{
  printf("%.*s\n", (uint32_t)key_length, key);

  return MEMCACHED_SUCCESS;
}

int main(int argc, char *argv[])
{
  memcached_st *memc;
  memcached_return_t rc;
  memcached_server_st *servers;
  memcached_dump_fn callbacks[1];

  callbacks[0]= &key_printer;

  options_parse(argc, argv);

  memc= memcached_create(NULL);
  process_hash_option(memc, opt_hash);

  if (!opt_servers)
  {
    char *temp;

    if ((temp= getenv("MEMCACHED_SERVERS")))
      opt_servers= strdup(temp);
    else
    {
      fprintf(stderr, "No Servers provided\n");
      exit(1);
    }
  }

  if (opt_servers)
    servers= memcached_servers_parse(opt_servers);
  else
    servers= memcached_servers_parse(argv[--argc]);

  memcached_server_push(memc, servers);
  memcached_server_list_free(servers);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL,
                         (uint64_t)opt_binary);
  if (!initialize_sasl(memc, opt_username, opt_passwd))
  {
    memcached_free(memc);
    return 1;
  }

  rc= memcached_dump(memc, callbacks, NULL, 1);

  if (rc != MEMCACHED_SUCCESS)
  {
    fprintf(stderr, "memdump: memcache error %s", memcached_strerror(memc, rc));
    if (memc->cached_errno)
      fprintf(stderr, " system error %s", strerror(memc->cached_errno));
    fprintf(stderr, "\n");
  }

  memcached_free(memc);

  if (opt_servers)
    free(opt_servers);
  if (opt_hash)
    free(opt_hash);

  shutdown_sasl();

  return 0;
}

static void options_parse(int argc, char *argv[])
{
  int option_index= 0;
  int option_rv;

  static struct option long_options[]=
    {
      {(OPTIONSTRING)"version", no_argument, NULL, OPT_VERSION},
      {(OPTIONSTRING)"help", no_argument, NULL, OPT_HELP},
      {(OPTIONSTRING)"verbose", no_argument, &opt_verbose, OPT_VERBOSE},
      {(OPTIONSTRING)"debug", no_argument, &opt_verbose, OPT_DEBUG},
      {(OPTIONSTRING)"servers", required_argument, NULL, OPT_SERVERS},
      {(OPTIONSTRING)"hash", required_argument, NULL, OPT_HASH},
      {(OPTIONSTRING)"binary", no_argument, NULL, OPT_BINARY},
      {(OPTIONSTRING)"username", required_argument, NULL, OPT_USERNAME},
      {(OPTIONSTRING)"password", required_argument, NULL, OPT_PASSWD},
      {0, 0, 0, 0}
    };

  while (1)
  {
    option_rv= getopt_long(argc, argv, "Vhvds:", long_options, &option_index);

    if (option_rv == -1) break;

    switch (option_rv)
    {
    case 0:
      break;
    case OPT_BINARY:
      opt_binary = 1;
      break;
    case OPT_VERBOSE: /* --verbose or -v */
      opt_verbose = OPT_VERBOSE;
      break;
    case OPT_DEBUG: /* --debug or -d */
      opt_verbose = OPT_DEBUG;
      break;
    case OPT_VERSION: /* --version or -V */
      version_command(PROGRAM_NAME);
      break;
    case OPT_HELP: /* --help or -h */
      help_command(PROGRAM_NAME, PROGRAM_DESCRIPTION, long_options, NULL);
      break;
    case OPT_SERVERS: /* --servers or -s */
      opt_servers= strdup(optarg);
      break;
    case OPT_HASH:
      opt_hash= strdup(optarg);
      break;
    case OPT_USERNAME:
       opt_username= optarg;
       break;
    case OPT_PASSWD:
       opt_passwd= optarg;
       break;
    case '?':
      /* getopt_long already printed an error message. */
      exit(1);
    default:
      abort();
    }
  }
}
