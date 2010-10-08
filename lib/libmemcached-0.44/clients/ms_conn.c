/*
 * File:   ms_conn.c
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */

#include "config.h"

#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/uio.h>
#include <event.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
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
#include "ms_setting.h"
#include "ms_thread.h"
#include "ms_atomic.h"

#ifdef linux
/* /usr/include/netinet/in.h defines macros from ntohs() to _bswap_nn to
 * optimize the conversion functions, but the prototypes generate warnings
 * from gcc. The conversion methods isn't the bottleneck for my app, so
 * just remove the warnings by undef'ing the optimization ..
 */
#undef ntohs
#undef ntohl
#undef htons
#undef htonl
#endif

/* for network write */
#define TRANSMIT_COMPLETE      0
#define TRANSMIT_INCOMPLETE    1
#define TRANSMIT_SOFT_ERROR    2
#define TRANSMIT_HARD_ERROR    3

/* for generating key */
#define KEY_PREFIX_BASE        0x1010101010101010 /* not include ' ' '\r' '\n' '\0' */
#define KEY_PREFIX_MASK        0x1010101010101010

/* For parse the value length return by server */
#define KEY_TOKEN              1
#define VALUELEN_TOKEN         3

/* global increasing counter, to ensure the key prefix unique */
static uint64_t key_prefix_seq= KEY_PREFIX_BASE;

/* global increasing counter, generating request id for UDP */
static volatile uint32_t udp_request_id= 0;

extern pthread_key_t ms_thread_key;

/* generate upd request id */
static uint32_t ms_get_udp_request_id(void);


/* connect initialize */
static void ms_task_init(ms_conn_t *c);
static int ms_conn_udp_init(ms_conn_t *c, const bool is_udp);
static int ms_conn_sock_init(ms_conn_t *c);
static int ms_conn_event_init(ms_conn_t *c);
static int ms_conn_init(ms_conn_t *c,
                        const int init_state,
                        const int read_buffer_size,
                        const bool is_udp);
static void ms_warmup_num_init(ms_conn_t *c);
static int ms_item_win_init(ms_conn_t *c);


/* connection close */
void ms_conn_free(ms_conn_t *c);
static void ms_conn_close(ms_conn_t *c);


/* create network connection */
static int ms_new_socket(struct addrinfo *ai);
static void ms_maximize_sndbuf(const int sfd);
static int ms_network_connect(ms_conn_t *c,
                              char *srv_host_name,
                              const int srv_port,
                              const bool is_udp,
                              int *ret_sfd);
static int ms_reconn(ms_conn_t *c);


/* read and parse */
static int ms_tokenize_command(char *command,
                               token_t *tokens,
                               const int max_tokens);
static int ms_ascii_process_line(ms_conn_t *c, char *command);
static int ms_try_read_line(ms_conn_t *c);
static int ms_sort_udp_packet(ms_conn_t *c, char *buf, int rbytes);
static int ms_udp_read(ms_conn_t *c, char *buf, int len);
static int ms_try_read_network(ms_conn_t *c);
static void ms_verify_value(ms_conn_t *c,
                            ms_mlget_task_item_t *mlget_item,
                            char *value,
                            int vlen);
static void ms_ascii_complete_nread(ms_conn_t *c);
static void ms_bin_complete_nread(ms_conn_t *c);
static void ms_complete_nread(ms_conn_t *c);


/* send functions */
static int ms_add_msghdr(ms_conn_t *c);
static int ms_ensure_iov_space(ms_conn_t *c);
static int ms_add_iov(ms_conn_t *c, const void *buf, int len);
static int ms_build_udp_headers(ms_conn_t *c);
static int ms_transmit(ms_conn_t *c);


/* status adjustment */
static void ms_conn_shrink(ms_conn_t *c);
static void ms_conn_set_state(ms_conn_t *c, int state);
static bool ms_update_event(ms_conn_t *c, const int new_flags);
static uint32_t ms_get_rep_sock_index(ms_conn_t *c, int cmd);
static uint32_t ms_get_next_sock_index(ms_conn_t *c);
static int ms_update_conn_sock_event(ms_conn_t *c);
static bool ms_need_yield(ms_conn_t *c);
static void ms_update_start_time(ms_conn_t *c);


/* main loop */
static void ms_drive_machine(ms_conn_t *c);
void ms_event_handler(const int fd, const short which, void *arg);


/* ascii protocol */
static int ms_build_ascii_write_buf_set(ms_conn_t *c, ms_task_item_t *item);
static int ms_build_ascii_write_buf_get(ms_conn_t *c, ms_task_item_t *item);
static int ms_build_ascii_write_buf_mlget(ms_conn_t *c);


/* binary protocol */
static int ms_bin_process_response(ms_conn_t *c);
static void ms_add_bin_header(ms_conn_t *c,
                              uint8_t opcode,
                              uint8_t hdr_len,
                              uint16_t key_len,
                              uint32_t body_len);
static void ms_add_key_to_iov(ms_conn_t *c, ms_task_item_t *item);
static int ms_build_bin_write_buf_set(ms_conn_t *c, ms_task_item_t *item);
static int ms_build_bin_write_buf_get(ms_conn_t *c, ms_task_item_t *item);
static int ms_build_bin_write_buf_mlget(ms_conn_t *c);


/**
 * each key has two parts, prefix and suffix. The suffix is a
 * string random get form the character table. The prefix is a
 * uint64_t variable. And the prefix must be unique. we use the
 * prefix to identify a key. And the prefix can't include
 * character ' ' '\r' '\n' '\0'.
 *
 * @return uint64_t
 */
uint64_t ms_get_key_prefix(void)
{
  uint64_t key_prefix;

  pthread_mutex_lock(&ms_global.seq_mutex);
  key_prefix_seq|= KEY_PREFIX_MASK;
  key_prefix= key_prefix_seq;
  key_prefix_seq++;
  pthread_mutex_unlock(&ms_global.seq_mutex);

  return key_prefix;
} /* ms_get_key_prefix */


/**
 * get an unique udp request id
 *
 * @return an unique UDP request id
 */
static uint32_t ms_get_udp_request_id(void)
{
  return atomic_add_32_nv(&udp_request_id, 1);
}


/**
 * initialize current task structure
 *
 * @param c, pointer of the concurrency
 */
static void ms_task_init(ms_conn_t *c)
{
  c->curr_task.cmd= CMD_NULL;
  c->curr_task.item= 0;
  c->curr_task.verify= false;
  c->curr_task.finish_verify= true;
  c->curr_task.get_miss= true;

  c->curr_task.get_opt= 0;
  c->curr_task.set_opt= 0;
  c->curr_task.cycle_undo_get= 0;
  c->curr_task.cycle_undo_set= 0;
  c->curr_task.verified_get= 0;
  c->curr_task.overwrite_set= 0;
} /* ms_task_init */


/**
 * initialize udp for the connection structure
 *
 * @param c, pointer of the concurrency
 * @param is_udp, whether it's udp
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_conn_udp_init(ms_conn_t *c, const bool is_udp)
{
  c->hdrbuf= 0;
  c->rudpbuf= 0;
  c->udppkt= 0;

  c->rudpsize= UDP_DATA_BUFFER_SIZE;
  c->hdrsize= 0;

  c->rudpbytes= 0;
  c->packets= 0;
  c->recvpkt= 0;
  c->pktcurr= 0;
  c->ordcurr= 0;

  c->udp= is_udp;

  if (c->udp || (! c->udp && ms_setting.facebook_test))
  {
    c->rudpbuf= (char *)malloc((size_t)c->rudpsize);
    c->udppkt= (ms_udppkt_t *)malloc(MAX_UDP_PACKET * sizeof(ms_udppkt_t));

    if ((c->rudpbuf == NULL) || (c->udppkt == NULL))
    {
      if (c->rudpbuf != NULL)
        free(c->rudpbuf);
      if (c->udppkt != NULL)
        free(c->udppkt);
      fprintf(stderr, "malloc()\n");
      return -1;
    }
    memset(c->udppkt, 0, MAX_UDP_PACKET * sizeof(ms_udppkt_t));
  }

  return 0;
} /* ms_conn_udp_init */


/**
 * initialize the connection structure
 *
 * @param c, pointer of the concurrency
 * @param init_state, (conn_read, conn_write, conn_closing)
 * @param read_buffer_size
 * @param is_udp, whether it's udp
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_conn_init(ms_conn_t *c,
                        const int init_state,
                        const int read_buffer_size,
                        const bool is_udp)
{
  assert(c != NULL);

  c->rbuf= c->wbuf= 0;
  c->iov= 0;
  c->msglist= 0;

  c->rsize= read_buffer_size;
  c->wsize= WRITE_BUFFER_SIZE;
  c->iovsize= IOV_LIST_INITIAL;
  c->msgsize= MSG_LIST_INITIAL;

  /* for replication, each connection need connect all the server */
  if (ms_setting.rep_write_srv > 0)
  {
    c->total_sfds= ms_setting.srv_cnt * ms_setting.sock_per_conn;
  }
  else
  {
    c->total_sfds= ms_setting.sock_per_conn;
  }
  c->alive_sfds= 0;

  c->rbuf= (char *)malloc((size_t)c->rsize);
  c->wbuf= (char *)malloc((size_t)c->wsize);
  c->iov= (struct iovec *)malloc(sizeof(struct iovec) * (size_t)c->iovsize);
  c->msglist= (struct msghdr *)malloc(
    sizeof(struct msghdr) * (size_t)c->msgsize);
  if (ms_setting.mult_key_num > 1)
  {
    c->mlget_task.mlget_item= (ms_mlget_task_item_t *)
                              malloc(
      sizeof(ms_mlget_task_item_t) * (size_t)ms_setting.mult_key_num);
  }
  c->tcpsfd= (int *)malloc((size_t)c->total_sfds * sizeof(int));

  if ((c->rbuf == NULL) || (c->wbuf == NULL) || (c->iov == NULL)
      || (c->msglist == NULL) || (c->tcpsfd == NULL)
      || ((ms_setting.mult_key_num > 1)
          && (c->mlget_task.mlget_item == NULL)))
  {
    if (c->rbuf != NULL)
      free(c->rbuf);
    if (c->wbuf != NULL)
      free(c->wbuf);
    if (c->iov != NULL)
      free(c->iov);
    if (c->msglist != NULL)
      free(c->msglist);
    if (c->mlget_task.mlget_item != NULL)
      free(c->mlget_task.mlget_item);
    if (c->tcpsfd != NULL)
      free(c->tcpsfd);
    fprintf(stderr, "malloc()\n");
    return -1;
  }

  c->state= init_state;
  c->rvbytes= 0;
  c->rbytes= 0;
  c->rcurr= c->rbuf;
  c->wcurr= c->wbuf;
  c->iovused= 0;
  c->msgcurr= 0;
  c->msgused= 0;
  c->cur_idx= c->total_sfds;       /* default index is a invalid value */

  c->ctnwrite= false;
  c->readval= false;
  c->change_sfd= false;

  c->precmd.cmd= c->currcmd.cmd= CMD_NULL;
  c->precmd.isfinish= true;         /* default the previous command finished */
  c->currcmd.isfinish= false;
  c->precmd.retstat= c->currcmd.retstat= MCD_FAILURE;
  c->precmd.key_prefix= c->currcmd.key_prefix= 0;

  c->mlget_task.mlget_num= 0;
  c->mlget_task.value_index= -1;         /* default invalid value */

  if (ms_setting.binary_prot)
  {
    c->protocol= binary_prot;
  }
  else
  {
    c->protocol= ascii_prot;
  }

  /* initialize udp */
  if (ms_conn_udp_init(c, is_udp) != 0)
  {
    return -1;
  }

  /* initialize task */
  ms_task_init(c);

  if (! (ms_setting.facebook_test && is_udp))
  {
    atomic_add_32(&ms_stats.active_conns, 1);
  }

  return 0;
} /* ms_conn_init */


