/*
 * File:   ms_setting.c
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */

#include "config.h"

#include <libmemcached/memcached.h>

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <pwd.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>



#include "ms_setting.h"
#include "ms_conn.h"

#define MAX_EXEC_NUM               0x4000000000000000      /* 1 << 62 */
#define ADDR_ALIGN(addr)    ((addr + 15) & ~(16 - 1))      /* 16 bytes aligned */
#define RAND_CHAR_SIZE             (10 * 1024 * 1024)      /* 10M character table */
#define RESERVED_RAND_CHAR_SIZE    (2 * 1024 * 1024)       /* reserved 2M to avoid pointer sloping over */

#define DEFAULT_CONFIG_NAME ".memslap.cnf"

#define DEFAULT_THREADS_NUM        1                       /* default start one thread */
#define DEFAULT_CONNS_NUM          16                      /* default each thread with 16 connections */
#define DEFAULT_EXE_NUM            0                       /* default execute number is 0 */
#define DEFAULT_VERIFY_RATE        0.0                     /* default it doesn't do data verification */
#define DEFAULT_OVERWRITE_RATE     0.0                     /* default it doesn't do overwrite */
#define DEFAULT_DIV                1                       /* default it runs single get */
#define DEFAULT_RUN_TIME           600                     /* default run time 10 minutes */
#define DEFAULT_WINDOW_SIZE        (10 * UNIT_ITEMS_COUNT) /* default window size is 10k */
#define DEFAULT_SOCK_PER_CONN      1                       /* default socks per connection is 1 */

/* Use this for string generation */
#define CHAR_COUNT                 64 /* number of characters used to generate character table */
const char ALPHANUMBERICS[]=
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-";

ms_setting_st ms_setting;       /* store the settings specified by user */


/* read setting from configuration file */
static void ms_get_serverlist(char *str);
static uint32_t ms_get_cpu_count(void);
ms_conf_type_t ms_get_conf_type(char *line);
static int ms_is_line_data(char *line);
static int ms_read_is_data(char *line, ssize_t nread);
static void ms_no_config_file(void);
static void ms_parse_cfg_file(char *cfg_file);


/* initialize setting structure */
static void ms_init_random_block(void);
static void ms_calc_avg_size(void);
static int ms_shuffle_distr(ms_distr_t *distr, int length);
static void ms_build_distr(void);
static void ms_print_setting(void);
static void ms_setting_slapmode_init_pre(void);
static void ms_setting_slapmode_init_post(void);

#if !defined(HAVE_GETLINE)
#include <limits.h>
static ssize_t getline (char **line, size_t *line_size, FILE *fp)
{
  char delim= '\n';
  ssize_t result= 0;
  size_t cur_len= 0;

  if (line == NULL || line_size == NULL || fp == NULL)
  {
    errno = EINVAL;
    return -1;
  }

  if (*line == NULL || *line_size == 0)
  {
    char *new_line;
    *line_size = 120;
    new_line= (char *) realloc (*line, *line_size);
    if (new_line == NULL)
    {
      result= -1;
      return result;
    }
    *line= new_line;
  }

  for (;;)
  {
    int i= getc(fp);
    if (i == EOF)
    {
      result = -1;
      break;
    }

    /* Make enough space for len+1 (for final NUL) bytes.  */
    if (cur_len + 1 >= *line_size)
    {
      size_t needed_max=
        SSIZE_MAX < SIZE_MAX ? (size_t) SSIZE_MAX + 1 : SIZE_MAX;
      size_t needed= (2 * (*line_size)) + 1;
      char *new_line;

      if (needed_max < needed)
        needed= needed_max;
      if (cur_len + 1 >= needed)
      {
        result= -1;
        errno= EOVERFLOW;
        return result;
      }

      new_line= (char *)realloc(*line, needed);
      if (new_line == NULL)
      {
        result= -1;
        return result;
      }

      *line= new_line;
      *line_size= needed;
    }

    (*line)[cur_len]= (char)i;
    cur_len++;

    if (i == delim)
      break;
  }
  (*line)[cur_len] = '\0';
  if (cur_len != 0)
    return (ssize_t)cur_len;
  return result;
}
#endif

/**
 * parse the server list string, and build the servers
 * information structure array. this function is used to parse
 * the command line options specified by user.
 *
 * @param str, the string of server list
 */
