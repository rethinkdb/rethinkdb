/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 * Authors: 
 *          Brian Aker
 *          Toru Maesaka
 */
#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include <libmemcached/memcached.h>

#include "client_options.h"
#include "utilities.h"

#define PROGRAM_NAME "memstat"
#define PROGRAM_DESCRIPTION "Output the state of a memcached cluster."

/* Prototypes */
static void options_parse(int argc, char *argv[]);
static void run_analyzer(memcached_st *memc, memcached_stat_st *memc_stat);
static void print_analysis_report(memcached_st *memc,
                                  memcached_analysis_st *report);

static int opt_verbose= 0;
static int opt_displayflag= 0;
static int opt_analyze= 0;
static char *opt_servers= NULL;
static char *stat_args= NULL;
static char *analyze_mode= NULL;

static struct option long_options[]=
{
  {(OPTIONSTRING)"args", required_argument, NULL, OPT_STAT_ARGS},
  {(OPTIONSTRING)"version", no_argument, NULL, OPT_VERSION},
  {(OPTIONSTRING)"help", no_argument, NULL, OPT_HELP},
  {(OPTIONSTRING)"verbose", no_argument, &opt_verbose, OPT_VERBOSE},
  {(OPTIONSTRING)"debug", no_argument, &opt_verbose, OPT_DEBUG},
  {(OPTIONSTRING)"servers", required_argument, NULL, OPT_SERVERS},
  {(OPTIONSTRING)"flag", no_argument, &opt_displayflag, OPT_FLAG},
  {(OPTIONSTRING)"analyze", optional_argument, NULL, OPT_ANALYZE},
  {0, 0, 0, 0},
};


static memcached_return_t stat_printer(memcached_server_instance_st instance,
                                       const char *key, size_t key_length,
                                       const char *value, size_t value_length,
                                       void *context)
{
  static memcached_server_instance_st last= NULL;
  (void)context;

  if (last != instance)
  {
    printf("Server: %s (%u)\n", memcached_server_name(instance),
           (uint32_t)memcached_server_port(instance));
    last= instance;
  }

  printf("\t %.*s: %.*s\n", (int)key_length, key, (int)value_length, value);

  return MEMCACHED_SUCCESS;
}

int main(int argc, char *argv[])
{
  memcached_return_t rc;
  memcached_st *memc;
  memcached_server_st *servers;

  options_parse(argc, argv);
  initialize_sockets();

  if (! opt_servers)
  {
    char *temp;

    if ((temp= getenv("MEMCACHED_SERVERS")))
      opt_servers= strdup(temp);
    else
    {
      fprintf(stderr, "No Servers provided\n\n");
      help_command(PROGRAM_NAME, PROGRAM_DESCRIPTION, long_options, 0);
      exit(1);
    }
  }

  memc= memcached_create(NULL);

  servers= memcached_servers_parse(opt_servers);
  rc= memcached_server_push(memc, servers);
  memcached_server_list_free(servers);

  if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_SOME_ERRORS)
  {
    printf("Failure to communicate with servers (%s)\n",
           memcached_strerror(memc, rc));
    exit(1);
  }

  if (opt_analyze)
  {
    memcached_stat_st *memc_stat;

    memc_stat= memcached_stat(memc, NULL, &rc);

    if (! memc_stat)
      exit(-1);

    run_analyzer(memc, memc_stat);

    memcached_stat_free(memc, memc_stat);
  }
  else
  {
    rc= memcached_stat_execute(memc, stat_args, stat_printer, NULL);
  }

  free(opt_servers);

  memcached_free(memc);

  return rc == MEMCACHED_SUCCESS ? 0: -1;
}