/**
 * when doing 100% get operation, it could preset some objects
 * to warmup the server. this function is used to initialize the
 * number of the objects to preset.
 *
 * @param c, pointer of the concurrency
 */
static void ms_warmup_num_init(ms_conn_t *c)
{
  /* no set operation, preset all the items in the window  */
  if (ms_setting.cmd_distr[CMD_SET].cmd_prop < PROP_ERROR)
  {
    c->warmup_num= c->win_size;
    c->remain_warmup_num= c->warmup_num;
  }
  else
  {
    c->warmup_num= 0;
    c->remain_warmup_num= c->warmup_num;
  }
} /* ms_warmup_num_init */


/**
 * each connection has an item window, this function initialize
 * the window. The window is used to generate task.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_item_win_init(ms_conn_t *c)
{
  ms_thread_t *ms_thread= pthread_getspecific(ms_thread_key);
  int exp_cnt= 0;

  c->win_size= (int)ms_setting.win_size;
  c->set_cursor= 0;
  c->exec_num= ms_thread->thread_ctx->exec_num_perconn;
  c->remain_exec_num= c->exec_num;

  c->item_win= (ms_task_item_t *)malloc(
    sizeof(ms_task_item_t) * (size_t)c->win_size);
  if (c->item_win == NULL)
  {
    fprintf(stderr, "Can't allocate task item array for conn.\n");
    return -1;
  }
  memset(c->item_win, 0, sizeof(ms_task_item_t) * (size_t)c->win_size);

  for (int i= 0; i < c->win_size; i++)
  {
    c->item_win[i].key_size= (int)ms_setting.distr[i].key_size;
    c->item_win[i].key_prefix= ms_get_key_prefix();
    c->item_win[i].key_suffix_offset= ms_setting.distr[i].key_offset;
    c->item_win[i].value_size= (int)ms_setting.distr[i].value_size;
    c->item_win[i].value_offset= INVALID_OFFSET;         /* default in invalid offset */
    c->item_win[i].client_time= 0;

    /* set expire time base on the proportion */
    if (exp_cnt < ms_setting.exp_ver_per * i)
    {
      c->item_win[i].exp_time= FIXED_EXPIRE_TIME;
      exp_cnt++;
    }
    else
    {
      c->item_win[i].exp_time= 0;
    }
  }

  ms_warmup_num_init(c);

  return 0;
} /* ms_item_win_init */


/**
 * each connection structure can include one or more sock
 * handlers. this function create these socks and connect the
 * server(s).
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_conn_sock_init(ms_conn_t *c)
{
  ms_thread_t *ms_thread= pthread_getspecific(ms_thread_key);
  uint32_t i;
  int ret_sfd;
  uint32_t srv_idx= 0;

  assert(c != NULL);
  assert(c->tcpsfd != NULL);

  for (i= 0; i < c->total_sfds; i++)
  {
    ret_sfd= 0;
    if (ms_setting.rep_write_srv > 0)
    {
      /* for replication, each connection need connect all the server */
      srv_idx= i % ms_setting.srv_cnt;
    }
    else
    {
      /* all the connections in a thread connects the same server */
      srv_idx= ms_thread->thread_ctx->srv_idx;
    }

    if (ms_network_connect(c, ms_setting.servers[srv_idx].srv_host_name,
                           ms_setting.servers[srv_idx].srv_port,
                           ms_setting.udp, &ret_sfd) != 0)
    {
      break;
    }

    if (i == 0)
    {
      c->sfd= ret_sfd;
    }

    if (! ms_setting.udp)
    {
      c->tcpsfd[i]= ret_sfd;
    }

    c->alive_sfds++;
  }

  /* initialize udp sock handler if necessary */
  if (ms_setting.facebook_test)
  {
    ret_sfd= 0;
    if (ms_network_connect(c, ms_setting.servers[srv_idx].srv_host_name,
                           ms_setting.servers[srv_idx].srv_port,
                           true, &ret_sfd) != 0)
    {
      c->udpsfd= 0;
    }
    else
    {
      c->udpsfd= ret_sfd;
    }
  }

  if ((i != c->total_sfds) || (ms_setting.facebook_test && (c->udpsfd == 0)))
  {
    if (ms_setting.udp)
    {
      close(c->sfd);
    }
    else
    {
      for (uint32_t j= 0; j < i; j++)
      {
        close(c->tcpsfd[j]);
      }
    }

    if (c->udpsfd != 0)
    {
      close(c->udpsfd);
    }

    return -1;
  }

  return 0;
} /* ms_conn_sock_init */


/**
 * each connection is managed by libevent, this function
 * initialize the event of the connection structure.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_conn_event_init(ms_conn_t *c)
{
  ms_thread_t *ms_thread= pthread_getspecific(ms_thread_key);
  short event_flags= EV_WRITE | EV_PERSIST;

  event_set(&c->event, c->sfd, event_flags, ms_event_handler, (void *)c);
  event_base_set(ms_thread->base, &c->event);
  c->ev_flags= event_flags;

  if (event_add(&c->event, NULL) == -1)
  {
    return -1;
  }

  return 0;
} /* ms_conn_event_init */


/**
 * setup a connection, each connection structure of each
 * thread must call this function to initialize.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
int ms_setup_conn(ms_conn_t *c)
{
  if (ms_item_win_init(c) != 0)
  {
    return -1;
  }

  if (ms_conn_init(c, conn_write, DATA_BUFFER_SIZE, ms_setting.udp) != 0)
  {
    return -1;
  }

  if (ms_conn_sock_init(c) != 0)
  {
    return -1;
  }

  if (ms_conn_event_init(c) != 0)
  {
    return -1;
  }

  return 0;
} /* ms_setup_conn */


/**
 * Frees a connection.
 *
 * @param c, pointer of the concurrency
 */
void ms_conn_free(ms_conn_t *c)
{
  ms_thread_t *ms_thread= pthread_getspecific(ms_thread_key);
  if (c != NULL)
  {
    if (c->hdrbuf != NULL)
      free(c->hdrbuf);
    if (c->msglist != NULL)
      free(c->msglist);
    if (c->rbuf != NULL)
      free(c->rbuf);
    if (c->wbuf != NULL)
      free(c->wbuf);
    if (c->iov != NULL)
      free(c->iov);
    if (c->mlget_task.mlget_item != NULL)
      free(c->mlget_task.mlget_item);
    if (c->rudpbuf != NULL)
      free(c->rudpbuf);
    if (c->udppkt != NULL)
      free(c->udppkt);
    if (c->item_win != NULL)
      free(c->item_win);
    if (c->tcpsfd != NULL)
      free(c->tcpsfd);

    if (--ms_thread->nactive_conn == 0)
    {
      free(ms_thread->conn);
    }
  }
} /* ms_conn_free */


/**
 * close a connection
 *
 * @param c, pointer of the concurrency
 */
static void ms_conn_close(ms_conn_t *c)
{
  ms_thread_t *ms_thread= pthread_getspecific(ms_thread_key);
  assert(c != NULL);

  /* delete the event, the socket and the connection */
  event_del(&c->event);

  for (uint32_t i= 0; i < c->total_sfds; i++)
  {
    if (c->tcpsfd[i] > 0)
    {
      close(c->tcpsfd[i]);
    }
  }
  c->sfd= 0;

  if (ms_setting.facebook_test)
  {
    close(c->udpsfd);
  }

  atomic_dec_32(&ms_stats.active_conns);

  ms_conn_free(c);

  if (ms_setting.run_time == 0)
  {
    pthread_mutex_lock(&ms_global.run_lock.lock);
    ms_global.run_lock.count++;
    pthread_cond_signal(&ms_global.run_lock.cond);
    pthread_mutex_unlock(&ms_global.run_lock.lock);
  }

  if (ms_thread->nactive_conn == 0)
  {
    pthread_exit(NULL);
  }
} /* ms_conn_close */


/**
 * create a new sock
 *
 * @param ai, server address information
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_new_socket(struct addrinfo *ai)
{
  int sfd;

  if ((sfd= socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1)
  {
    fprintf(stderr, "socket() error: %s.\n", strerror(errno));
    return -1;
  }

  return sfd;
} /* ms_new_socket */


/**
 * Sets a socket's send buffer size to the maximum allowed by the system.
 *
 * @param sfd, file descriptor of socket
 */
static void ms_maximize_sndbuf(const int sfd)
{
  socklen_t intsize= sizeof(int);
  unsigned int last_good= 0;
  unsigned int min, max, avg;
  unsigned int old_size;

  /* Start with the default size. */
  if (getsockopt(sfd, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize) != 0)
  {
    fprintf(stderr, "getsockopt(SO_SNDBUF)\n");
    return;
  }

  /* Binary-search for the real maximum. */
  min= old_size;
  max= MAX_SENDBUF_SIZE;

  while (min <= max)
  {
    avg= ((unsigned int)(min + max)) / 2;
    if (setsockopt(sfd, SOL_SOCKET, SO_SNDBUF, (void *)&avg, intsize) == 0)
    {
      last_good= avg;
      min= avg + 1;
    }
    else
    {
      max= avg - 1;
    }
  }
} /* ms_maximize_sndbuf */


/**
 * socket connects the server
 *
 * @param c, pointer of the concurrency
 * @param srv_host_name, the host name of the server
 * @param srv_port, port of server
 * @param is_udp, whether it's udp
 * @param ret_sfd, the connected socket file descriptor
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_network_connect(ms_conn_t *c,
                              char *srv_host_name,
                              const int srv_port,
                              const bool is_udp,
                              int *ret_sfd)
{
  int sfd;
  struct linger ling=
  {
    0, 0
  };
  struct addrinfo *ai;
  struct addrinfo *next;
  struct addrinfo hints;
  char port_buf[NI_MAXSERV];
  int  error;
  int  success= 0;

  int flags= 1;

  /*
   * the memset call clears nonstandard fields in some impementations
   * that otherwise mess things up.
   */
  memset(&hints, 0, sizeof(hints));
#ifdef AI_ADDRCONFIG
  hints.ai_flags= AI_PASSIVE | AI_ADDRCONFIG;
#else
  hints.ai_flags= AI_PASSIVE;