static void ms_get_serverlist(char *str)
{
  ms_mcd_server_t *srvs= NULL;

  /**
   * Servers list format is like this. For example:
   * "localhost:11108, localhost:11109"
   */
  memcached_server_st *server_pool;
  server_pool = memcached_servers_parse(str);

  for (uint32_t loop= 0; loop < memcached_server_list_count(server_pool); loop++)
  {
    assert(ms_setting.srv_cnt < ms_setting.total_srv_cnt);
    strcpy(ms_setting.servers[ms_setting.srv_cnt].srv_host_name, server_pool[loop].hostname);
    ms_setting.servers[ms_setting.srv_cnt].srv_port= server_pool[loop].port;
    ms_setting.servers[ms_setting.srv_cnt].disconn_cnt= 0;
    ms_setting.servers[ms_setting.srv_cnt].reconn_cnt= 0;
    ms_setting.srv_cnt++;

    if (ms_setting.srv_cnt >= ms_setting.total_srv_cnt)
    {
      srvs= (ms_mcd_server_t *)realloc( ms_setting.servers,
                                        (size_t)ms_setting.total_srv_cnt * sizeof(ms_mcd_server_t) * 2);
      if (srvs == NULL)
      {
        fprintf(stderr, "Can't reallocate servers structure.\n");
        exit(1);
      }
      ms_setting.servers= srvs;
      ms_setting.total_srv_cnt*= 2;
    }
  }

  memcached_server_free(server_pool);
} /* ms_get_serverlist */


/**
 * used to get the CPU count of the current system
 *
 * @return return the cpu count if get, else return 1
 */
static uint32_t ms_get_cpu_count()
{
#ifdef HAVE__SC_NPROCESSORS_ONLN
  return sysconf(_SC_NPROCESSORS_CONF);

#else
# ifdef HAVE_CPU_SET_T
  int cpu_count= 0;
  cpu_set_t cpu_set;

  sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);

  for (int i= 0; i < (sizeof(cpu_set_t) * 8); i++)
  {
    if (CPU_ISSET(i, &cpu_set))
    {
      cpu_count++;
    }
  }

  return cpu_count;

# endif
#endif

  /* the system with one cpu at least */
  return 1;
} /* ms_get_cpu_count */


/**
 * used to get the configure type based on the type string read
 * from the configuration file.
 *
 * @param line, string of one line
 *
 * @return ms_conf_type_t
 */
ms_conf_type_t ms_get_conf_type(char *line)
{
  if (! memcmp(line, "key", strlen("key")))
  {
    return CONF_KEY;
  }
  else if (! memcmp(line, "value", strlen("value")))
  {
    return CONF_VALUE;
  }
  else if (! memcmp(line, "cmd", strlen("cmd")))
  {
    return CONF_CMD;
  }
  else
  {
    return CONF_NULL;
  }
} /* ms_get_conf_type */


/**
 * judge whether the line is a line with useful data. used to
 * parse the configuration file.
 *
 * @param line, string of one line
 *
 * @return if success, return 1, else return 0
 */
static int ms_is_line_data(char *line)
{
  assert(line != NULL);

  char *begin_ptr= line;

  while (isspace(*begin_ptr))
  {
    begin_ptr++;
  }
  if ((begin_ptr[0] == '\0') || (begin_ptr[0] == '#'))
    return 0;

  return 1;
} /* ms_is_line_data */


/**
 * function to bypass blank line and comments
 *
 * @param line, string of one line
 * @param nread, length of the line
 *
 * @return if it's EOF or not line data, return 0, else return 1
 */
static int ms_read_is_data(char *line, ssize_t nread)
{
  if ((nread == EOF) || ! ms_is_line_data(line))
    return 0;

  return 1;
} /* ms_read_is_data */


/**
 *  if no configuration file, use this function to create the default
 *  configuration file.
 */
static void ms_no_config_file()
{
  char userpath[PATH_MAX];
  struct passwd *usr= NULL;
  FILE *fd;

  usr= getpwuid(getuid());

  snprintf(userpath, PATH_MAX, "%s/%s", usr->pw_dir, DEFAULT_CONFIG_NAME);

  if (access (userpath, F_OK | R_OK) == 0)
    goto exit;

  fd= fopen(userpath, "w+");

  if (fd == NULL)
  {
    fprintf(stderr, "Could not create default configure file %s\n", userpath);
    perror(strerror(errno));
    exit(1);
  }
  fprintf(fd, "%s", DEFAULT_CONGIF_STR);
  fclose(fd);

exit:
  ms_setting.cfg_file= strdup(userpath);
} /* ms_no_config_file */


