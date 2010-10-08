/*
 *  memslap
 *
 *  (c) Copyright 2009, Schooner Information Technology, Inc.
 *  All rights reserved.
 *  http://www.schoonerinfotech.com/
 *
 *  Use and distribution licensed under the BSD license.  See
 *  the COPYING file for full text.
 *
 *  Authors:
 *      Brian Aker
 *      Mingqiang Zhuang <mingqiangzhuang@hengtiansoft.com>
 *
 */
#include "config.h"

#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
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


#include "ms_sigsegv.h"
#include "ms_setting.h"
#include "ms_thread.h"

#define PROGRAM_NAME    "memslap"
#define PROGRAM_DESCRIPTION \
                        "Generates workload against memcached servers."

#ifdef __sun
  /* For some odd reason the option struct on solaris defines the argument
   * as char* and not const char*
   */
#define OPTIONSTRING char*
#else
#define OPTIONSTRING const char*
#endif

/* options */
static struct option long_options[]=
{
  { (OPTIONSTRING)"servers",        required_argument,            NULL,
    OPT_SERVERS            },
  { (OPTIONSTRING)"threads",        required_argument,            NULL,
    OPT_THREAD_NUMBER      },
  { (OPTIONSTRING)"concurrency",    required_argument,            NULL,
    OPT_CONCURRENCY        },
  { (OPTIONSTRING)"conn_sock",      required_argument,            NULL,
    OPT_SOCK_PER_CONN      },
  { (OPTIONSTRING)"execute_number", required_argument,            NULL,
    OPT_EXECUTE_NUMBER     },
  { (OPTIONSTRING)"time",           required_argument,            NULL,
    OPT_TIME               },
  { (OPTIONSTRING)"cfg_cmd",        required_argument,            NULL,
    OPT_CONFIG_CMD         },
  { (OPTIONSTRING)"win_size",       required_argument,            NULL,
    OPT_WINDOW_SIZE        },
  { (OPTIONSTRING)"fixed_size",     required_argument,            NULL,
    OPT_FIXED_LTH          },
  { (OPTIONSTRING)"verify",         required_argument,            NULL,
    OPT_VERIFY             },
  { (OPTIONSTRING)"division",       required_argument,            NULL,
    OPT_GETS_DIVISION      },
  { (OPTIONSTRING)"stat_freq",      required_argument,            NULL,
    OPT_STAT_FREQ          },
  { (OPTIONSTRING)"exp_verify",     required_argument,            NULL,
    OPT_EXPIRE             },
  { (OPTIONSTRING)"overwrite",      required_argument,            NULL,
    OPT_OVERWRITE          },
  { (OPTIONSTRING)"reconnect",      no_argument,                  NULL,
    OPT_RECONNECT          },
  { (OPTIONSTRING)"udp",            no_argument,                  NULL,
    OPT_UDP                },
  { (OPTIONSTRING)"facebook",       no_argument,                  NULL,
    OPT_FACEBOOK_TEST      },
  { (OPTIONSTRING)"binary",         no_argument,                  NULL,
    OPT_BINARY_PROTOCOL    },
  { (OPTIONSTRING)"tps",            required_argument,            NULL,
    OPT_TPS                },
  { (OPTIONSTRING)"rep_write",      required_argument,            NULL,
    OPT_REP_WRITE_SRV      },
  { (OPTIONSTRING)"verbose",        no_argument,                  NULL,
    OPT_VERBOSE            },
  { (OPTIONSTRING)"help",           no_argument,                  NULL,
    OPT_HELP               },
  { (OPTIONSTRING)"version",        no_argument,                  NULL,
    OPT_VERSION            },
  { 0, 0, 0, 0 },
};

/* Prototypes */
static void ms_sync_lock_init(void);
static void ms_sync_lock_destroy(void);
static void ms_global_struct_init(void);
static void ms_global_struct_destroy(void);
static void ms_version_command(const char *command_name);
static const char *ms_lookup_help(ms_options_t option);
static int64_t ms_parse_time(void);
static int64_t ms_parse_size(void);
static void ms_options_parse(int argc, char *argv[]);
static int ms_check_para(void);
static void ms_statistic_init(void);
static void ms_stats_init(void);
static void ms_print_statistics(int in_time);
static void ms_print_memslap_stats(struct timeval *start_time,
                                   struct timeval *end_time);