#endif /* AI_ADDRCONFIG */
  if (is_udp)
  {
    hints.ai_protocol= IPPROTO_UDP;
    hints.ai_socktype= SOCK_DGRAM;
    hints.ai_family= AF_INET;      /* This left here because of issues with OSX 10.5 */
  }
  else
  {
    hints.ai_family= AF_UNSPEC;
    hints.ai_protocol= IPPROTO_TCP;
    hints.ai_socktype= SOCK_STREAM;
  }

  snprintf(port_buf, NI_MAXSERV, "%d", srv_port);
  error= getaddrinfo(srv_host_name, port_buf, &hints, &ai);
  if (error != 0)
  {
    if (error != EAI_SYSTEM)
      fprintf(stderr, "getaddrinfo(): %s.\n", gai_strerror(error));
    else
      perror("getaddrinfo()\n");

    return -1;
  }

  for (next= ai; next; next= next->ai_next)
  {
    if ((sfd= ms_new_socket(next)) == -1)
    {
      freeaddrinfo(ai);
      return -1;
    }

    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));
    if (is_udp)
    {
      ms_maximize_sndbuf(sfd);
    }
    else
    {
      setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags,
                 sizeof(flags));
      setsockopt(sfd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
      setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags,
                 sizeof(flags));
    }

    if (is_udp)
    {
      c->srv_recv_addr_size= sizeof(struct sockaddr);
      memcpy(&c->srv_recv_addr, next->ai_addr, c->srv_recv_addr_size);
    }
    else
    {
      if (connect(sfd, next->ai_addr, next->ai_addrlen) == -1)
      {
        close(sfd);
        freeaddrinfo(ai);
        return -1;
      }
    }

    if (((flags= fcntl(sfd, F_GETFL, 0)) < 0)
        || (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0))
    {
      fprintf(stderr, "setting O_NONBLOCK\n");
      close(sfd);
      freeaddrinfo(ai);
      return -1;
    }

    if (ret_sfd != NULL)
    {
      *ret_sfd= sfd;
    }

    success++;
  }

  freeaddrinfo(ai);

  /* Return zero if we detected no errors in starting up connections */
  return success == 0;
} /* ms_network_connect */


/**
 * reconnect a disconnected sock
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_reconn(ms_conn_t *c)
{
  ms_thread_t *ms_thread= pthread_getspecific(ms_thread_key);
  uint32_t srv_idx= 0;
  uint32_t srv_conn_cnt= 0;

  if (ms_setting.rep_write_srv > 0)
  {
    srv_idx= c->cur_idx % ms_setting.srv_cnt;
    srv_conn_cnt= ms_setting.sock_per_conn  * ms_setting.nconns;
  }
  else
  {
    srv_idx= ms_thread->thread_ctx->srv_idx;
    srv_conn_cnt= ms_setting.nconns / ms_setting.srv_cnt;
  }

  /* close the old socket handler */
  close(c->sfd);
  c->tcpsfd[c->cur_idx]= 0;

  if (atomic_add_32_nv(&ms_setting.servers[srv_idx].disconn_cnt, 1)
      % srv_conn_cnt == 0)
  {
    gettimeofday(&ms_setting.servers[srv_idx].disconn_time, NULL);
    fprintf(stderr, "Server %s:%d disconnect\n",
            ms_setting.servers[srv_idx].srv_host_name,
            ms_setting.servers[srv_idx].srv_port);
  }

  if (ms_setting.rep_write_srv > 0)
  {
    uint32_t i= 0;

    for (i= 0; i < c->total_sfds; i++)
    {
      if (c->tcpsfd[i] != 0)
      {
        break;
      }
    }

    /* all socks disconnect */
    if (i == c->total_sfds)
    {
      return -1;
    }
  }
  else
  {
    do
    {
      /* reconnect success, break the loop */
      if (ms_network_connect(c, ms_setting.servers[srv_idx].srv_host_name,
                             ms_setting.servers[srv_idx].srv_port,
                             ms_setting.udp, &c->sfd) == 0)
      {
        c->tcpsfd[c->cur_idx]= c->sfd;
        if (atomic_add_32_nv(&ms_setting.servers[srv_idx].reconn_cnt, 1)
            % (uint32_t)srv_conn_cnt == 0)
        {
          gettimeofday(&ms_setting.servers[srv_idx].reconn_time, NULL);
          int reconn_time=
            (int)(ms_setting.servers[srv_idx].reconn_time.tv_sec
                  - ms_setting.servers[srv_idx].disconn_time
                     .tv_sec);
          fprintf(stderr, "Server %s:%d reconnect after %ds\n",
                  ms_setting.servers[srv_idx].srv_host_name,
                  ms_setting.servers[srv_idx].srv_port, reconn_time);
        }
        break;
      }

      if (ms_setting.rep_write_srv == 0 && c->total_sfds > 0)
      {
        /* wait a second and reconnect */
        sleep(1);
      }
    }
    while (ms_setting.rep_write_srv == 0 && c->total_sfds > 0);
  }

  if ((c->total_sfds > 1) && (c->tcpsfd[c->cur_idx] == 0))
  {
    c->sfd= 0;
    c->alive_sfds--;
  }

  return 0;
} /* ms_reconn */


/**
 *  reconnect several disconnected socks in the connection
 *  structure, the ever-1-second timer of the thread will check
 *  whether some socks in the connections disconnect. if
 *  disconnect, reconnect the sock.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
int ms_reconn_socks(ms_conn_t *c)
{
  ms_thread_t *ms_thread= pthread_getspecific(ms_thread_key);
  uint32_t srv_idx= 0;
  int ret_sfd= 0;
  uint32_t srv_conn_cnt= 0;
  struct timeval cur_time;

  assert(c != NULL);

  if ((c->total_sfds == 1) || (c->total_sfds == c->alive_sfds))
  {
    return 0;
  }

  for (uint32_t i= 0; i < c->total_sfds; i++)
  {
    if (c->tcpsfd[i] == 0)
    {
      gettimeofday(&cur_time, NULL);

      /**
       *  For failover test of replication, reconnect the socks after
       *  it disconnects more than 5 seconds, Otherwise memslap will
       *  block at connect() function and the work threads can't work
       *  in this interval.
       */
      if (cur_time.tv_sec
          - ms_setting.servers[srv_idx].disconn_time.tv_sec < 5)
      {
        break;
      }

      if (ms_setting.rep_write_srv > 0)
      {
        srv_idx= i % ms_setting.srv_cnt;
        srv_conn_cnt= ms_setting.sock_per_conn * ms_setting.nconns;
      }
      else
      {
        srv_idx= ms_thread->thread_ctx->srv_idx;
        srv_conn_cnt= ms_setting.nconns / ms_setting.srv_cnt;
      }

      if (ms_network_connect(c, ms_setting.servers[srv_idx].srv_host_name,
                             ms_setting.servers[srv_idx].srv_port,
                             ms_setting.udp, &ret_sfd) == 0)
      {
        c->tcpsfd[i]= ret_sfd;
        c->alive_sfds++;

        if (atomic_add_32_nv(&ms_setting.servers[srv_idx].reconn_cnt, 1)
            % (uint32_t)srv_conn_cnt == 0)
        {
          gettimeofday(&ms_setting.servers[srv_idx].reconn_time, NULL);
          int reconn_time=
            (int)(ms_setting.servers[srv_idx].reconn_time.tv_sec
                  - ms_setting.servers[srv_idx].disconn_time
                     .tv_sec);
          fprintf(stderr, "Server %s:%d reconnect after %ds\n",
                  ms_setting.servers[srv_idx].srv_host_name,
                  ms_setting.servers[srv_idx].srv_port, reconn_time);
        }
      }
    }
  }

  return 0;
} /* ms_reconn_socks */


/**
 * Tokenize the command string by replacing whitespace with '\0' and update
 * the token array tokens with pointer to start of each token and length.
 * Returns total number of tokens.  The last valid token is the terminal
 * token (value points to the first unprocessed character of the string and
 * length zero).
 *
 * Usage example:
 *
 *  while(ms_tokenize_command(command, ncommand, tokens, max_tokens) > 0) {
 *      for(int ix = 0; tokens[ix].length != 0; ix++) {
 *          ...
 *      }
 *      ncommand = tokens[ix].value - command;
 *      command  = tokens[ix].value;
 *   }
 *
 * @param command, the command string to token
 * @param tokens, array to store tokens
 * @param max_tokens, maximum tokens number
 *
 * @return int, the number of tokens
 */
static int ms_tokenize_command(char *command,
                               token_t *tokens,
                               const int max_tokens)
{
  char *s, *e;
  int  ntokens= 0;

  assert(command != NULL && tokens != NULL && max_tokens > 1);

  for (s= e= command; ntokens < max_tokens - 1; ++e)
  {
    if (*e == ' ')
    {
      if (s != e)
      {
        tokens[ntokens].value= s;
        tokens[ntokens].length= (size_t)(e - s);
        ntokens++;
        *e= '\0';
      }
      s= e + 1;
    }
    else if (*e == '\0')
    {
      if (s != e)
      {
        tokens[ntokens].value= s;
        tokens[ntokens].length= (size_t)(e - s);
        ntokens++;
      }

      break;       /* string end */
    }
  }

  return ntokens;
} /* ms_tokenize_command */


/**
 * parse the response of server.
 *
 * @param c, pointer of the concurrency
 * @param command, the string responded by server
 *
 * @return int, if the command completed return 0, else return
 *         -1
 */
static int ms_ascii_process_line(ms_conn_t *c, char *command)
{
  int ret= 0;
  int64_t value_len;
  char *buffer= command;

  assert(c != NULL);

  /**
   * for command get, we store the returned value into local buffer
   * then continue in ms_complete_nread().
   */

  switch (buffer[0])
  {
  case 'V':                     /* VALUE || VERSION */
    if (buffer[1] == 'A')       /* VALUE */
    {
      token_t tokens[MAX_TOKENS];
      ms_tokenize_command(command, tokens, MAX_TOKENS);
      value_len= strtol(tokens[VALUELEN_TOKEN].value, NULL, 10);
      c->currcmd.key_prefix= *(uint64_t *)tokens[KEY_TOKEN].value;

      /*
       *  We read the \r\n into the string since not doing so is more
       *  cycles then the waster of memory to do so.
       *
       *  We are null terminating through, which will most likely make
       *  some people lazy about using the return length.
       */
      c->rvbytes= (int)(value_len + 2);
      c->readval= true;
      ret= -1;
    }

    break;

  case 'O':   /* OK */
    c->currcmd.retstat= MCD_SUCCESS;

  case 'S':                    /* STORED STATS SERVER_ERROR */
    if (buffer[2] == 'A')      /* STORED STATS */
    {       /* STATS*/
      c->currcmd.retstat= MCD_STAT;
    }
    else if (buffer[1] == 'E')
    {
      /* SERVER_ERROR */
      printf("<%d %s\n", c->sfd, buffer);

      c->currcmd.retstat= MCD_SERVER_ERROR;
    }
    else if (buffer[1] == 'T')
    {
      /* STORED */
      c->currcmd.retstat= MCD_STORED;
    }
    else
    {
      c->currcmd.retstat= MCD_UNKNOWN_READ_FAILURE;
    }
    break;

  case 'D':   /* DELETED DATA */
    if (buffer[1] == 'E')
    {
      c->currcmd.retstat= MCD_DELETED;
    }
    else
    {
      c->currcmd.retstat= MCD_UNKNOWN_READ_FAILURE;
    }

    break;

  case 'N':   /* NOT_FOUND NOT_STORED*/
    if (buffer[4] == 'F')
    {
      c->currcmd.retstat= MCD_NOTFOUND;
    }
    else if (buffer[4] == 'S')
    {
      printf("<%d %s\n", c->sfd, buffer);
      c->currcmd.retstat= MCD_NOTSTORED;
    }
    else
    {
      c->currcmd.retstat= MCD_UNKNOWN_READ_FAILURE;
    }
    break;

  case 'E':   /* PROTOCOL ERROR or END */
    if (buffer[1] == 'N')
    {
      /* END */
      c->currcmd.retstat= MCD_END;
    }
    else if (buffer[1] == 'R')
    {
      printf("<%d ERROR\n", c->sfd);
      c->currcmd.retstat= MCD_PROTOCOL_ERROR;
    }
    else if (buffer[1] == 'X')
    {
      c->currcmd.retstat= MCD_DATA_EXISTS;
      printf("<%d %s\n", c->sfd, buffer);
    }
    else
    {
      c->currcmd.retstat= MCD_UNKNOWN_READ_FAILURE;
    }
    break;

  case 'C':   /* CLIENT ERROR */
    printf("<%d %s\n", c->sfd, buffer);
    c->currcmd.retstat= MCD_CLIENT_ERROR;
    break;

  default:
    c->currcmd.retstat= MCD_UNKNOWN_READ_FAILURE;
    break;
  } /* switch */

  return ret;
} /* ms_ascii_process_line */