/**
 * parse the configuration file
 *
 * @param cfg_file, the configuration file name
 */
static void ms_parse_cfg_file(char *cfg_file)
{
  FILE *f;
  size_t start_len, end_len;
  double proportion;
  char *line= NULL;
  size_t  read_len;
  ssize_t nread;
  int cmd_type;
  ms_conf_type_t conf_type;
  int end_of_file= 0;
  ms_key_distr_t *key_distr= NULL;
  ms_value_distr_t *val_distr= NULL;

  if (cfg_file == NULL)
  {
    ms_no_config_file();
    cfg_file= ms_setting.cfg_file;
  }

  /*read key value configure file*/
  if ((f= fopen(cfg_file, "r")) == NULL)
  {
    fprintf(stderr, "Can not open file: '%s'.\n", cfg_file);
    exit(1);
  }

  while (1)
  {
    if ((((nread= getline(&line, &read_len, f)) == 1)
         || ! ms_read_is_data(line, nread)) && (nread != EOF)) /* bypass blank line */
      continue;

    if (nread == EOF)
    {
      fprintf(stderr, "Bad configuration file, no configuration find.\n");
      exit(1);
    }
    conf_type= ms_get_conf_type(line);
    break;
  }

  while (! end_of_file)
  {
    switch (conf_type)
    {
    case CONF_KEY:
      while (1)
      {
        if ((((nread= getline(&line, &read_len, f)) == 1)
             || ! ms_read_is_data(line, nread)) && (nread != EOF))     /* bypass blank line */
          continue;

        if (nread != EOF)
        {
          if (sscanf(line, "%zu %zu %lf ", &start_len,
                     &end_len, &proportion) != 3)
          {
            conf_type= ms_get_conf_type(line);
            break;
          }
          ms_setting.key_distr[ms_setting.key_rng_cnt].start_len= start_len;
          ms_setting.key_distr[ms_setting.key_rng_cnt].end_len= end_len;
          ms_setting.key_distr[ms_setting.key_rng_cnt].key_prop= proportion;
          ms_setting.key_rng_cnt++;

          if (ms_setting.key_rng_cnt >= ms_setting.total_key_rng_cnt)
          {
            key_distr= (ms_key_distr_t *)realloc(
              ms_setting.key_distr,
              (size_t)ms_setting.
                 total_key_rng_cnt * sizeof(ms_key_distr_t) * 2);
            if (key_distr == NULL)
            {
              fprintf(stderr,
                      "Can't reallocate key distribution structure.\n");
              exit(1);
            }
            ms_setting.key_distr= key_distr;
            ms_setting.total_key_rng_cnt*= 2;
          }
          continue;
        }
        end_of_file= 1;
        break;
      }
      break;

    case CONF_VALUE:
      while (1)
      {
        if ((((nread= getline(&line, &read_len, f)) == 1)
             || ! ms_read_is_data(line, nread)) && (nread != EOF))     /* bypass blank line */
          continue;

        if (nread != EOF)
        {
          if (sscanf(line, "%zu %zu %lf", &start_len, &end_len,
                     &proportion) != 3)
          {
            conf_type= ms_get_conf_type(line);
            break;
          }
          ms_setting.value_distr[ms_setting.val_rng_cnt].start_len=
            start_len;
          ms_setting.value_distr[ms_setting.val_rng_cnt].end_len= end_len;
          ms_setting.value_distr[ms_setting.val_rng_cnt].value_prop=
            proportion;
          ms_setting.val_rng_cnt++;

          if (ms_setting.val_rng_cnt >= ms_setting.total_val_rng_cnt)
          {
            val_distr= (ms_value_distr_t *)realloc(
              ms_setting.value_distr,
              (size_t)ms_setting.
                 total_val_rng_cnt * sizeof(ms_value_distr_t) * 2);
            if (val_distr == NULL)
            {
              fprintf(stderr,
                      "Can't reallocate key distribution structure.\n");
              exit(1);
            }
            ms_setting.value_distr= val_distr;
            ms_setting.total_val_rng_cnt*= 2;
          }
          continue;
        }
        end_of_file= 1;
        break;
      }
      break;

    case CONF_CMD:
      while (1)
      {
        if ((((nread= getline(&line, &read_len, f)) == 1)
             || ! ms_read_is_data(line, nread)) && (nread != EOF))     /* bypass blank line */
          continue;

        if (nread != EOF)
        {
          if (sscanf(line, "%d %lf", &cmd_type, &proportion) != 2)
          {
            conf_type= ms_get_conf_type(line);
            break;
          }
          if (cmd_type >= CMD_NULL)
          {
            continue;
          }
          ms_setting.cmd_distr[ms_setting.cmd_used_count].cmd_type=
            cmd_type;
          ms_setting.cmd_distr[ms_setting.cmd_used_count].cmd_prop=
            proportion;
          ms_setting.cmd_used_count++;
          continue;
        }
        end_of_file= 1;
        break;
      }

    case CONF_NULL:
      while (1)
      {
        if ((((nread= getline(&line, &read_len, f)) == 1)
             || ! ms_read_is_data(line, nread)) && (nread != EOF))     /* bypass blank line */
          continue;

        if (nread != EOF)
        {
          if ((conf_type= ms_get_conf_type(line)) != CONF_NULL)
          {
            break;
          }
          continue;
        }
        end_of_file= 1;
        break;
      }
      break;

    default:
      assert(0);
      break;
    } /* switch */
  }

  fclose(f);

  if (line != NULL)
  {
    free(line);
  }
} /* ms_parse_cfg_file */


