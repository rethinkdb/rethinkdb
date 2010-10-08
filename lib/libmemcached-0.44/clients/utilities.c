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
#include <ctype.h>
#include <string.h>
#include "utilities.h"


long int timedif(struct timeval a, struct timeval b)
{
  long us, s;

  us = (int)(a.tv_usec - b.tv_usec);
  us /= 1000;
  s = (int)(a.tv_sec - b.tv_sec);
  s *= 1000;
  return s + us;
}

void version_command(const char *command_name)
{
  printf("%s v%u.%u\n", command_name, 1U, 0U);
  exit(0);
}

static const char *lookup_help(memcached_options option)
{
  switch (option)
  {
  case OPT_SERVERS: return("List which servers you wish to connect to.");
  case OPT_VERSION: return("Display the version of the application and then exit.");
  case OPT_HELP: return("Display this message and then exit.");
  case OPT_VERBOSE: return("Give more details on the progression of the application.");
  case OPT_DEBUG: return("Provide output only useful for debugging.");
  case OPT_FLAG: return("Provide flag information for storage operation.");
  case OPT_EXPIRE: return("Set the expire option for the object.");
  case OPT_SET: return("Use set command with memcached when storing.");
  case OPT_REPLACE: return("Use replace command with memcached when storing.");
  case OPT_ADD: return("Use add command with memcached when storing.");
  case OPT_SLAP_EXECUTE_NUMBER: return("Number of times to execute the given test.");
  case OPT_SLAP_INITIAL_LOAD: return("Number of key pairs to load before executing tests.");
  case OPT_SLAP_TEST: return("Test to run (currently \"get\" or \"set\").");
  case OPT_SLAP_CONCURRENCY: return("Number of users to simulate with load.");
  case OPT_SLAP_NON_BLOCK: return("Set TCP up to use non-blocking IO.");
  case OPT_SLAP_TCP_NODELAY: return("Set TCP socket up to use nodelay.");
  case OPT_FLUSH: return("Flush servers before running tests.");
  case OPT_HASH: return("Select hash type.");
  case OPT_BINARY: return("Switch to binary protocol.");
  case OPT_ANALYZE: return("Analyze the provided servers.");
  case OPT_UDP: return("Use UDP protocol when communicating with server.");
  case OPT_USERNAME: return "Username to use for SASL authentication";
  case OPT_PASSWD: return "Password to use for SASL authentication";
  case OPT_FILE: return "Path to file in which to save result";
  case OPT_STAT_ARGS: return "Argument for statistics";
  default: WATCHPOINT_ASSERT(0);
  };

  WATCHPOINT_ASSERT(0);
  return "forgot to document this function :)";
}

void help_command(const char *command_name, const char *description,
                  const struct option *long_options,
                  memcached_programs_help_st *options __attribute__((unused)))
{
  unsigned int x;

  printf("%s v%u.%u\n\n", command_name, 1U, 0U);
  printf("\t%s\n\n", description);
  printf("Current options. A '=' means the option takes a value.\n\n");

  for (x= 0; long_options[x].name; x++)
  {
    const char *help_message;

    printf("\t --%s%c\n", long_options[x].name,
           long_options[x].has_arg ? '=' : ' ');
    if ((help_message= lookup_help(long_options[x].val)))
      printf("\t\t%s\n", help_message);
  }

  printf("\n");
  exit(0);
}

void process_hash_option(memcached_st *memc, char *opt_hash)
{
  uint64_t set;
  memcached_return_t rc;

  if (opt_hash == NULL)
    return;

  set= MEMCACHED_HASH_DEFAULT; /* Just here to solve warning */
  if (!strcasecmp(opt_hash, "CRC"))
    set= MEMCACHED_HASH_CRC;
  else if (!strcasecmp(opt_hash, "FNV1_64"))
    set= MEMCACHED_HASH_FNV1_64;
  else if (!strcasecmp(opt_hash, "FNV1A_64"))
    set= MEMCACHED_HASH_FNV1A_64;
  else if (!strcasecmp(opt_hash, "FNV1_32"))
    set= MEMCACHED_HASH_FNV1_32;
  else if (!strcasecmp(opt_hash, "FNV1A_32"))
    set= MEMCACHED_HASH_FNV1A_32;
  else
  {
    fprintf(stderr, "hash: type not recognized %s\n", opt_hash);
    exit(1);
  }

  rc= memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_HASH, set);
  if (rc != MEMCACHED_SUCCESS)
  {
    fprintf(stderr, "hash: memcache error %s\n", memcached_strerror(memc, rc));
    exit(1);
  }
}

#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
static char *username;
static char *passwd;

static int get_username(void *context, int id, const char **result,
                        unsigned int *len)
{
  (void)context;
  if (!result || (id != SASL_CB_USER && id != SASL_CB_AUTHNAME))
    return SASL_BADPARAM;

  *result= username;
  if (len)
     *len= (username == NULL) ? 0 : (unsigned int)strlen(username);

  return SASL_OK;
}

static int get_password(sasl_conn_t *conn, void *context, int id,
                        sasl_secret_t **psecret)
{
  (void)context;
  static sasl_secret_t* x;

  if (!conn || ! psecret || id != SASL_CB_PASS)
    return SASL_BADPARAM;

  if (passwd == NULL)
  {
     *psecret= NULL;
     return SASL_OK;
  }

  size_t len= strlen(passwd);
  x = realloc(x, sizeof(sasl_secret_t) + len);
  if (!x)
    return SASL_NOMEM;

  x->len = len;
  strcpy((void *)x->data, passwd);

  *psecret = x;
  return SASL_OK;
}

/* callbacks we support */
static sasl_callback_t sasl_callbacks[] = {
  {
    SASL_CB_USER, &get_username, NULL
  }, {
    SASL_CB_AUTHNAME, &get_username, NULL
  }, {
    SASL_CB_PASS, &get_password, NULL
  }, {
    SASL_CB_LIST_END, NULL, NULL
  }
};
#endif

bool initialize_sasl(memcached_st *memc, char *user, char *password)
{
#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
  if (user != NULL && password != NULL)
  {
    username= user;
    passwd= password;

    if (sasl_client_init(NULL) != SASL_OK)
    {
      fprintf(stderr, "Failed to initialize sasl library!\n");
      return false;
    }
    memcached_set_sasl_callbacks(memc, sasl_callbacks);
  }
#else
  (void)memc;
  (void)user;
  (void)password;
#endif

  return true;
}

void shutdown_sasl(void)
{
#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
  if (username != NULL || passwd != NULL)
    sasl_done();
#endif
}

void initialize_sockets(void)
{
  /* Define the function for all platforms to avoid #ifdefs in each program */
#ifdef WIN32
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0)
  {
    fprintf(stderr, "Socket Initialization Error. Program aborted\n");
    exit(EXIT_FAILURE);
  }
#endif
}