/**
 * after one operation completes, reset the concurrency
 *
 * @param c, pointer of the concurrency
 * @param timeout, whether it's timeout
 */
void ms_reset_conn(ms_conn_t *c, bool timeout)
{
  assert(c != NULL);

  if (c->udp)
  {
    if ((c->packets > 0) && (c->packets < MAX_UDP_PACKET))
    {
      memset(c->udppkt, 0, sizeof(ms_udppkt_t) * (size_t)c->packets);
    }

    c->packets= 0;
    c->recvpkt= 0;
    c->pktcurr= 0;
    c->ordcurr= 0;
    c->rudpbytes= 0;
  }
  c->currcmd.isfinish= true;
  c->ctnwrite= false;
  c->rbytes= 0;
  c->rcurr= c->rbuf;
  c->msgcurr = 0;
  c->msgused = 0;
  c->iovused = 0;
  ms_conn_set_state(c, conn_write);
  memcpy(&c->precmd, &c->currcmd, sizeof(ms_cmdstat_t));    /* replicate command state */

  if (timeout)
  {
    ms_drive_machine(c);
  }
} /* ms_reset_conn */


/**
 * if we have a complete line in the buffer, process it.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_try_read_line(ms_conn_t *c)
{
  if (c->protocol == binary_prot)
  {
    /* Do we have the complete packet header? */
    if ((uint64_t)c->rbytes < sizeof(c->binary_header))
    {
      /* need more data! */
      return 0;
    }
    else
    {
#ifdef NEED_ALIGN
      if (((long)(c->rcurr)) % 8 != 0)
      {
        /* must realign input buffer */
        memmove(c->rbuf, c->rcurr, c->rbytes);
        c->rcurr= c->rbuf;
        if (settings.verbose)
        {
          fprintf(stderr, "%d: Realign input buffer.\n", c->sfd);
        }
      }
#endif
      protocol_binary_response_header *rsp;
      rsp= (protocol_binary_response_header *)c->rcurr;

      c->binary_header= *rsp;
      c->binary_header.response.extlen= rsp->response.extlen;
      c->binary_header.response.keylen= ntohs(rsp->response.keylen);
      c->binary_header.response.bodylen= ntohl(rsp->response.bodylen);
      c->binary_header.response.status= ntohs(rsp->response.status);

      if (c->binary_header.response.magic != PROTOCOL_BINARY_RES)
      {
        fprintf(stderr, "Invalid magic:  %x\n",
                c->binary_header.response.magic);
        ms_conn_set_state(c, conn_closing);
        return 0;
      }

      /* process this complete response */
      if (ms_bin_process_response(c) == 0)
      {
        /* current operation completed */
        ms_reset_conn(c, false);
        return -1;
      }
      else
      {
        c->rbytes-= (int32_t)sizeof(c->binary_header);
        c->rcurr+= sizeof(c->binary_header);
      }
    }
  }
  else
  {
    char *el, *cont;

    assert(c != NULL);
    assert(c->rcurr <= (c->rbuf + c->rsize));

    if (c->rbytes == 0)
      return 0;

    el= memchr(c->rcurr, '\n', (size_t)c->rbytes);
    if (! el)
      return 0;

    cont= el + 1;
    if (((el - c->rcurr) > 1) && (*(el - 1) == '\r'))
    {
      el--;
    }
    *el= '\0';

    assert(cont <= (c->rcurr + c->rbytes));

    /* process this complete line */
    if (ms_ascii_process_line(c, c->rcurr) == 0)
    {
      /* current operation completed */
      ms_reset_conn(c, false);
      return -1;
    }
    else
    {
      /* current operation didn't complete */
      c->rbytes-= (int32_t)(cont - c->rcurr);
      c->rcurr= cont;
    }

    assert(c->rcurr <= (c->rbuf + c->rsize));
  }

  return -1;
} /* ms_try_read_line */


/**
 *  because the packet of UDP can't ensure the order, the
 *  function is used to sort the received udp packet.
 *
 * @param c, pointer of the concurrency
 * @param buf, the buffer to store the ordered packages data
 * @param rbytes, the maximum capacity of the buffer
 *
 * @return int, if success, return the copy bytes, else return
 *         -1
 */
static int ms_sort_udp_packet(ms_conn_t *c, char *buf, int rbytes)
{
  int len= 0;
  int wbytes= 0;
  uint16_t req_id= 0;
  uint16_t seq_num= 0;
  uint16_t packets= 0;
  unsigned char *header= NULL;

  /* no enough data */
  assert(c != NULL);
  assert(buf != NULL);
  assert(c->rudpbytes >= UDP_HEADER_SIZE);

  /* calculate received packets count */
  if (c->rudpbytes % UDP_MAX_PAYLOAD_SIZE >= UDP_HEADER_SIZE)
  {
    /* the last packet has some data */
    c->recvpkt= c->rudpbytes / UDP_MAX_PAYLOAD_SIZE + 1;
  }
  else
  {
    c->recvpkt= c->rudpbytes / UDP_MAX_PAYLOAD_SIZE;
  }

  /* get the total packets count if necessary */
  if (c->packets == 0)
  {
    c->packets= HEADER_TO_PACKETS((unsigned char *)c->rudpbuf);
  }

  /* build the ordered packet array */
  for (int i= c->pktcurr; i < c->recvpkt; i++)
  {
    header= (unsigned char *)c->rudpbuf + i * UDP_MAX_PAYLOAD_SIZE;
    req_id= (uint16_t)HEADER_TO_REQID(header);
    assert(req_id == c->request_id % (1 << 16));

    packets= (uint16_t)HEADER_TO_PACKETS(header);
    assert(c->packets == HEADER_TO_PACKETS(header));

    seq_num= (uint16_t)HEADER_TO_SEQNUM(header);
    c->udppkt[seq_num].header= header;
    c->udppkt[seq_num].data= (char *)header + UDP_HEADER_SIZE;

    if (i == c->recvpkt - 1)
    {
      /* last received packet */
      if (c->rudpbytes % UDP_MAX_PAYLOAD_SIZE == 0)
      {
        c->udppkt[seq_num].rbytes= UDP_MAX_PAYLOAD_SIZE - UDP_HEADER_SIZE;
        c->pktcurr++;
      }
      else
      {
        c->udppkt[seq_num].rbytes= c->rudpbytes % UDP_MAX_PAYLOAD_SIZE
                                   - UDP_HEADER_SIZE;
      }
    }
    else
    {
      c->udppkt[seq_num].rbytes= UDP_MAX_PAYLOAD_SIZE - UDP_HEADER_SIZE;
      c->pktcurr++;
    }
  }

  for (int i= c->ordcurr; i < c->recvpkt; i++)
  {
    /* there is some data to copy */
    if ((c->udppkt[i].data != NULL)
        && (c->udppkt[i].copybytes < c->udppkt[i].rbytes))
    {
      header= c->udppkt[i].header;
      len= c->udppkt[i].rbytes - c->udppkt[i].copybytes;
      if (len > rbytes - wbytes)
      {
        len= rbytes - wbytes;
      }

      assert(len <= rbytes - wbytes);
      assert(i == HEADER_TO_SEQNUM(header));

      memcpy(buf + wbytes, c->udppkt[i].data + c->udppkt[i].copybytes,
             (size_t)len);
      wbytes+= len;
      c->udppkt[i].copybytes+= len;

      if ((c->udppkt[i].copybytes == c->udppkt[i].rbytes)
          && (c->udppkt[i].rbytes == UDP_MAX_PAYLOAD_SIZE - UDP_HEADER_SIZE))
      {
        /* finish copying all the data of this packet, next */
        c->ordcurr++;
      }

      /* last received packet, and finish copying all the data */
      if ((c->recvpkt == c->packets) && (i == c->recvpkt - 1)
          && (c->udppkt[i].copybytes == c->udppkt[i].rbytes))
      {
        break;
      }

      /* no space to copy data */
      if (wbytes >= rbytes)
      {
        break;
      }

      /* it doesn't finish reading all the data of the packet from network */
      if ((i != c->recvpkt - 1)
          && (c->udppkt[i].rbytes < UDP_MAX_PAYLOAD_SIZE - UDP_HEADER_SIZE))
      {
        break;
      }
    }
    else
    {
      /* no data to copy */
      break;
    }
  }

  return wbytes == 0 ? -1 : wbytes;
} /* ms_sort_udp_packet */


/**
 * encapsulate upd read like tcp read
 *
 * @param c, pointer of the concurrency
 * @param buf, read buffer
 * @param len, length to read
 *
 * @return int, if success, return the read bytes, else return
 *         -1
 */
static int ms_udp_read(ms_conn_t *c, char *buf, int len)
{
  int res= 0;
  int avail= 0;
  int rbytes= 0;
  int copybytes= 0;

  assert(c->udp);

  while (1)
  {
    if (c->rudpbytes + UDP_MAX_PAYLOAD_SIZE > c->rudpsize)
    {
      char *new_rbuf= realloc(c->rudpbuf, (size_t)c->rudpsize * 2);
      if (! new_rbuf)
      {
        fprintf(stderr, "Couldn't realloc input buffer.\n");
        c->rudpbytes= 0;          /* ignore what we read */
        return -1;
      }
      c->rudpbuf= new_rbuf;
      c->rudpsize*= 2;
    }

    avail= c->rudpsize - c->rudpbytes;
    /* UDP each time read a packet, 1400 bytes */
    res= (int)read(c->sfd, c->rudpbuf + c->rudpbytes, (size_t)avail);

    if (res > 0)
    {
      atomic_add_size(&ms_stats.bytes_read, res);
      c->rudpbytes+= res;
      rbytes+= res;
      if (res == avail)
      {
        continue;
      }
      else
      {
        break;
      }
    }

    if (res == 0)
    {
      /* "connection" closed */
      return res;
    }

    if (res == -1)
    {
      /* no data to read */
      return res;
    }
  }

  /* copy data to read buffer */
  if (rbytes > 0)
  {
    copybytes= ms_sort_udp_packet(c, buf, len);
  }

  if (copybytes == -1)
  {
    atomic_add_size(&ms_stats.pkt_disorder, 1);
  }

  return copybytes;
} /* ms_udp_read */


/*
 * read from network as much as we can, handle buffer overflow and connection
 * close.
 * before reading, move the remaining incomplete fragment of a command
 * (if any) to the beginning of the buffer.
 * return 0 if there's nothing to read on the first read.
 */

/**
 * read from network as much as we can, handle buffer overflow and connection
 * close. before reading, move the remaining incomplete fragment of a command
 * (if any) to the beginning of the buffer.
 *
 * @param c, pointer of the concurrency
 *
 * @return int,
 *         return 0 if there's nothing to read on the first read.
 *         return 1 if get data
 *         return -1 if error happens
 */