static void ms_monitor_slap_mode(void);
void ms_help_command(const char *command_name, const char *description);


/* initialize the global locks */
static void ms_sync_lock_init()
{
  ms_global.init_lock.count= 0;
  pthread_mutex_init(&ms_global.init_lock.lock, NULL);
  pthread_cond_init(&ms_global.init_lock.cond, NULL);

  ms_global.warmup_lock.count = 0;
  pthread_mutex_init(&ms_global.warmup_lock.lock, NULL);
  pthread_cond_init(&ms_global.warmup_lock.cond, NULL);

  ms_global.run_lock.count= 0;
  pthread_mutex_init(&ms_global.run_lock.lock, NULL);
  pthread_cond_init(&ms_global.run_lock.cond, NULL);

  pthread_mutex_init(&ms_global.quit_mutex, NULL);
  pthread_mutex_init(&ms_global.seq_mutex, NULL);
} /* ms_sync_lock_init */


/* destroy the global locks */
static void ms_sync_lock_destroy()
{
  pthread_mutex_destroy(&ms_global.init_lock.lock);
  pthread_cond_destroy(&ms_global.init_lock.cond);

  pthread_mutex_destroy(&ms_global.warmup_lock.lock);
  pthread_cond_destroy(&ms_global.warmup_lock.cond);

  pthread_mutex_destroy(&ms_global.run_lock.lock);
  pthread_cond_destroy(&ms_global.run_lock.cond);

  pthread_mutex_destroy(&ms_global.quit_mutex);
  pthread_mutex_destroy(&ms_global.seq_mutex);

  if (ms_setting.stat_freq > 0)
  {
    pthread_mutex_destroy(&ms_statistic.stat_mutex);
  }
} /* ms_sync_lock_destroy */


/* initialize the global structure */
static void ms_global_struct_init()
{
  ms_sync_lock_init();
  ms_global.finish_warmup= false;
  ms_global.time_out= false;
}


/* destroy the global structure */
static void ms_global_struct_destroy()
{
  ms_sync_lock_destroy();
}


/**
 * output the version information
 *
 * @param command_name, the string of this process
 */
static void ms_version_command(const char *command_name)
{
  printf("%s v%u.%u\n", command_name, 1U, 0U);
  exit(0);
}


/**
 * get the description of the option
 *
 * @param option, option of command line
 *
 * @return char*, description of the command option
 */