/* calculate the average size of key and value */
static void ms_calc_avg_size()
{
  double avg_val_size= 0.0;
  double avg_key_size= 0.0;
  double val_pro= 0.0;
  double key_pro= 0.0;
  double averge_len= 0.0;
  size_t start_len= 0;
  size_t end_len= 0;

  for (int j= 0; j < ms_setting.val_rng_cnt; j++)
  {
    val_pro= ms_setting.value_distr[j].value_prop;
    start_len= ms_setting.value_distr[j].start_len;
    end_len= ms_setting.value_distr[j].end_len;

    averge_len= val_pro * ((double)(start_len + end_len)) / 2;
    avg_val_size+= averge_len;
  }

  for (int j= 0; j < ms_setting.key_rng_cnt; j++)
  {
    key_pro= ms_setting.key_distr[j].key_prop;
    start_len= ms_setting.key_distr[j].start_len;
    end_len= ms_setting.key_distr[j].end_len;

    averge_len= key_pro * ((double)(start_len + end_len)) / 2;
    avg_key_size+= averge_len;
  }

  ms_setting.avg_val_size= (size_t)avg_val_size;
  ms_setting.avg_key_size= (size_t)avg_key_size;
} /* ms_calc_avg_size */


/**
 * used to shuffle key and value distribution array to ensure
 * (key, value) pair with different set.
 *
 * @param distr, pointer of distribution structure array
 * @param length, length of the array
 *
 * @return always return 0
 */
static int ms_shuffle_distr(ms_distr_t *distr, int length)
{
  int i, j;
  int tmp_offset;
  size_t  tmp_size;
  int64_t rnd;

  for (i= 0; i < length; i++)
  {
    rnd= random();
    j= (int)(rnd % (length - i)) + i;

    switch (rnd % 3)
    {
    case 0:
      tmp_size= distr[j].key_size;
      distr[j].key_size= distr[i].key_size;
      distr[i].key_size= tmp_size;
      break;

    case 1:
      tmp_offset= distr[j].key_offset;
      distr[j].key_offset= distr[i].key_offset;
      distr[i].key_offset= tmp_offset;
      break;

    case 2:
      tmp_size= distr[j].value_size;
      distr[j].value_size= distr[i].value_size;
      distr[i].value_size= tmp_size;
      break;

    default:
      break;
    } /* switch */
  }

  return 0;
} /* ms_shuffle_distr */


/**
 * according to the key and value distribution, to build the
 * (key, value) pair distribution. the (key, value) pair
 * distribution array is global, each connection set or get
 * object keeping this distribution, for the final result, we
 * can reach the expected key and value distribution.
 */