static int ms_try_read_network(ms_conn_t *c)
{
  int gotdata= 0;
  int res;
  int64_t avail;

  assert(c != NULL);

  if ((c->rcurr != c->rbuf)
      && (! c->readval || (c->rvbytes > c->rsize - (c->rcurr - c->rbuf))
          || (c->readval && (c->rcurr - c->rbuf > c->rbytes))))
  {
    if (c->rbytes != 0)     /* otherwise there's nothing to copy */
      memmove(c->rbuf, c->rcurr, (size_t)c->rbytes);
    c->rcurr= c->rbuf;
  }

  while (1)
  {
    if (c->rbytes >= c->rsize)
    {
      char *new_rbuf= realloc(c->rbuf, (size_t)c->rsize * 2);
      if (! new_rbuf)
      {
        fprintf(stderr, "Couldn't realloc input buffer.\n");
        c->rbytes= 0;          /* ignore what we read */
        return -1;
      }
      c->rcurr= c->rbuf= new_rbuf;
      c->rsize*= 2;
    }

    avail= c->rsize - c->rbytes - (c->rcurr - c->rbuf);
    if (avail == 0)
    {
      break;
    }

    if (c->udp)
    {
      res= (int32_t)ms_udp_read(c, c->rcurr + c->rbytes, (int32_t)avail);
    }
    else
    {
      res= (int)read(c->sfd, c->rcurr + c->rbytes, (size_t)avail);
    }

    if (res > 0)
    {
      if (! c->udp)
      {
        atomic_add_size(&ms_stats.bytes_read, res);
      }
      gotdata= 1;
      c->rbytes+= res;
      if (res == avail)
      {
        continue;
      }
      else
      {
        break;
      }
    }
    if (res == 0)
    {
      /* connection closed */
      ms_conn_set_state(c, conn_closing);
      return -1;
    }
    if (res == -1)
    {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        break;
      /* Should close on unhandled errors. */
      ms_conn_set_state(c, conn_closing);
      return -1;
    }
  }

  return gotdata;
} /* ms_try_read_network */


/**
 * after get the object from server, verify the value if
 * necessary.
 *
 * @param c, pointer of the concurrency
 * @param mlget_item, pointer of mulit-get task item structure
 * @param value, received value string
 * @param vlen, received value string length
 */
static void ms_verify_value(ms_conn_t *c,
                            ms_mlget_task_item_t *mlget_item,
                            char *value,
                            int vlen)
{
  if (c->curr_task.verify)
  {
    assert(c->curr_task.item->value_offset != INVALID_OFFSET);
    char *orignval= &ms_setting.char_block[c->curr_task.item->value_offset];
    char *orignkey=
      &ms_setting.char_block[c->curr_task.item->key_suffix_offset];

    /* verify expire time if necessary */
    if (c->curr_task.item->exp_time > 0)
    {
      struct timeval curr_time;
      gettimeofday(&curr_time, NULL);

      /* object expired but get it now */
      if (curr_time.tv_sec - c->curr_task.item->client_time
          > c->curr_task.item->exp_time + EXPIRE_TIME_ERROR)
      {
        atomic_add_size(&ms_stats.exp_get, 1);

        if (ms_setting.verbose)
        {
          char set_time[64];
          char cur_time[64];
          strftime(set_time, 64, "%Y-%m-%d %H:%M:%S",
                   localtime(&c->curr_task.item->client_time));
          strftime(cur_time, 64, "%Y-%m-%d %H:%M:%S",
                   localtime(&curr_time.tv_sec));
          fprintf(stderr,
                  "\n<%d expire time verification failed, "
                  "object expired but get it now\n"
                  "\tkey len: %d\n"
                  "\tkey: %" PRIx64 " %.*s\n"
                  "\tset time: %s current time: %s "
                  "diff time: %d expire time: %d\n"
                  "\texpected data: \n"
                  "\treceived data len: %d\n"
                  "\treceived data: %.*s\n",
                  c->sfd,
                  c->curr_task.item->key_size,
                  c->curr_task.item->key_prefix,
                  c->curr_task.item->key_size - (int)KEY_PREFIX_SIZE,
                  orignkey,
                  set_time,
                  cur_time,
                  (int)(curr_time.tv_sec - c->curr_task.item->client_time),
                  c->curr_task.item->exp_time,
                  vlen,
                  vlen,
                  value);
          fflush(stderr);
        }
      }
    }
    else
    {
      if ((c->curr_task.item->value_size != vlen)
          || (memcmp(orignval, value, (size_t)vlen) != 0))
      {
        atomic_add_size(&ms_stats.vef_failed, 1);

        if (ms_setting.verbose)
        {
          fprintf(stderr,
                  "\n<%d data verification failed\n"
                  "\tkey len: %d\n"
                  "\tkey: %" PRIx64" %.*s\n"
                  "\texpected data len: %d\n"
                  "\texpected data: %.*s\n"
                  "\treceived data len: %d\n"
                  "\treceived data: %.*s\n",
                  c->sfd,
                  c->curr_task.item->key_size,
                  c->curr_task.item->key_prefix,
                  c->curr_task.item->key_size - (int)KEY_PREFIX_SIZE,
                  orignkey,
                  c->curr_task.item->value_size,
                  c->curr_task.item->value_size,
                  orignval,
                  vlen,
                  vlen,
                  value);
          fflush(stderr);
        }
      }
    }

    c->curr_task.finish_verify= true;

    if (mlget_item != NULL)
    {
      mlget_item->finish_verify= true;
    }
  }
} /* ms_verify_value */


/**
 * For ASCII protocol, after store the data into the local
 * buffer, run this function to handle the data.
 *
 * @param c, pointer of the concurrency
 */
static void ms_ascii_complete_nread(ms_conn_t *c)
{
  assert(c != NULL);
  assert(c->rbytes >= c->rvbytes);
  assert(c->protocol == ascii_prot);
  if (c->rvbytes > 2)
  {
    assert(
      c->rcurr[c->rvbytes - 1] == '\n' && c->rcurr[c->rvbytes - 2] == '\r');
  }

  /* multi-get */
  ms_mlget_task_item_t *mlget_item= NULL;
  if (((ms_setting.mult_key_num > 1)
       && (c->mlget_task.mlget_num >= ms_setting.mult_key_num))
      || ((c->remain_exec_num == 0) && (c->mlget_task.mlget_num > 0)))
  {
    c->mlget_task.value_index++;
    mlget_item= &c->mlget_task.mlget_item[c->mlget_task.value_index];

    if (mlget_item->item->key_prefix == c->currcmd.key_prefix)
    {
      c->curr_task.item= mlget_item->item;
      c->curr_task.verify= mlget_item->verify;
      c->curr_task.finish_verify= mlget_item->finish_verify;
      mlget_item->get_miss= false;
    }
    else
    {
      /* Try to find the task item in multi-get task array */
      for (int i= 0; i < c->mlget_task.mlget_num; i++)
      {
        mlget_item= &c->mlget_task.mlget_item[i];
        if (mlget_item->item->key_prefix == c->currcmd.key_prefix)
        {
          c->curr_task.item= mlget_item->item;
          c->curr_task.verify= mlget_item->verify;
          c->curr_task.finish_verify= mlget_item->finish_verify;
          mlget_item->get_miss= false;

          break;
        }
      }
    }
  }

  ms_verify_value(c, mlget_item, c->rcurr, c->rvbytes - 2);

  c->curr_task.get_miss= false;
  c->rbytes-= c->rvbytes;
  c->rcurr= c->rcurr + c->rvbytes;
  assert(c->rcurr <= (c->rbuf + c->rsize));
  c->readval= false;
  c->rvbytes= 0;
} /* ms_ascii_complete_nread */


/**
 * For binary protocol, after store the data into the local
 * buffer, run this function to handle the data.
 *
 * @param c, pointer of the concurrency
 */
static void ms_bin_complete_nread(ms_conn_t *c)
{
  assert(c != NULL);
  assert(c->rbytes >= c->rvbytes);
  assert(c->protocol == binary_prot);

  int extlen= c->binary_header.response.extlen;
  int keylen= c->binary_header.response.keylen;
  uint8_t opcode= c->binary_header.response.opcode;

  /* not get command or not include value, just return */
  if (((opcode != PROTOCOL_BINARY_CMD_GET)
       && (opcode != PROTOCOL_BINARY_CMD_GETQ))
      || (c->rvbytes <= extlen + keylen))
  {
    /* get miss */
    if (c->binary_header.response.opcode == PROTOCOL_BINARY_CMD_GET)
    {
      c->currcmd.retstat= MCD_END;
      c->curr_task.get_miss= true;
    }

    c->readval= false;
    c->rvbytes= 0;
    ms_reset_conn(c, false);
    return;
  }

  /* multi-get */
  ms_mlget_task_item_t *mlget_item= NULL;
  if (((ms_setting.mult_key_num > 1)
       && (c->mlget_task.mlget_num >= ms_setting.mult_key_num))
      || ((c->remain_exec_num == 0) && (c->mlget_task.mlget_num > 0)))
  {
    c->mlget_task.value_index++;
    mlget_item= &c->mlget_task.mlget_item[c->mlget_task.value_index];

    c->curr_task.item= mlget_item->item;
    c->curr_task.verify= mlget_item->verify;
    c->curr_task.finish_verify= mlget_item->finish_verify;
    mlget_item->get_miss= false;
  }

  ms_verify_value(c,
                  mlget_item,
                  c->rcurr + extlen + keylen,
                  c->rvbytes - extlen - keylen);

  c->currcmd.retstat= MCD_END;
  c->curr_task.get_miss= false;
  c->rbytes-= c->rvbytes;
  c->rcurr= c->rcurr + c->rvbytes;
  assert(c->rcurr <= (c->rbuf + c->rsize));
  c->readval= false;
  c->rvbytes= 0;

  if (ms_setting.mult_key_num > 1)
  {
    /* multi-get have check all the item */
    if (c->mlget_task.value_index == c->mlget_task.mlget_num - 1)
    {
      ms_reset_conn(c, false);
    }
  }
  else
  {
    /* single get */
    ms_reset_conn(c, false);
  }
} /* ms_bin_complete_nread */


/**
 * we get here after reading the value of get commands.
 *
 * @param c, pointer of the concurrency
 */
static void ms_complete_nread(ms_conn_t *c)
{
  assert(c != NULL);
  assert(c->rbytes >= c->rvbytes);
  assert(c->protocol == ascii_prot
         || c->protocol == binary_prot);

  if (c->protocol == binary_prot)
  {
    ms_bin_complete_nread(c);
  }
  else
  {
    ms_ascii_complete_nread(c);
  }
} /* ms_complete_nread */


/**
 * Adds a message header to a connection.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_add_msghdr(ms_conn_t *c)
{
  struct msghdr *msg;

  assert(c != NULL);

  if (c->msgsize == c->msgused)
  {
    msg=
      realloc(c->msglist, (size_t)c->msgsize * 2 * sizeof(struct msghdr));
    if (! msg)
      return -1;

    c->msglist= msg;
    c->msgsize*= 2;
  }

  msg= c->msglist + c->msgused;

  /**
   *  this wipes msg_iovlen, msg_control, msg_controllen, and
   *  msg_flags, the last 3 of which aren't defined on solaris:
   */
  memset(msg, 0, sizeof(struct msghdr));

  msg->msg_iov= &c->iov[c->iovused];

  if (c->udp && (c->srv_recv_addr_size > 0))
  {
    msg->msg_name= &c->srv_recv_addr;
    msg->msg_namelen= c->srv_recv_addr_size;
  }

  c->msgbytes= 0;
  c->msgused++;

  if (c->udp)
  {
    /* Leave room for the UDP header, which we'll fill in later. */
    return ms_add_iov(c, NULL, UDP_HEADER_SIZE);
  }

  return 0;
} /* ms_add_msghdr */