static const char *ms_lookup_help(ms_options_t option)
{
  switch (option)
  {
  case OPT_SERVERS:
    return
      "List one or more servers to connect. Servers count must be less than\n"
      "        threads count. e.g.: --servers=localhost:1234,localhost:11211";

  case OPT_VERSION:
    return "Display the version of the application and then exit.";

  case OPT_HELP:
    return "Display this message and then exit.";

  case OPT_EXECUTE_NUMBER:
    return "Number of operations(get and set) to execute for the\n"
           "        given test. Default 1000000.";

  case OPT_THREAD_NUMBER:
    return
      "Number of threads to startup, better equal to CPU numbers. Default 8.";

  case OPT_CONCURRENCY:
    return "Number of concurrency to simulate with load. Default 128.";

  case OPT_FIXED_LTH:
    return "Fixed length of value.";

  case OPT_VERIFY:
    return "The proportion of date verification, e.g.: --verify=0.01";

  case OPT_GETS_DIVISION:
    return "Number of keys to multi-get once. Default 1, means single get.";

  case OPT_TIME:
    return
      "How long the test to run, suffix: s-seconds, m-minutes, h-hours,\n"
      "        d-days e.g.: --time=2h.";

  case OPT_CONFIG_CMD:
    return
      "Load the configure file to get command,key and value distribution list.";

  case OPT_WINDOW_SIZE:
    return
      "Task window size of each concurrency, suffix: K, M e.g.: --win_size=10k.\n"
      "        Default 10k.";

  case OPT_UDP:
    return
      "UDP support, default memslap uses TCP, TCP port and UDP port of\n"
      "        server must be same.";

  case OPT_EXPIRE:
    return
      "The proportion of objects with expire time, e.g.: --exp_verify=0.01.\n"
      "        Default no object with expire time";

  case OPT_OVERWRITE:
    return
      "The proportion of objects need overwrite, e.g.: --overwrite=0.01.\n"
      "        Default never overwrite object.";

  case OPT_STAT_FREQ:
    return
      "Frequency of dumping statistic information. suffix: s-seconds,\n"
      "        m-minutes, e.g.: --resp_freq=10s.";

  case OPT_SOCK_PER_CONN:
    return "Number of TCP socks per concurrency. Default 1.";

  case OPT_RECONNECT:
    return
      "Reconnect support, when connection is closed it will be reconnected.";

  case OPT_VERBOSE:
    return
      "Whether it outputs detailed information when verification fails.";

  case OPT_FACEBOOK_TEST:
    return
      "Whether it enables facebook test feature, set with TCP and multi-get with UDP.";

  case OPT_BINARY_PROTOCOL:
    return
      "Whether it enables binary protocol. Default with ASCII protocol.";

  case OPT_TPS:
    return "Expected throughput, suffix: K, e.g.: --tps=10k.";

  case OPT_REP_WRITE_SRV:
    return "The first nth servers can write data, e.g.: --rep_write=2.";

  default:
    return "Forgot to document this option :)";
  } /* switch */
} /* ms_lookup_help */


/**
 * output the help information
 *
 * @param command_name, the string of this process
 * @param description, description of this process
 * @param long_options, global options array
 */
void ms_help_command(const char *command_name, const char *description)
{
  char *help_message= NULL;

  printf("%s v%u.%u\n", command_name, 1U, 0U);
  printf("    %s\n\n", description);
  printf(
    "Usage:\n"
    "    memslap -hV | -s servers [-F config_file] [-t time | -x exe_num] [...]\n\n"
    "Options:\n");

  for (int x= 0; long_options[x].name; x++)
  {
    printf("    -%c, --%s%c\n", long_options[x].val, long_options[x].name,
           long_options[x].has_arg ? '=' : ' ');

    if ((help_message= (char *)ms_lookup_help(long_options[x].val)) != NULL)
    {
      printf("        %s\n", help_message);
    }
  }

  printf(
    "\nExamples:\n"
    "    memslap -s 127.0.0.1:11211 -S 5s\n"
    "    memslap -s 127.0.0.1:11211 -t 2m -v 0.2 -e 0.05 -b\n"
    "    memslap -s 127.0.0.1:11211 -F config -t 2m -w 40k -S 20s -o 0.2\n"
    "    memslap -s 127.0.0.1:11211 -F config -t 2m -T 4 -c 128 -d 20 -P 40k\n"
    "    memslap -s 127.0.0.1:11211 -F config -t 2m -d 50 -a -n 40\n"
    "    memslap -s 127.0.0.1:11211,127.0.0.1:11212 -F config -t 2m\n"
    "    memslap -s 127.0.0.1:11211,127.0.0.1:11212 -F config -t 2m -p 2\n\n");

  exit(0);
} /* ms_help_command */


/* used to parse the time string  */
static int64_t ms_parse_time()
{
  int64_t ret= 0;
  char unit= optarg[strlen(optarg) - 1];

  optarg[strlen(optarg) - 1]= '\0';
  ret= atoi(optarg);

  switch (unit)
  {
  case 'd':
  case 'D':
    ret*= 24;

  case 'h':
  case 'H':
    ret*= 60;

  case 'm':
  case 'M':
    ret*= 60;

  case 's':
  case 'S':
    break;

  default:
    ret= -1;
    break;
  } /* switch */

  return ret;
} /* ms_parse_time */


