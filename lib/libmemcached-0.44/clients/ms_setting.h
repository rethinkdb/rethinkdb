/*
 * File:   ms_setting.h
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */
#ifndef MS_SETTING_H
#define MS_SETTING_H

#include "ms_memslap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCD_SRVS_NUM_INIT         8
#define MCD_HOST_LENGTH           64
#define KEY_RANGE_COUNT_INIT      8
#define VALUE_RANGE_COUNT_INIT    8
#define PROP_ERROR                0.001

#define MIN_KEY_SIZE              16
#define MAX_KEY_SIZE              250
#define MAX_VALUE_SIZE            (1024 * 1024)

/* the content of the configuration file for memslap running without configuration file */
#define DEFAULT_CONGIF_STR \
  "key\n"                  \
  "64 64 1\n"              \
  "value\n"                \
  "1024 1024 1\n"          \
  "cmd\n"                  \
  "0 0.1\n"                \
  "1 0.9"

/* Used to parse the value length return by server and path string */
typedef struct token_s
{
  char *value;
  size_t length;
} token_t;

#define MAX_TOKENS    10

/* server information */
typedef struct mcd_sever
{
  char srv_host_name[MCD_HOST_LENGTH];              /* host name of server */
  int srv_port;                                     /* server port */

  /* for calculating how long the server disconnects */
  volatile uint32_t disconn_cnt;                    /* number of disconnections count */
  volatile uint32_t reconn_cnt;                     /* number of reconnections count */
  struct timeval disconn_time;                      /* start time of disconnection */
  struct timeval reconn_time;                       /* end time of reconnection */
} ms_mcd_server_t;

/* information of an item distribution including key and value */
typedef struct distr
{
  size_t key_size;                  /* size of key */
  int key_offset;                   /* offset of one key in character block */
  size_t value_size;                /* size of value */
} ms_distr_t;

/* information of key distribution */
typedef struct key_distr
{
  size_t start_len;                 /* start of the key length range */
  size_t end_len;                   /* end of the key length range */
  double key_prop;                  /* key proportion */
} ms_key_distr_t;

/* information of value distribution */
typedef struct value_distr
{
  size_t start_len;                 /* start of the value length range */
  size_t end_len;                   /* end of the value length range */
  double value_prop;                /* value proportion */
} ms_value_distr_t;

/* memcached command types */
typedef enum cmd_type
{
  CMD_SET,
  CMD_GET,
  CMD_NULL,
} ms_cmd_type_t;

/* types in the configuration file */
typedef enum conf_type
{
  CONF_KEY,
  CONF_VALUE,
  CONF_CMD,
  CONF_NULL,
} ms_conf_type_t;

/* information of command distribution */
typedef struct cmd_distr
{
  ms_cmd_type_t cmd_type;               /* command type */
  double cmd_prop;                      /* proportion of the command */
} ms_cmd_distr_t;

/* global setting structure */
typedef struct setting
{
  uint32_t ncpu;                             /* cpu count of this system */
  uint32_t nthreads;                         /* total thread count, must equal or less than cpu cores */
  uint32_t nconns;                      /* total conn count, must multiply by total thread count */
  int64_t exec_num;                     /* total execute number */
  int run_time;                         /* total run time */

  uint32_t char_blk_size;               /* global character block size */
  char *char_block;                     /* global character block with random character */
  ms_distr_t *distr;                    /* distribution from configure file */

  char *srv_str;                        /* string includes servers information */
  char *cfg_file;                       /* configure file name */

  ms_mcd_server_t *servers;             /* servers array */
  uint32_t total_srv_cnt;                    /* total servers count of the servers array */
  uint32_t srv_cnt;                          /* servers count */

  ms_key_distr_t *key_distr;            /* array of key distribution */
  int total_key_rng_cnt;                /* total key range count of the array */
  int key_rng_cnt;                      /* actual key range count */

  ms_value_distr_t *value_distr;        /* array of value distribution */
  int total_val_rng_cnt;                /* total value range count of the array */
  int val_rng_cnt;                      /* actual value range count */

  ms_cmd_distr_t cmd_distr[CMD_NULL];   /* total we have CMD_NULL commands */
  int cmd_used_count;                   /* supported command count */

  size_t fixed_value_size;              /* fixed value size */
  size_t avg_val_size;                  /* average value size */
  size_t avg_key_size;                  /* average value size */

  double verify_percent;                /* percent of data verification */
  double exp_ver_per;                   /* percent of data verification with expire time */
  double overwrite_percent;             /* percent of overwrite */
  int mult_key_num;                     /* number of keys used by multi-get once */
  size_t win_size;                      /* item window size per connection */
  bool udp;                             /* whether or not use UDP */
  int stat_freq;                        /* statistic frequency second */
  bool reconnect;                       /* whether it reconnect when connection close */
  bool verbose;                         /* whether it outputs detailed information when verification */
  bool facebook_test;                   /* facebook test, TCP set and multi-get with UDP */
  uint32_t sock_per_conn;                    /* number of socks per connection structure */
  bool binary_prot;                     /* whether it use binary protocol */
  int expected_tps;                     /* expected throughput */
  uint32_t rep_write_srv;                    /* which servers are used to do replication writing */
} ms_setting_st;

extern ms_setting_st ms_setting;

/* previous part of initialization of setting structure */
void ms_setting_init_pre(void);


/* post part of initialization of setting structure */
void ms_setting_init_post(void);


/* clean up the global setting structure */
void ms_setting_cleanup(void);


#define UNUSED_ARGUMENT(x)    (void)x

#ifdef __cplusplus
}
#endif

#endif /* end of MS_SETTING_H */