/**
 * Ensures that there is room for another structure iovec in a connection's
 * iov list.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_ensure_iov_space(ms_conn_t *c)
{
  assert(c != NULL);

  if (c->iovused >= c->iovsize)
  {
    int i, iovnum;
    struct iovec *new_iov= (struct iovec *)realloc(c->iov,
                                                   ((size_t)c->iovsize
                                                    * 2)
                                                   * sizeof(struct iovec));
    if (! new_iov)
      return -1;

    c->iov= new_iov;
    c->iovsize*= 2;

    /* Point all the msghdr structures at the new list. */
    for (i= 0, iovnum= 0; i < c->msgused; i++)
    {
      c->msglist[i].msg_iov= &c->iov[iovnum];
      iovnum+= (int)c->msglist[i].msg_iovlen;
    }
  }

  return 0;
} /* ms_ensure_iov_space */


/**
 * Adds data to the list of pending data that will be written out to a
 * connection.
 *
 * @param c, pointer of the concurrency
 * @param buf, the buffer includes data to send
 * @param len, the data length in the buffer
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_add_iov(ms_conn_t *c, const void *buf, int len)
{
  struct msghdr *m;
  int  leftover;
  bool limit_to_mtu;

  assert(c != NULL);

  do
  {
    m= &c->msglist[c->msgused - 1];

    /*
     * Limit UDP packets, to UDP_MAX_PAYLOAD_SIZE bytes.
     */
    limit_to_mtu= c->udp;

    /* We may need to start a new msghdr if this one is full. */
    if ((m->msg_iovlen == IOV_MAX)
        || (limit_to_mtu && (c->msgbytes >= UDP_MAX_SEND_PAYLOAD_SIZE)))
    {
      ms_add_msghdr(c);
      m= &c->msglist[c->msgused - 1];
    }

    if (ms_ensure_iov_space(c) != 0)
      return -1;

    /* If the fragment is too big to fit in the datagram, split it up */
    if (limit_to_mtu && (len + c->msgbytes > UDP_MAX_SEND_PAYLOAD_SIZE))
    {
      leftover= len + c->msgbytes - UDP_MAX_SEND_PAYLOAD_SIZE;
      len-= leftover;
    }
    else
    {
      leftover= 0;
    }

    m= &c->msglist[c->msgused - 1];
    m->msg_iov[m->msg_iovlen].iov_base= (void *)buf;
    m->msg_iov[m->msg_iovlen].iov_len= (size_t)len;

    c->msgbytes+= len;
    c->iovused++;
    m->msg_iovlen++;

    buf= ((char *)buf) + len;
    len= leftover;
  }
  while (leftover > 0);

  return 0;
} /* ms_add_iov */


/**
 * Constructs a set of UDP headers and attaches them to the outgoing messages.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_build_udp_headers(ms_conn_t *c)
{
  int i;
  unsigned char *hdr;

  assert(c != NULL);

  c->request_id= ms_get_udp_request_id();

  if (c->msgused > c->hdrsize)
  {
    void *new_hdrbuf;
    if (c->hdrbuf)
      new_hdrbuf= realloc(c->hdrbuf,
                          (size_t)c->msgused * 2 * UDP_HEADER_SIZE);
    else
      new_hdrbuf= malloc((size_t)c->msgused * 2 * UDP_HEADER_SIZE);
    if (! new_hdrbuf)
      return -1;

    c->hdrbuf= (unsigned char *)new_hdrbuf;
    c->hdrsize= c->msgused * 2;
  }

  /* If this is a multi-packet request, drop it. */
  if (c->udp && (c->msgused > 1))
  {
    fprintf(stderr, "multi-packet request for UDP not supported.\n");
    return -1;
  }

  hdr= c->hdrbuf;
  for (i= 0; i < c->msgused; i++)
  {
    c->msglist[i].msg_iov[0].iov_base= (void *)hdr;
    c->msglist[i].msg_iov[0].iov_len= UDP_HEADER_SIZE;
    *hdr++= (unsigned char)(c->request_id / 256);
    *hdr++= (unsigned char)(c->request_id % 256);
    *hdr++= (unsigned char)(i / 256);
    *hdr++= (unsigned char)(i % 256);
    *hdr++= (unsigned char)(c->msgused / 256);
    *hdr++= (unsigned char)(c->msgused % 256);
    *hdr++= (unsigned char)1;          /* support facebook memcached */
    *hdr++= (unsigned char)0;
    assert(hdr ==
           ((unsigned char *)c->msglist[i].msg_iov[0].iov_base
            + UDP_HEADER_SIZE));
  }

  return 0;
} /* ms_build_udp_headers */


/**
 * Transmit the next chunk of data from our list of msgbuf structures.
 *
 * @param c, pointer of the concurrency
 *
 * @return  TRANSMIT_COMPLETE   All done writing.
 *          TRANSMIT_INCOMPLETE More data remaining to write.
 *          TRANSMIT_SOFT_ERROR Can't write any more right now.
 *          TRANSMIT_HARD_ERROR Can't write (c->state is set to conn_closing)
 */