/* used to parse the size string */
static int64_t ms_parse_size()
{
  int64_t ret= -1;
  char unit= optarg[strlen(optarg) - 1];

  optarg[strlen(optarg) - 1]= '\0';
  ret= strtoll(optarg, (char **)NULL, 10);

  switch (unit)
  {
  case 'k':
  case 'K':
    ret*= 1024;
    break;

  case 'm':
  case 'M':
    ret*= 1024 * 1024;
    break;

  case 'g':
  case 'G':
    ret*= 1024 * 1024 * 1024;
    break;

  default:
    ret= -1;
    break;
  } /* switch */

  return ret;
} /* ms_parse_size */


/* used to parse the options of command line */
static void ms_options_parse(int argc, char *argv[])
{
  int option_index= 0;
  int option_rv;

  while ((option_rv= getopt_long(argc, argv, "VhURbaBs:x:T:c:X:v:d:"
                                             "t:S:F:w:e:o:n:P:p:",
                                 long_options, &option_index)) != -1)
  {
    switch (option_rv)
    {
    case 0:
      break;

    case OPT_VERSION:     /* --version or -V */
      ms_version_command(PROGRAM_NAME);
      break;

    case OPT_HELP:     /* --help or -h */
      ms_help_command(PROGRAM_NAME, PROGRAM_DESCRIPTION);
      break;

    case OPT_SERVERS:     /* --servers or -s */
      ms_setting.srv_str= strdup(optarg);
      break;

    case OPT_CONCURRENCY:       /* --concurrency or -c */
      ms_setting.nconns= (uint32_t)strtoul(optarg, (char **) NULL, 10);
      if (ms_setting.nconns <= 0)
      {
        fprintf(stderr, "Concurrency must be greater than 0.:-)\n");
        exit(1);
      }
      break;

    case OPT_EXECUTE_NUMBER:        /* --execute_number or -x */
      ms_setting.exec_num= (int)strtol(optarg, (char **) NULL, 10);
      if (ms_setting.exec_num <= 0)
      {
        fprintf(stderr, "Execute number must be greater than 0.:-)\n");
        exit(1);
      }
      break;

    case OPT_THREAD_NUMBER:     /* --threads or -T */
      ms_setting.nthreads= (uint32_t)strtoul(optarg, (char **) NULL, 10);
      if (ms_setting.nthreads <= 0)
      {
        fprintf(stderr, "Threads number must be greater than 0.:-)\n");
        exit(1);
      }
      break;

    case OPT_FIXED_LTH:         /* --fixed_size or -X */
      ms_setting.fixed_value_size= (size_t)strtoull(optarg, (char **) NULL, 10);
      if ((ms_setting.fixed_value_size <= 0)
          || (ms_setting.fixed_value_size > MAX_VALUE_SIZE))
      {
        fprintf(stderr, "Value size must be between 0 and 1M.:-)\n");
        exit(1);
      }
      break;

    case OPT_VERIFY:        /* --verify or -v */
      ms_setting.verify_percent= atof(optarg);
      if ((ms_setting.verify_percent <= 0)
          || (ms_setting.verify_percent > 1.0))
      {
        fprintf(stderr, "Data verification rate must be "
                        "greater than 0 and less than 1.0. :-)\n");
        exit(1);
      }
      break;

    case OPT_GETS_DIVISION:         /* --division or -d */
      ms_setting.mult_key_num= (int)strtol(optarg, (char **) NULL, 10);
      if (ms_setting.mult_key_num <= 0)
      {
        fprintf(stderr, "Multi-get key number must be greater than 0.:-)\n");
        exit(1);
      }
      break;

    case OPT_TIME:      /* --time or -t */
      ms_setting.run_time= (int)ms_parse_time();
      if (ms_setting.run_time == -1)
      {
        fprintf(stderr, "Please specify the run time. :-)\n"
                        "'s' for second, 'm' for minute, 'h' for hour, "
                        "'d' for day. e.g.: --time=24h (means 24 hours).\n");
        exit(1);
      }

      if (ms_setting.run_time == 0)
      {
        fprintf(stderr, "Running time can not be 0. :-)\n");
        exit(1);
      }
      break;

    case OPT_CONFIG_CMD:        /* --cfg_cmd or -F */
      ms_setting.cfg_file= strdup(optarg);
      break;

    case OPT_WINDOW_SIZE:       /* --win_size or -w */
      ms_setting.win_size= (size_t)ms_parse_size();
      if (ms_setting.win_size == (size_t)-1)
      {
        fprintf(
          stderr,
          "Please specify the item window size. :-)\n"
          "e.g.: --win_size=10k (means 10k task window size).\n");
        exit(1);
      }
      break;

    case OPT_UDP:       /* --udp or -U*/
      ms_setting.udp= true;
      break;

    case OPT_EXPIRE:        /* --exp_verify or -e */
      ms_setting.exp_ver_per= atof(optarg);
      if ((ms_setting.exp_ver_per <= 0) || (ms_setting.exp_ver_per > 1.0))
      {
        fprintf(stderr, "Expire time verification rate must be "
                        "greater than 0 and less than 1.0. :-)\n");
        exit(1);
      }
      break;

    case OPT_OVERWRITE:         /* --overwrite or -o */
      ms_setting.overwrite_percent= atof(optarg);
      if ((ms_setting.overwrite_percent <= 0)
          || (ms_setting.overwrite_percent > 1.0))
      {
        fprintf(stderr, "Objects overwrite rate must be "
                        "greater than 0 and less than 1.0. :-)\n");
        exit(1);
      }
      break;

    case OPT_STAT_FREQ:         /* --stat_freq or -S */
      ms_setting.stat_freq= (int)ms_parse_time();
      if (ms_setting.stat_freq == -1)
      {
        fprintf(stderr, "Please specify the frequency of dumping "
                        "statistic information. :-)\n"
                        "'s' for second, 'm' for minute, 'h' for hour, "
                        "'d' for day. e.g.: --time=24h (means 24 hours).\n");
        exit(1);
      }

      if (ms_setting.stat_freq == 0)
      {
        fprintf(stderr, "The frequency of dumping statistic information "
                        "can not be 0. :-)\n");
        exit(1);
      }
      break;

    case OPT_SOCK_PER_CONN:         /* --conn_sock or -n */
      ms_setting.sock_per_conn= (uint32_t)strtoul(optarg, (char **) NULL, 10);
      if (ms_setting.sock_per_conn <= 0)
      {
        fprintf(stderr, "Number of socks of each concurrency "
                        "must be greater than 0.:-)\n");
        exit(1);
      }
      break;

    case OPT_RECONNECT:     /* --reconnect or -R */
      ms_setting.reconnect= true;
      break;

    case OPT_VERBOSE:       /* --verbose or -b */
      ms_setting.verbose= true;
      break;

    case OPT_FACEBOOK_TEST:         /* --facebook or -a */
      ms_setting.facebook_test= true;
      break;

    case OPT_BINARY_PROTOCOL:       /* --binary or -B */
      ms_setting.binary_prot= true;
      break;

    case OPT_TPS:       /* --tps or -P */
      ms_setting.expected_tps= (int)ms_parse_size();
      if (ms_setting.expected_tps == -1)
      {
        fprintf(stderr,
                "Please specify the item expected throughput. :-)\n"
                "e.g.: --tps=10k (means 10k throughput).\n");
        exit(1);
      }
      break;

    case OPT_REP_WRITE_SRV:         /* --rep_write or -p */
      ms_setting.rep_write_srv= (uint32_t)strtoul(optarg, (char **) NULL, 10);
      if (ms_setting.rep_write_srv <= 0)
      {
        fprintf(stderr,
                "Number of replication writing server must be greater "
                "than 0.:-)\n");
        exit(1);
      }
      break;

    case '?':
      /* getopt_long already printed an error message. */
      exit(1);

    default:
      abort();
    } /* switch */
  }
} /* ms_options_parse */