static void ms_build_distr()
{
  int offset= 0;
  int end= 0;
  int key_cnt= 0;
  int value_cnt= 0;
  size_t average_len= 0;
  size_t diff_len= 0;
  size_t start_len= 0;
  size_t end_len= 0;
  int rnd= 0;
  ms_distr_t *distr= NULL;
  int units= (int)ms_setting.win_size / UNIT_ITEMS_COUNT;

  /* calculate average value size and key size */
  ms_calc_avg_size();

  ms_setting.char_blk_size= RAND_CHAR_SIZE;
  int key_scope_size=
    (int)((ms_setting.char_blk_size - RESERVED_RAND_CHAR_SIZE)
          / UNIT_ITEMS_COUNT);

  ms_setting.distr= (ms_distr_t *)malloc(
    sizeof(ms_distr_t) * ms_setting.win_size);
  if (ms_setting.distr == NULL)
  {
    fprintf(stderr, "Can't allocate distribution array.");
    exit(1);
  }

  /**
   *  character block is divided by how many different key
   *  size, each different key size has the same size character
   *  range.
   */
  for (int m= 0; m < units; m++)
  {
    for (int i= 0; i < UNIT_ITEMS_COUNT; i++)
    {
      ms_setting.distr[m * UNIT_ITEMS_COUNT + i].key_offset=
        ADDR_ALIGN(key_scope_size * i);
    }
  }

  /* initialize key size distribution */
  for (int m= 0; m < units; m++)
  {
    for (int j= 0; j < ms_setting.key_rng_cnt; j++)
    {
      key_cnt= (int)(UNIT_ITEMS_COUNT * ms_setting.key_distr[j].key_prop);
      start_len= ms_setting.key_distr[j].start_len;
      end_len= ms_setting.key_distr[j].end_len;
      if ((start_len < MIN_KEY_SIZE) || (end_len < MIN_KEY_SIZE))
      {
        fprintf(stderr, "key length must be greater than 16 bytes.\n");
        exit(1);
      }

      if (! ms_setting.binary_prot
          && ((start_len > MAX_KEY_SIZE) || (end_len > MAX_KEY_SIZE)))
      {
        fprintf(stderr, "key length must be less than 250 bytes.\n");
        exit(1);
      }

      average_len= (start_len + end_len) / 2;
      diff_len= (end_len - start_len) / 2;
      for (int k= 0; k < key_cnt; k++)
      {
        if (offset >= (m + 1) * UNIT_ITEMS_COUNT)
        {
          break;
        }
        rnd= (int)random();
        if (k % 2 == 0)
        {
          ms_setting.distr[offset].key_size=
            (diff_len == 0) ? average_len :
            average_len + (size_t)rnd
            % diff_len;
        }
        else
        {
          ms_setting.distr[offset].key_size=
            (diff_len == 0) ? average_len :
            average_len - (size_t)rnd
            % diff_len;
        }
        offset++;
      }
    }

    if (offset < (m + 1) * UNIT_ITEMS_COUNT)
    {
      end= (m + 1) * UNIT_ITEMS_COUNT - offset;
      for (int i= 0; i < end; i++)
      {
        ms_setting.distr[offset].key_size= ms_setting.avg_key_size;
        offset++;
      }
    }
  }
  offset= 0;

  /* initialize value distribution */
  if (ms_setting.fixed_value_size != 0)
  {
    for (int i= 0; i < units * UNIT_ITEMS_COUNT; i++)
    {
      ms_setting.distr[i].value_size= ms_setting.fixed_value_size;
    }
  }
  else
  {
    for (int m= 0; m < units; m++)
    {
      for (int j= 0; j < ms_setting.val_rng_cnt; j++)
      {
        value_cnt=
          (int)(UNIT_ITEMS_COUNT * ms_setting.value_distr[j].value_prop);
        start_len= ms_setting.value_distr[j].start_len;
        end_len= ms_setting.value_distr[j].end_len;
        if ((start_len <= 0) || (end_len <= 0))
        {
          fprintf(stderr, "value length must be greater than 0 bytes.\n");
          exit(1);
        }

        if ((start_len > MAX_VALUE_SIZE) || (end_len > MAX_VALUE_SIZE))
        {
          fprintf(stderr, "key length must be less than or equal to 1M.\n");
          exit(1);
        }

        average_len= (start_len + end_len) / 2;
        diff_len= (end_len - start_len) / 2;
        for (int k= 0; k < value_cnt; k++)
        {
          if (offset >= (m + 1) * UNIT_ITEMS_COUNT)
          {
            break;
          }
          rnd= (int)random();
          if (k % 2 == 0)
          {
            ms_setting.distr[offset].value_size=
              (diff_len == 0) ? average_len :
              average_len
              + (size_t)rnd % diff_len;
          }
          else
          {
            ms_setting.distr[offset].value_size=
              (diff_len == 0) ? average_len :
              average_len
              - (size_t)rnd % diff_len;
          }
          offset++;
        }
      }

      if (offset < (m + 1) * UNIT_ITEMS_COUNT)
      {
        end= (m + 1) * UNIT_ITEMS_COUNT - offset;
        for (int i= 0; i < end; i++)
        {
          ms_setting.distr[offset++].value_size= ms_setting.avg_val_size;
        }
      }
    }
  }

  /* shuffle distribution */
  for (int i= 0; i < units; i++)
  {
    distr= &ms_setting.distr[i * UNIT_ITEMS_COUNT];
    for (int j= 0; j < 4; j++)
    {
      ms_shuffle_distr(distr, UNIT_ITEMS_COUNT);
    }
  }
} /* ms_build_distr */