static int ms_transmit(ms_conn_t *c)
{
  assert(c != NULL);

  if ((c->msgcurr < c->msgused)
      && (c->msglist[c->msgcurr].msg_iovlen == 0))
  {
    /* Finished writing the current msg; advance to the next. */
    c->msgcurr++;
  }

  if (c->msgcurr < c->msgused)
  {
    ssize_t res;
    struct msghdr *m= &c->msglist[c->msgcurr];

    res= sendmsg(c->sfd, m, 0);
    if (res > 0)
    {
      atomic_add_size(&ms_stats.bytes_written, res);

      /* We've written some of the data. Remove the completed
       *  iovec entries from the list of pending writes. */
      while (m->msg_iovlen > 0 && res >= (ssize_t)m->msg_iov->iov_len)
      {
        res-= (ssize_t)m->msg_iov->iov_len;
        m->msg_iovlen--;
        m->msg_iov++;
      }

      /* Might have written just part of the last iovec entry;
       *  adjust it so the next write will do the rest. */
      if (res > 0)
      {
        m->msg_iov->iov_base= (void *)((unsigned char *)m->msg_iov->iov_base + res);
        m->msg_iov->iov_len-= (size_t)res;
      }
      return TRANSMIT_INCOMPLETE;
    }
    if ((res == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
    {
      if (! ms_update_event(c, EV_WRITE | EV_PERSIST))
      {
        fprintf(stderr, "Couldn't update event.\n");
        ms_conn_set_state(c, conn_closing);
        return TRANSMIT_HARD_ERROR;
      }
      return TRANSMIT_SOFT_ERROR;
    }

    /* if res==0 or res==-1 and error is not EAGAIN or EWOULDBLOCK,
     *  we have a real error, on which we close the connection */
    fprintf(stderr, "Failed to write, and not due to blocking.\n");

    ms_conn_set_state(c, conn_closing);
    return TRANSMIT_HARD_ERROR;
  }
  else
  {
    return TRANSMIT_COMPLETE;
  }
} /* ms_transmit */


/**
 * Shrinks a connection's buffers if they're too big.  This prevents
 * periodic large "mget" response from server chewing lots of client
 * memory.
 *
 * This should only be called in between requests since it can wipe output
 * buffers!
 *
 * @param c, pointer of the concurrency
 */
static void ms_conn_shrink(ms_conn_t *c)
{
  assert(c != NULL);

  if (c->udp)
    return;

  if ((c->rsize > READ_BUFFER_HIGHWAT) && (c->rbytes < DATA_BUFFER_SIZE))
  {
    char *newbuf;

    if (c->rcurr != c->rbuf)
      memmove(c->rbuf, c->rcurr, (size_t)c->rbytes);

    newbuf= (char *)realloc((void *)c->rbuf, DATA_BUFFER_SIZE);

    if (newbuf)
    {
      c->rbuf= newbuf;
      c->rsize= DATA_BUFFER_SIZE;
    }
    c->rcurr= c->rbuf;
  }

  if (c->udp && (c->rudpsize > UDP_DATA_BUFFER_HIGHWAT)
      && (c->rudpbytes + UDP_MAX_PAYLOAD_SIZE < UDP_DATA_BUFFER_SIZE))
  {
    char *new_rbuf= (char *)realloc(c->rudpbuf, (size_t)c->rudpsize * 2);
    if (! new_rbuf)
    {
      c->rudpbuf= new_rbuf;
      c->rudpsize= UDP_DATA_BUFFER_SIZE;
    }
    /* TODO check error condition? */
  }

  if (c->msgsize > MSG_LIST_HIGHWAT)
  {
    struct msghdr *newbuf= (struct msghdr *)realloc(
      (void *)c->msglist,
      MSG_LIST_INITIAL
      * sizeof(c->msglist[0]));
    if (newbuf)
    {
      c->msglist= newbuf;
      c->msgsize= MSG_LIST_INITIAL;
    }
    /* TODO check error condition? */
  }

  if (c->iovsize > IOV_LIST_HIGHWAT)
  {
    struct iovec *newbuf= (struct iovec *)realloc((void *)c->iov,
                                                  IOV_LIST_INITIAL
                                                  * sizeof(c->iov[0]));
    if (newbuf)
    {
      c->iov= newbuf;
      c->iovsize= IOV_LIST_INITIAL;
    }
    /* TODO check return value */
  }
} /* ms_conn_shrink */


/**
 * Sets a connection's current state in the state machine. Any special
 * processing that needs to happen on certain state transitions can
 * happen here.
 *
 * @param c, pointer of the concurrency
 * @param state, connection state
 */
static void ms_conn_set_state(ms_conn_t *c, int state)
{
  assert(c != NULL);

  if (state != c->state)
  {
    if (state == conn_read)
    {
      ms_conn_shrink(c);
    }
    c->state= state;
  }
} /* ms_conn_set_state */


/**
 * update the event if socks change state. for example: when
 * change the listen scoket read event to sock write event, or
 * change socket handler, we could call this function.
 *
 * @param c, pointer of the concurrency
 * @param new_flags, new event flags
 *
 * @return bool, if success, return true, else return false
 */
static bool ms_update_event(ms_conn_t *c, const int new_flags)
{
  assert(c != NULL);

  struct event_base *base= c->event.ev_base;
  if ((c->ev_flags == new_flags) && (ms_setting.rep_write_srv == 0)
      && (! ms_setting.facebook_test || (c->total_sfds == 1)))
  {
    return true;
  }

  if (event_del(&c->event) == -1)
  {
    /* try to delete the event again */
    if (event_del(&c->event) == -1)
    {
      return false;
    }
  }

  event_set(&c->event,
            c->sfd,
            (short)new_flags,
            ms_event_handler,
            (void *)c);
  event_base_set(base, &c->event);
  c->ev_flags= (short)new_flags;

  if (event_add(&c->event, NULL) == -1)
  {
    return false;
  }

  return true;
} /* ms_update_event */


/**
 * If user want to get the expected throughput, we could limit
 * the performance of memslap. we could give up some work and
 * just wait a short time. The function is used to check this
 * case.
 *
 * @param c, pointer of the concurrency
 *
 * @return bool, if success, return true, else return false
 */
static bool ms_need_yield(ms_conn_t *c)
{
  ms_thread_t *ms_thread= pthread_getspecific(ms_thread_key);
  int64_t tps= 0;
  int64_t time_diff= 0;
  struct timeval curr_time;
  ms_task_t *task= &c->curr_task;

  if (ms_setting.expected_tps > 0)
  {
    gettimeofday(&curr_time, NULL);
    time_diff= ms_time_diff(&ms_thread->startup_time, &curr_time);
    tps=
      (int64_t)((task->get_opt
                 + task->set_opt) / ((uint64_t)time_diff / 1000000));

    /* current throughput is greater than expected throughput */
    if (tps > ms_thread->thread_ctx->tps_perconn)
    {
      return true;
    }
  }

  return false;
} /* ms_need_yield */


/**
 * used to update the start time of each operation
 *
 * @param c, pointer of the concurrency
 */
static void ms_update_start_time(ms_conn_t *c)
{
  ms_task_item_t *item= c->curr_task.item;

  if ((ms_setting.stat_freq > 0) || c->udp
      || ((c->currcmd.cmd == CMD_SET) && (item->exp_time > 0)))
  {
    gettimeofday(&c->start_time, NULL);
    if ((c->currcmd.cmd == CMD_SET) && (item->exp_time > 0))
    {
      /* record the current time */
      item->client_time= c->start_time.tv_sec;
    }
  }
} /* ms_update_start_time */


/**
 * run the state machine
 *
 * @param c, pointer of the concurrency
 */
static void ms_drive_machine(ms_conn_t *c)
{
  bool stop= false;

  assert(c != NULL);

  while (! stop)
  {
    switch (c->state)
    {
    case conn_read:
      if (c->readval)
      {
        if (c->rbytes >= c->rvbytes)
        {
          ms_complete_nread(c);
          break;
        }
      }
      else
      {
        if (ms_try_read_line(c) != 0)
        {
          break;
        }
      }

      if (ms_try_read_network(c) != 0)
      {
        break;
      }

      /* doesn't read all the response data, wait event wake up */
      if (! c->currcmd.isfinish)
      {
        if (! ms_update_event(c, EV_READ | EV_PERSIST))
        {
          fprintf(stderr, "Couldn't update event.\n");
          ms_conn_set_state(c, conn_closing);
          break;
        }
        stop= true;
        break;
      }

      /* we have no command line and no data to read from network, next write */
      ms_conn_set_state(c, conn_write);
      memcpy(&c->precmd, &c->currcmd, sizeof(ms_cmdstat_t));        /* replicate command state */

      break;

    case conn_write:
      if (! c->ctnwrite && ms_need_yield(c))
      {
        usleep(10);

        if (! ms_update_event(c, EV_WRITE | EV_PERSIST))
        {
          fprintf(stderr, "Couldn't update event.\n");
          ms_conn_set_state(c, conn_closing);
          break;
        }
        stop= true;
        break;
      }

      if (! c->ctnwrite && (ms_exec_task(c) != 0))
      {
        ms_conn_set_state(c, conn_closing);
        break;
      }

      /* record the start time before starting to send data if necessary */
      if (! c->ctnwrite || (c->change_sfd && c->ctnwrite))
      {
        if (c->change_sfd)
        {
          c->change_sfd= false;
        }
        ms_update_start_time(c);
      }

      /* change sfd if necessary */
      if (c->change_sfd)
      {
        c->ctnwrite= true;
        stop= true;
        break;
      }

      /* execute task until nothing need be written to network */
      if (! c->ctnwrite && (c->msgcurr == c->msgused))
      {
        if (! ms_update_event(c, EV_WRITE | EV_PERSIST))
        {
          fprintf(stderr, "Couldn't update event.\n");
          ms_conn_set_state(c, conn_closing);
          break;
        }
        stop= true;
        break;
      }

      switch (ms_transmit(c))
      {
      case TRANSMIT_COMPLETE:
        /* we have no data to write to network, next wait repose */
        if (! ms_update_event(c, EV_READ | EV_PERSIST))
        {
          fprintf(stderr, "Couldn't update event.\n");
          ms_conn_set_state(c, conn_closing);
          c->ctnwrite= false;
          break;
        }
        ms_conn_set_state(c, conn_read);
        c->ctnwrite= false;
        stop= true;
        break;

      case TRANSMIT_INCOMPLETE:
        c->ctnwrite= true;
        break;                           /* Continue in state machine. */

      case TRANSMIT_HARD_ERROR:
        c->ctnwrite= false;
        break;

      case TRANSMIT_SOFT_ERROR:
        c->ctnwrite= true;
        stop= true;
        break;

      default:
        break;
      } /* switch */

      break;

    case conn_closing:
      /* recovery mode, need reconnect if connection close */
      if (ms_setting.reconnect && (! ms_global.time_out
                                   || ((ms_setting.run_time == 0)
                                       && (c->remain_exec_num > 0))))
      {
        if (ms_reconn(c) != 0)
        {
          ms_conn_close(c);
          stop= true;
          break;
        }

        ms_reset_conn(c, false);

        if (c->total_sfds == 1)
        {
          if (! ms_update_event(c, EV_WRITE | EV_PERSIST))
          {
            fprintf(stderr, "Couldn't update event.\n");
            ms_conn_set_state(c, conn_closing);
            break;
          }
        }

        break;
      }
      else
      {
        ms_conn_close(c);
        stop= true;
        break;
      }

    default:
      assert(0);
    } /* switch */
  }
} /* ms_drive_machine */


/**
 * the event handler of each thread
 *
 * @param fd, the file descriptor of socket
 * @param which, event flag
 * @param arg, argument
 */
void ms_event_handler(const int fd, const short which, void *arg)
{
  ms_conn_t *c= (ms_conn_t *)arg;

  assert(c != NULL);

  c->which= which;

  /* sanity */
  if (fd != c->sfd)
  {
    fprintf(stderr,
            "Catastrophic: event fd: %d doesn't match conn fd: %d\n",
            fd,
            c->sfd);
    ms_conn_close(c);
    exit(1);
  }
  assert(fd == c->sfd);

  ms_drive_machine(c);

  /* wait for next event */
} /* ms_event_handler */


/**
 * get the next socket descriptor index to run for replication
 *
 * @param c, pointer of the concurrency
 * @param cmd, command(get or set )
 *
 * @return int, if success, return the index, else return 0
 */
static uint32_t ms_get_rep_sock_index(ms_conn_t *c, int cmd)
{
  uint32_t sock_index= 0;
  uint32_t i= 0;

  if (c->total_sfds == 1)
  {
    return 0;
  }

  if (ms_setting.rep_write_srv == 0)
  {
    return sock_index;
  }

  do
  {
    if (cmd == CMD_SET)
    {
      for (i= 0; i < ms_setting.rep_write_srv; i++)
      {
        if (c->tcpsfd[i] > 0)
        {
          break;
        }
      }

      if (i == ms_setting.rep_write_srv)
      {
        /* random get one replication server to read */
        sock_index= (uint32_t)random() % c->total_sfds;
      }
      else
      {
        /* random get one replication writing server to write */
        sock_index= (uint32_t)random() % ms_setting.rep_write_srv;
      }
    }
    else if (cmd == CMD_GET)
    {
      /* random get one replication server to read */
      sock_index= (uint32_t)random() % c->total_sfds;
    }
  }
  while (c->tcpsfd[sock_index] == 0);

  return sock_index;
} /* ms_get_rep_sock_index */


/**
 * get the next socket descriptor index to run
 *
 * @param c, pointer of the concurrency
 *
 * @return int, return the index
 */
static uint32_t ms_get_next_sock_index(ms_conn_t *c)
{
  uint32_t sock_index= 0;

  do
  {
    sock_index= (++c->cur_idx == c->total_sfds) ? 0 : c->cur_idx;
  }
  while (c->tcpsfd[sock_index] == 0);

  return sock_index;
} /* ms_get_next_sock_index */


/**
 * update socket event of the connections
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_update_conn_sock_event(ms_conn_t *c)
{
  assert(c != NULL);

  switch (c->currcmd.cmd)
  {
  case CMD_SET:
    if (ms_setting.facebook_test && c->udp)
    {
      c->sfd= c->tcpsfd[0];
      c->udp= false;
      c->change_sfd= true;
    }
    break;

  case CMD_GET:
    if (ms_setting.facebook_test && ! c->udp)
    {
      c->sfd= c->udpsfd;
      c->udp= true;
      c->change_sfd= true;
    }
    break;

  default:
    break;
  } /* switch */

  if (! c->udp && (c->total_sfds > 1))
  {
    if (c->cur_idx != c->total_sfds)
    {
      if (ms_setting.rep_write_srv == 0)
      {
        c->cur_idx= ms_get_next_sock_index(c);
      }
      else
      {
        c->cur_idx= ms_get_rep_sock_index(c, c->currcmd.cmd);
      }
    }
    else
    {
      /* must select the first sock of the connection at the beginning */
      c->cur_idx= 0;
    }

    c->sfd= c->tcpsfd[c->cur_idx];
    assert(c->sfd != 0);
    c->change_sfd= true;
  }

  if (c->change_sfd)
  {
    if (! ms_update_event(c, EV_WRITE | EV_PERSIST))
    {
      fprintf(stderr, "Couldn't update event.\n");
      ms_conn_set_state(c, conn_closing);
      return -1;
    }
  }

  return 0;
} /* ms_update_conn_sock_event */


/**
 * for ASCII protocol, this function build the set command
 * string and send the command.
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_build_ascii_write_buf_set(ms_conn_t *c, ms_task_item_t *item)
{
  int value_offset;
  int write_len;
  char *buffer= c->wbuf;

  write_len= sprintf(buffer,
                     " %u %d %d\r\n",
                     0,
                     item->exp_time,
                     item->value_size);

  if (write_len > c->wsize)
  {
    /* ought to be always enough. just fail for simplicity */
    fprintf(stderr, "output command line too long.\n");
    return -1;
  }

  if (item->value_offset == INVALID_OFFSET)
  {
    value_offset= item->key_suffix_offset;
  }
  else
  {
    value_offset= item->value_offset;
  }

  if ((ms_add_iov(c, "set ", 4) != 0)
      || (ms_add_iov(c, (char *)&item->key_prefix,
                     (int)KEY_PREFIX_SIZE) != 0)
      || (ms_add_iov(c, &ms_setting.char_block[item->key_suffix_offset],
                     item->key_size - (int)KEY_PREFIX_SIZE) != 0)
      || (ms_add_iov(c, buffer, write_len) != 0)
      || (ms_add_iov(c, &ms_setting.char_block[value_offset],
                     item->value_size) != 0)
      || (ms_add_iov(c, "\r\n", 2) != 0)
      || (c->udp && (ms_build_udp_headers(c) != 0)))
  {
    return -1;
  }

  return 0;
} /* ms_build_ascii_write_buf_set */


/**
 * used to send set command to server
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 *
 * @return int, if success, return 0, else return -1
 */
int ms_mcd_set(ms_conn_t *c, ms_task_item_t *item)
{
  assert(c != NULL);

  c->currcmd.cmd= CMD_SET;
  c->currcmd.isfinish= false;
  c->currcmd.retstat= MCD_FAILURE;

  if (ms_update_conn_sock_event(c) != 0)
  {
    return -1;
  }

  c->msgcurr= 0;
  c->msgused= 0;
  c->iovused= 0;
  if (ms_add_msghdr(c) != 0)
  {
    fprintf(stderr, "Out of memory preparing request.");
    return -1;
  }

  /* binary protocol */
  if (c->protocol == binary_prot)
  {
    if (ms_build_bin_write_buf_set(c, item) != 0)
    {
      return -1;
    }
  }
  else
  {
    if (ms_build_ascii_write_buf_set(c, item) != 0)
    {
      return -1;
    }
  }

  atomic_add_size(&ms_stats.obj_bytes,
                  item->key_size + item->value_size);
  atomic_add_size(&ms_stats.cmd_set, 1);

  return 0;
} /* ms_mcd_set */