static int ms_check_para()
{
  if (ms_setting.srv_str == NULL)
  {
    char *temp;

    if ((temp= getenv("MEMCACHED_SERVERS")))
    {
      ms_setting.srv_str= strdup(temp);
    }
    else
    {
      fprintf(stderr, "No Servers provided\n\n");
      return -1;
    }
  }

  if (ms_setting.nconns % (uint32_t)ms_setting.nthreads != 0)
  {
    fprintf(stderr, "Concurrency must be the multiples of threads count.\n");
    return -1;
  }

  if (ms_setting.win_size % UNIT_ITEMS_COUNT != 0)
  {
    fprintf(stderr, "Window size must be the multiples of 1024.\n\n");
    return -1;
  }

  return 0;
} /* ms_check_para */


/* initialize the statistic structure */
static void ms_statistic_init()
{
  pthread_mutex_init(&ms_statistic.stat_mutex, NULL);
  ms_init_stats(&ms_statistic.get_stat, "Get");
  ms_init_stats(&ms_statistic.set_stat, "Set");
  ms_init_stats(&ms_statistic.total_stat, "Total");
} /* ms_statistic_init */


/* initialize the global state structure */
static void ms_stats_init()
{
  memset(&ms_stats, 0, sizeof(ms_stats_t));
  if (ms_setting.stat_freq > 0)
  {
    ms_statistic_init();
  }
} /* ms_stats_init */