/**
 * used to initialize the global character block. The character
 * block is used to generate the suffix of the key and value. we
 * only store a pointer in the character block for each key
 * suffix or value string. It can save much memory to store key
 * or value string.
 */
static void ms_init_random_block()
{
  char *ptr= NULL;

  assert(ms_setting.char_blk_size > 0);

  ms_setting.char_block= (char *)malloc(ms_setting.char_blk_size);
  if (ms_setting.char_block == NULL)
  {
    fprintf(stderr, "Can't allocate global char block.");
    exit(1);
  }
  ptr= ms_setting.char_block;

  for (int i= 0; (size_t)i < ms_setting.char_blk_size; i++)
  {
    *(ptr++)= ALPHANUMBERICS[random() % CHAR_COUNT];
  }
} /* ms_init_random_block */


/**
 * after initialization, call this function to output the main
 * configuration user specified.
 */
static void ms_print_setting()
{
  fprintf(stdout, "servers : %s\n", ms_setting.srv_str);
  fprintf(stdout, "threads count: %d\n", ms_setting.nthreads);
  fprintf(stdout, "concurrency: %d\n", ms_setting.nconns);
  if (ms_setting.run_time > 0)
  {
    fprintf(stdout, "run time: %ds\n", ms_setting.run_time);
  }
  else
  {
    fprintf(stdout, "execute number: %" PRId64 "\n", ms_setting.exec_num);
  }
  fprintf(stdout, "windows size: %" PRId64 "k\n",
          (int64_t)(ms_setting.win_size / 1024));
  fprintf(stdout, "set proportion: set_prop=%.2f\n",
          ms_setting.cmd_distr[CMD_SET].cmd_prop);
  fprintf(stdout, "get proportion: get_prop=%.2f\n",
          ms_setting.cmd_distr[CMD_GET].cmd_prop);
  fflush(stdout);
} /* ms_print_setting */


/**
 * previous part of slap mode initialization of setting structure
 */
static void ms_setting_slapmode_init_pre()
{
  ms_setting.exec_num= DEFAULT_EXE_NUM;
  ms_setting.verify_percent= DEFAULT_VERIFY_RATE;
  ms_setting.exp_ver_per= DEFAULT_VERIFY_RATE;
  ms_setting.overwrite_percent= DEFAULT_OVERWRITE_RATE;
  ms_setting.mult_key_num= DEFAULT_DIV;
  ms_setting.fixed_value_size= 0;
  ms_setting.win_size= DEFAULT_WINDOW_SIZE;
  ms_setting.udp= false;
  ms_setting.reconnect= false;
  ms_setting.verbose= false;
  ms_setting.facebook_test= false;
  ms_setting.binary_prot= false;
  ms_setting.stat_freq= 0;
  ms_setting.srv_str= NULL;
  ms_setting.cfg_file= NULL;
  ms_setting.sock_per_conn= DEFAULT_SOCK_PER_CONN;
  ms_setting.expected_tps= 0;
  ms_setting.rep_write_srv= 0;
} /* ms_setting_slapmode_init_pre */


/**
 * previous part of initialization of setting structure
 */
void ms_setting_init_pre()
{
  memset(&ms_setting, 0, sizeof(ms_setting));

  /* common initialize */
  ms_setting.ncpu= ms_get_cpu_count();
  ms_setting.nthreads= DEFAULT_THREADS_NUM;
  ms_setting.nconns= DEFAULT_CONNS_NUM;
  ms_setting.run_time= DEFAULT_RUN_TIME;
  ms_setting.total_srv_cnt= MCD_SRVS_NUM_INIT;
  ms_setting.servers= (ms_mcd_server_t *)malloc(
    (size_t)ms_setting.total_srv_cnt
    * sizeof(ms_mcd_server_t));
  if (ms_setting.servers == NULL)
  {
    fprintf(stderr, "Can't allocate servers structure.\n");
    exit(1);
  }

  ms_setting_slapmode_init_pre();
} /* ms_setting_init_pre */