/**
 * for ASCII protocol, this function build the get command
 * string and send the command.
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_build_ascii_write_buf_get(ms_conn_t *c, ms_task_item_t *item)
{
  if ((ms_add_iov(c, "get ", 4) != 0)
      || (ms_add_iov(c, (char *)&item->key_prefix,
                     (int)KEY_PREFIX_SIZE) != 0)
      || (ms_add_iov(c, &ms_setting.char_block[item->key_suffix_offset],
                     item->key_size - (int)KEY_PREFIX_SIZE) != 0)
      || (ms_add_iov(c, "\r\n", 2) != 0)
      || (c->udp && (ms_build_udp_headers(c) != 0)))
  {
    return -1;
  }

  return 0;
} /* ms_build_ascii_write_buf_get */


/**
 * used to send the get command to server
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 *
 * @return int, if success, return 0, else return -1
 */
int ms_mcd_get(ms_conn_t *c, ms_task_item_t *item)
{
  assert(c != NULL);

  c->currcmd.cmd= CMD_GET;
  c->currcmd.isfinish= false;
  c->currcmd.retstat= MCD_FAILURE;

  if (ms_update_conn_sock_event(c) != 0)
  {
    return -1;
  }

  c->msgcurr= 0;
  c->msgused= 0;
  c->iovused= 0;
  if (ms_add_msghdr(c) != 0)
  {
    fprintf(stderr, "Out of memory preparing request.");
    return -1;
  }

  /* binary protocol */
  if (c->protocol == binary_prot)
  {
    if (ms_build_bin_write_buf_get(c, item) != 0)
    {
      return -1;
    }
  }
  else
  {
    if (ms_build_ascii_write_buf_get(c, item) != 0)
    {
      return -1;
    }
  }

  atomic_add_size(&ms_stats.cmd_get, 1);

  return 0;
} /* ms_mcd_get */


/**
 * for ASCII protocol, this function build the multi-get command
 * string and send the command.
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_build_ascii_write_buf_mlget(ms_conn_t *c)
{
  ms_task_item_t *item;

  if (ms_add_iov(c, "get", 3) != 0)
  {
    return -1;
  }

  for (int i= 0; i < c->mlget_task.mlget_num; i++)
  {
    item= c->mlget_task.mlget_item[i].item;
    assert(item != NULL);
    if ((ms_add_iov(c, " ", 1) != 0)
        || (ms_add_iov(c, (char *)&item->key_prefix,
                       (int)KEY_PREFIX_SIZE) != 0)
        || (ms_add_iov(c, &ms_setting.char_block[item->key_suffix_offset],
                       item->key_size - (int)KEY_PREFIX_SIZE) != 0))
    {
      return -1;
    }
  }

  if ((ms_add_iov(c, "\r\n", 2) != 0)
      || (c->udp && (ms_build_udp_headers(c) != 0)))
  {
    return -1;
  }

  return 0;
} /* ms_build_ascii_write_buf_mlget */


/**
 * used to send the multi-get command to server
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
int ms_mcd_mlget(ms_conn_t *c)
{
  ms_task_item_t *item;

  assert(c != NULL);
  assert(c->mlget_task.mlget_num >= 1);

  c->currcmd.cmd= CMD_GET;
  c->currcmd.isfinish= false;
  c->currcmd.retstat= MCD_FAILURE;

  if (ms_update_conn_sock_event(c) != 0)
  {
    return -1;
  }

  c->msgcurr= 0;
  c->msgused= 0;
  c->iovused= 0;
  if (ms_add_msghdr(c) != 0)
  {
    fprintf(stderr, "Out of memory preparing request.");
    return -1;
  }

  /* binary protocol */
  if (c->protocol == binary_prot)
  {
    if (ms_build_bin_write_buf_mlget(c) != 0)
    {
      return -1;
    }
  }
  else
  {
    if (ms_build_ascii_write_buf_mlget(c) != 0)
    {
      return -1;
    }
  }

  /* decrease operation time of each item */
  for (int i= 0; i < c->mlget_task.mlget_num; i++)
  {
    item= c->mlget_task.mlget_item[i].item;
    atomic_add_size(&ms_stats.cmd_get, 1);
  }

  return 0;
} /* ms_mcd_mlget */


/**
 * binary protocol support
 */

/**
 * for binary protocol, parse the response of server
 *
 * @param c, pointer of the concurrency
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_bin_process_response(ms_conn_t *c)
{
  const char *errstr= NULL;

  assert(c != NULL);

  uint32_t bodylen= c->binary_header.response.bodylen;
  uint8_t  opcode= c->binary_header.response.opcode;
  uint16_t status= c->binary_header.response.status;

  if (bodylen > 0)
  {
    c->rvbytes= (int32_t)bodylen;
    c->readval= true;
    return 1;
  }
  else
  {
    switch (status)
    {
    case PROTOCOL_BINARY_RESPONSE_SUCCESS:
      if (opcode == PROTOCOL_BINARY_CMD_SET)
      {
        c->currcmd.retstat= MCD_STORED;
      }
      else if (opcode == PROTOCOL_BINARY_CMD_DELETE)
      {
        c->currcmd.retstat= MCD_DELETED;
      }
      else if (opcode == PROTOCOL_BINARY_CMD_GET)
      {
        c->currcmd.retstat= MCD_END;
      }
      break;

    case PROTOCOL_BINARY_RESPONSE_ENOMEM:
      errstr= "Out of memory";
      c->currcmd.retstat= MCD_SERVER_ERROR;
      break;

    case PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND:
      errstr= "Unknown command";
      c->currcmd.retstat= MCD_UNKNOWN_READ_FAILURE;
      break;

    case PROTOCOL_BINARY_RESPONSE_KEY_ENOENT:
      errstr= "Not found";
      c->currcmd.retstat= MCD_NOTFOUND;
      break;

    case PROTOCOL_BINARY_RESPONSE_EINVAL:
      errstr= "Invalid arguments";
      c->currcmd.retstat= MCD_PROTOCOL_ERROR;
      break;

    case PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS:
      errstr= "Data exists for key.";
      break;

    case PROTOCOL_BINARY_RESPONSE_E2BIG:
      errstr= "Too large.";
      c->currcmd.retstat= MCD_SERVER_ERROR;
      break;

    case PROTOCOL_BINARY_RESPONSE_NOT_STORED:
      errstr= "Not stored.";
      c->currcmd.retstat= MCD_NOTSTORED;
      break;

    default:
      errstr= "Unknown error";
      c->currcmd.retstat= MCD_UNKNOWN_READ_FAILURE;
      break;
    } /* switch */

    if (errstr != NULL)
    {
      fprintf(stderr, "%s\n", errstr);
    }
  }

  return 0;
} /* ms_bin_process_response */


/* build binary header and add the header to the buffer to send */

/**
 * build binary header and add the header to the buffer to send
 *
 * @param c, pointer of the concurrency
 * @param opcode, operation code
 * @param hdr_len, length of header
 * @param key_len, length of key
 * @param body_len. length of body
 */
static void ms_add_bin_header(ms_conn_t *c,
                              uint8_t opcode,
                              uint8_t hdr_len,
                              uint16_t key_len,
                              uint32_t body_len)
{
  protocol_binary_request_header *header;

  assert(c != NULL);

  header= (protocol_binary_request_header *)c->wcurr;

  header->request.magic= (uint8_t)PROTOCOL_BINARY_REQ;
  header->request.opcode= (uint8_t)opcode;
  header->request.keylen= htons(key_len);

  header->request.extlen= (uint8_t)hdr_len;
  header->request.datatype= (uint8_t)PROTOCOL_BINARY_RAW_BYTES;
  header->request.reserved= 0;

  header->request.bodylen= htonl(body_len);
  header->request.opaque= 0;
  header->request.cas= 0;

  ms_add_iov(c, c->wcurr, sizeof(header->request));
} /* ms_add_bin_header */


/**
 * add the key to the socket write buffer array
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 */
static void ms_add_key_to_iov(ms_conn_t *c, ms_task_item_t *item)
{
  ms_add_iov(c, (char *)&item->key_prefix, (int)KEY_PREFIX_SIZE);
  ms_add_iov(c, &ms_setting.char_block[item->key_suffix_offset],
             item->key_size - (int)KEY_PREFIX_SIZE);
}


/**
 * for binary protocol, this function build the set command
 * and add the command to send buffer array.
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_build_bin_write_buf_set(ms_conn_t *c, ms_task_item_t *item)
{
  assert(c->wbuf == c->wcurr);

  int value_offset;
  protocol_binary_request_set *rep= (protocol_binary_request_set *)c->wcurr;
  uint16_t keylen= (uint16_t)item->key_size;
  uint32_t bodylen= (uint32_t)sizeof(rep->message.body)
                    + (uint32_t)keylen + (uint32_t)item->value_size;

  ms_add_bin_header(c,
                    PROTOCOL_BINARY_CMD_SET,
                    sizeof(rep->message.body),
                    keylen,
                    bodylen);
  rep->message.body.flags= 0;
  rep->message.body.expiration= htonl((uint32_t)item->exp_time);
  ms_add_iov(c, &rep->message.body, sizeof(rep->message.body));
  ms_add_key_to_iov(c, item);

  if (item->value_offset == INVALID_OFFSET)
  {
    value_offset= item->key_suffix_offset;
  }
  else
  {
    value_offset= item->value_offset;
  }
  ms_add_iov(c, &ms_setting.char_block[value_offset], item->value_size);

  return 0;
} /* ms_build_bin_write_buf_set */


/**
 * for binary protocol, this function build the get command and
 * add the command to send buffer array.
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_build_bin_write_buf_get(ms_conn_t *c, ms_task_item_t *item)
{
  assert(c->wbuf == c->wcurr);

  ms_add_bin_header(c, PROTOCOL_BINARY_CMD_GET, 0, (uint16_t)item->key_size,
                    (uint32_t)item->key_size);
  ms_add_key_to_iov(c, item);

  return 0;
} /* ms_build_bin_write_buf_get */


/**
 * for binary protocol, this function build the multi-get
 * command and add the command to send buffer array.
 *
 * @param c, pointer of the concurrency
 * @param item, pointer of task item which includes the object
 *            information
 *
 * @return int, if success, return 0, else return -1
 */
static int ms_build_bin_write_buf_mlget(ms_conn_t *c)
{
  ms_task_item_t *item;

  assert(c->wbuf == c->wcurr);

  for (int i= 0; i < c->mlget_task.mlget_num; i++)
  {
    item= c->mlget_task.mlget_item[i].item;
    assert(item != NULL);

    ms_add_bin_header(c,
                      PROTOCOL_BINARY_CMD_GET,
                      0,
                      (uint16_t)item->key_size,
                      (uint32_t)item->key_size);
    ms_add_key_to_iov(c, item);
    c->wcurr+= sizeof(protocol_binary_request_get);
  }

  c->wcurr= c->wbuf;

  return 0;
} /* ms_build_bin_write_buf_mlget */