/* use to output the statistic */
static void ms_print_statistics(int in_time)
{
  int obj_size= (int)(ms_setting.avg_key_size + ms_setting.avg_val_size);

  printf("\033[1;1H\033[2J\n");
  ms_dump_format_stats(&ms_statistic.get_stat, in_time,
                       ms_setting.stat_freq, obj_size);
  ms_dump_format_stats(&ms_statistic.set_stat, in_time,
                       ms_setting.stat_freq, obj_size);
  ms_dump_format_stats(&ms_statistic.total_stat, in_time,
                       ms_setting.stat_freq, obj_size);
} /* ms_print_statistics */


/* used to print the states of memslap */
static void ms_print_memslap_stats(struct timeval *start_time,
                                   struct timeval *end_time)
{
  char buf[1024];
  char *pos= buf;

  pos+= sprintf(pos, "cmd_get: %zu\n",
                ms_stats.cmd_get);
  pos+= sprintf(pos, "cmd_set: %zu\n",
                ms_stats.cmd_set);
  pos+= sprintf(pos, "get_misses: %zu\n",
                ms_stats.get_misses);

  if (ms_setting.verify_percent > 0)
  {
    pos+= sprintf(pos, "verify_misses: %zu\n",
                  ms_stats.vef_miss);
    pos+= sprintf(pos, "verify_failed: %zu\n",
                  ms_stats.vef_failed);
  }

  if (ms_setting.exp_ver_per > 0)
  {
    pos+= sprintf(pos, "expired_get: %zu\n",
                  ms_stats.exp_get);
    pos+= sprintf(pos, "unexpired_unget: %zu\n",
                  ms_stats.unexp_unget);
  }

  pos+= sprintf(pos,
                "written_bytes: %zu\n",
                ms_stats.bytes_written);
  pos+= sprintf(pos, "read_bytes: %zu\n",
                ms_stats.bytes_read);
  pos+= sprintf(pos, "object_bytes: %zu\n",
                ms_stats.obj_bytes);

  if (ms_setting.udp || ms_setting.facebook_test)
  {
    pos+= sprintf(pos, "packet_disorder: %zu\n",
                  ms_stats.pkt_disorder);
    pos+= sprintf(pos, "packet_drop: %zu\n",
                  ms_stats.pkt_drop);
    pos+= sprintf(pos, "udp_timeout: %zu\n",
                  ms_stats.udp_timeout);
  }

  if (ms_setting.stat_freq > 0)
  {
    ms_dump_stats(&ms_statistic.get_stat);
    ms_dump_stats(&ms_statistic.set_stat);
    ms_dump_stats(&ms_statistic.total_stat);
  }