/**
 * post part of slap mode initialization of setting structure
 */
static void ms_setting_slapmode_init_post()
{
  ms_setting.total_key_rng_cnt= KEY_RANGE_COUNT_INIT;
  ms_setting.key_distr=
    (ms_key_distr_t *)malloc((size_t)ms_setting.total_key_rng_cnt * sizeof(ms_key_distr_t));

  if (ms_setting.key_distr == NULL)
  {
    fprintf(stderr, "Can't allocate key distribution structure.\n");
    exit(1);
  }

  ms_setting.total_val_rng_cnt= VALUE_RANGE_COUNT_INIT;

  ms_setting.value_distr=
    (ms_value_distr_t *)malloc((size_t)ms_setting.total_val_rng_cnt * sizeof( ms_value_distr_t));

  if (ms_setting.value_distr == NULL)
  {
    fprintf(stderr, "Can't allocate value distribution structure.\n");
    exit(1);
  }

  ms_parse_cfg_file(ms_setting.cfg_file);

  /* run time mode */
  if ((ms_setting.exec_num == 0) && (ms_setting.run_time != 0))
  {
    ms_setting.exec_num= (int64_t)MAX_EXEC_NUM;
  }
  else
  {
    /* execute number mode */
    ms_setting.run_time= 0;
  }

  if (ms_setting.rep_write_srv > 0)
  {
    /* for replication test, need enable reconnect feature */
    ms_setting.reconnect= true;
  }

  if (ms_setting.facebook_test && (ms_setting.mult_key_num < 2))
  {
    fprintf(stderr, "facebook test must work with multi-get, "
                    "please specify multi-get key number "
                    "with '--division' option.\n");
    exit(1);
  }

  if (ms_setting.facebook_test && ms_setting.udp)
  {
    fprintf(stderr, "facebook test couldn't work with UDP.\n");
    exit(1);
  }

  if (ms_setting.udp && (ms_setting.sock_per_conn > 1))
  {
    fprintf(stderr, "UDP doesn't support multi-socks "
                    "in one connection structure.\n");
    exit(1);
  }

  if ((ms_setting.rep_write_srv > 0) && (ms_setting.srv_cnt < 2))
  {
    fprintf(stderr, "Please specify 2 servers at least for replication\n");
    exit(1);
  }

  if ((ms_setting.rep_write_srv > 0)
      && (ms_setting.srv_cnt < ms_setting.rep_write_srv))
  {
    fprintf(stderr, "Servers to do replication writing "
                    "is larger than the total servers\n");
    exit(1);
  }

  if (ms_setting.udp && (ms_setting.rep_write_srv > 0))
  {
    fprintf(stderr, "UDP doesn't support replication.\n");
    exit(1);
  }

  if (ms_setting.facebook_test && (ms_setting.rep_write_srv > 0))
  {
    fprintf(stderr, "facebook test couldn't work with replication.\n");
    exit(1);
  }

  ms_build_distr();

  /* initialize global character block */
  ms_init_random_block();
  ms_print_setting();
} /* ms_setting_slapmode_init_post */


/**
 * post part of initialization of setting structure
 */
void ms_setting_init_post()
{
  ms_get_serverlist(ms_setting.srv_str);
  ms_setting_slapmode_init_post();
}


/**
 * clean up the global setting structure
 */
void ms_setting_cleanup()
{
  if (ms_setting.distr != NULL)
  {
    free(ms_setting.distr);
  }

  if (ms_setting.char_block != NULL)
  {
    free(ms_setting.char_block);
  }

  if (ms_setting.srv_str != NULL)
  {
    free(ms_setting.srv_str);
  }

  if (ms_setting.cfg_file != NULL)
  {
    free(ms_setting.cfg_file);
  }

  if (ms_setting.servers != NULL)
  {
    free(ms_setting.servers);
  }

  if (ms_setting.key_distr != NULL)
  {
    free(ms_setting.key_distr);
  }

  if (ms_setting.value_distr != NULL)
  {
    free(ms_setting.value_distr);
  }
} /* ms_setting_cleanup */