static void run_analyzer(memcached_st *memc, memcached_stat_st *memc_stat)
{
  memcached_return_t rc;

  if (analyze_mode == NULL)
  {
    memcached_analysis_st *report;
    report= memcached_analyze(memc, memc_stat, &rc);
    if (rc != MEMCACHED_SUCCESS || report == NULL)
    {
      printf("Failure to analyze servers (%s)\n",
             memcached_strerror(memc, rc));
      exit(1);
    }
    print_analysis_report(memc, report);
    free(report);
  }
  else if (strcmp(analyze_mode, "latency") == 0)
  {
    memcached_st **servers;
    uint32_t flags, server_count= memcached_server_count(memc);
    uint32_t num_of_tests= 32;
    const char *test_key= "libmemcached_test_key";

    servers= malloc(sizeof(memcached_st*) * server_count);
    if (!servers)
    {
      fprintf(stderr, "Failed to allocate memory\n");
      return;
    }

    for (uint32_t x= 0; x < server_count; x++)
    {
      memcached_server_instance_st instance=
        memcached_server_instance_by_position(memc, x);

      if ((servers[x]= memcached_create(NULL)) == NULL)
      {
        fprintf(stderr, "Failed to memcached_create()\n");
        if (x > 0)
          memcached_free(servers[0]);
        x--;
        for (; x > 0; x--)
          memcached_free(servers[x]);

        free(servers);
        return;
      }
      memcached_server_add(servers[x],
                           memcached_server_name(instance),
                           memcached_server_port(instance));
    }

    printf("Network Latency Test:\n\n");
    struct timeval start_time, end_time;
    uint32_t slowest_server= 0;
    long elapsed_time, slowest_time= 0;

    for (uint32_t x= 0; x < server_count; x++)
    {
      memcached_server_instance_st instance=
        memcached_server_instance_by_position(memc, x);
      gettimeofday(&start_time, NULL);

      for (uint32_t y= 0; y < num_of_tests; y++)
      {
        size_t vlen;
        char *val= memcached_get(servers[x], test_key, strlen(test_key),
                                 &vlen, &flags, &rc);
        if (rc != MEMCACHED_NOTFOUND && rc != MEMCACHED_SUCCESS)
          break;
        free(val);
      }
      gettimeofday(&end_time, NULL);

      elapsed_time= (long) timedif(end_time, start_time);
      elapsed_time /= (long) num_of_tests;

      if (elapsed_time > slowest_time)
      {
        slowest_server= x;
        slowest_time= elapsed_time;
      }

      if (rc != MEMCACHED_NOTFOUND && rc != MEMCACHED_SUCCESS)
      {
        printf("\t %s (%d)  =>  failed to reach the server\n",
               memcached_server_name(instance),
               memcached_server_port(instance));
      }
      else
      {
        printf("\t %s (%d)  =>  %ld.%ld seconds\n",
               memcached_server_name(instance),
               memcached_server_port(instance),
               elapsed_time / 1000, elapsed_time % 1000);
      }
    }

    if (server_count > 1 && slowest_time > 0)
    {
      memcached_server_instance_st slowest=
        memcached_server_instance_by_position(memc, slowest_server);

      printf("---\n");
      printf("Slowest Server: %s (%d) => %ld.%ld seconds\n",
             memcached_server_name(slowest),
             memcached_server_port(slowest),
             slowest_time / 1000, slowest_time % 1000);
    }
    printf("\n");

    for (uint32_t x= 0; x < server_count; x++)
      memcached_free(servers[x]);

    free(servers);
    free(analyze_mode);
  }
  else
  {
    fprintf(stderr, "Invalid Analyzer Option provided\n");
    free(analyze_mode);
  }
}

static void print_analysis_report(memcached_st *memc,
                                  memcached_analysis_st *report)
                                  
{
  uint32_t server_count= memcached_server_count(memc);
  memcached_server_instance_st most_consumed_server= memcached_server_instance_by_position(memc, report->most_consumed_server);
  memcached_server_instance_st least_free_server= memcached_server_instance_by_position(memc, report->least_free_server);
  memcached_server_instance_st oldest_server= memcached_server_instance_by_position(memc, report->oldest_server);

  printf("Memcached Cluster Analysis Report\n\n");

  printf("\tNumber of Servers Analyzed         : %u\n", server_count);
  printf("\tAverage Item Size (incl/overhead)  : %u bytes\n",
         report->average_item_size);

  if (server_count == 1)
  {
    printf("\nFor a detailed report, you must supply multiple servers.\n");
    return;
  }

  printf("\n");
  printf("\tNode with most memory consumption  : %s:%u (%llu bytes)\n",
         memcached_server_name(most_consumed_server),
         (uint32_t)memcached_server_port(most_consumed_server),
         (unsigned long long)report->most_used_bytes);
  printf("\tNode with least free space         : %s:%u (%llu bytes remaining)\n",
         memcached_server_name(least_free_server),
         (uint32_t)memcached_server_port(least_free_server),
         (unsigned long long)report->least_remaining_bytes);
  printf("\tNode with longest uptime           : %s:%u (%us)\n",
         memcached_server_name(oldest_server),
         (uint32_t)memcached_server_port(oldest_server),
         report->longest_uptime);
  printf("\tPool-wide Hit Ratio                : %1.f%%\n", report->pool_hit_ratio);
  printf("\n");
}

static void options_parse(int argc, char *argv[])
{
  memcached_programs_help_st help_options[]=
  {
    {0},
  };

  int option_index= 0;
  int option_rv;

  while (1) 
  {
    option_rv= getopt_long(argc, argv, "Vhvds:a", long_options, &option_index);
    if (option_rv == -1) break;
    switch (option_rv)
    {
    case 0:
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
      help_command(PROGRAM_NAME, PROGRAM_DESCRIPTION, long_options, help_options);
      break;
    case OPT_SERVERS: /* --servers or -s */
      opt_servers= strdup(optarg);
      break;
    case OPT_STAT_ARGS:
      stat_args= strdup(optarg);
      break;
    case OPT_ANALYZE: /* --analyze or -a */
      opt_analyze= OPT_ANALYZE;
      analyze_mode= (optarg) ? strdup(optarg) : NULL;
      break;
    case '?':
      /* getopt_long already printed an error message. */
      exit(1);
    default:
      abort();
    }
  }
}