  int64_t time_diff= ms_time_diff(start_time, end_time);
  pos+= sprintf(
    pos,
    "\nRun time: %.1fs Ops: %llu TPS: %.0Lf Net_rate: %.1fM/s\n",
    (double)time_diff / 1000000,
    (unsigned long long)(ms_stats.cmd_get + ms_stats.cmd_set),
    (ms_stats.cmd_get
                 + ms_stats.cmd_set) / ((long double)time_diff / 1000000),
    (double)(
      ms_stats.bytes_written
      + ms_stats.bytes_read) / 1024 / 1024
    / ((double)time_diff / 1000000));

  fprintf(stdout, "%s", buf);
  fflush(stdout);
} /* ms_print_memslap_stats */


/* the loop of the main thread, wait the work threads to complete */
static void ms_monitor_slap_mode()
{
  int second= 0;
  struct timeval start_time, end_time;

  /* Wait all the threads complete initialization. */
  pthread_mutex_lock(&ms_global.init_lock.lock);
  while (ms_global.init_lock.count < ms_setting.nthreads)
  {
    pthread_cond_wait(&ms_global.init_lock.cond,
                      &ms_global.init_lock.lock);
  }
  pthread_mutex_unlock(&ms_global.init_lock.lock);

  /* only when there is no set operation it need warm up */
  if (ms_setting.cmd_distr[CMD_SET].cmd_prop < PROP_ERROR)
  {
    /* Wait all the connects complete warm up. */
    pthread_mutex_lock(&ms_global.warmup_lock.lock);
    while (ms_global.warmup_lock.count < ms_setting.nconns)
    {
      pthread_cond_wait(&ms_global.warmup_lock.cond, &ms_global.warmup_lock.lock);
    }
    pthread_mutex_unlock(&ms_global.warmup_lock.lock);
  }
  ms_global.finish_warmup= true;

  /* running in "run time" mode, user specify run time */
  if (ms_setting.run_time > 0)
  {
    gettimeofday(&start_time, NULL);
    while (1)
    {
      sleep(1);
      second++;

      if ((ms_setting.stat_freq > 0) && (second % ms_setting.stat_freq == 0)
          && (ms_stats.active_conns >= ms_setting.nconns)
          && (ms_stats.active_conns <= INT_MAX))
      {
        ms_print_statistics(second);
      }

      if (ms_setting.run_time <= second)
      {
        ms_global.time_out= true;
        break;
      }

      /* all connections disconnect */
      if ((second > 5) && (ms_stats.active_conns == 0))
      {
        break;
      }
    }
    gettimeofday(&end_time, NULL);
    sleep(1);       /* wait all threads clean up */
  }
  else
  {
    /* running in "execute number" mode, user specify execute number */
    gettimeofday(&start_time, NULL);

    /*
     * We loop until we know that all connects have cleaned up.
     */
    pthread_mutex_lock(&ms_global.run_lock.lock);
    while (ms_global.run_lock.count < ms_setting.nconns)
    {
      pthread_cond_wait(&ms_global.run_lock.cond, &ms_global.run_lock.lock);
    }
    pthread_mutex_unlock(&ms_global.run_lock.lock);

    gettimeofday(&end_time, NULL);
  }

  ms_print_memslap_stats(&start_time, &end_time);
} /* ms_monitor_slap_mode */


/* the main function */
int main(int argc, char *argv[])
{
  srandom((unsigned int)time(NULL));
  ms_global_struct_init();

  /* initialization */
  ms_setting_init_pre();
  ms_options_parse(argc, argv);
  if (ms_check_para())
  {
    ms_help_command(PROGRAM_NAME, PROGRAM_DESCRIPTION);
    exit(1);
  }
  ms_setting_init_post();
  ms_stats_init();
  ms_thread_init();

  /* waiting work thread complete its task */
  ms_monitor_slap_mode();

  /* clean up */
  ms_thread_cleanup();
  ms_global_struct_destroy();
  ms_setting_cleanup();

  return 0;
} /* main */
