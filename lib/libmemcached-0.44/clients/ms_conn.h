/*
 * File:   ms_conn.h
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */
#ifndef MS_CONN_H
#define MS_CONN_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <event.h>
#include <netdb.h>

#include "ms_task.h"
#include <libmemcached/memcached/protocol_binary.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_BUFFER_SIZE             (1024 * 1024 + 2048) /* read buffer, 1M + 2k, enough for the max value(1M) */
#define WRITE_BUFFER_SIZE            (32 * 1024)          /* write buffer, 32k */
#define UDP_DATA_BUFFER_SIZE         (1 * 1024 * 1024)    /* read buffer for UDP, 1M */
#define UDP_MAX_PAYLOAD_SIZE         1400                 /* server limit UDP payload size */
#define UDP_MAX_SEND_PAYLOAD_SIZE    1400                 /* mtu size is 1500 */
#define UDP_HEADER_SIZE              8                    /* UDP header size */
#define MAX_SENDBUF_SIZE             (256 * 1024 * 1024)  /* Maximum socket buffer size */
#define SOCK_WAIT_TIMEOUT            30                   /* maximum waiting time of UDP, 30s */
#define MAX_UDP_PACKET               (1 << 16)            /* maximum UDP packets, 65536 */

/* Initial size of the sendmsg() scatter/gather array. */
#define IOV_LIST_INITIAL             400

/* Initial number of sendmsg() argument structures to allocate. */
#define MSG_LIST_INITIAL             10

/* High water marks for buffer shrinking */
#define READ_BUFFER_HIGHWAT          (2 * DATA_BUFFER_SIZE)
#define UDP_DATA_BUFFER_HIGHWAT      (4 * UDP_DATA_BUFFER_SIZE)
#define IOV_LIST_HIGHWAT             600
#define MSG_LIST_HIGHWAT             100

/* parse udp header */
#define HEADER_TO_REQID(ptr)      ((uint16_t)*ptr * 256 \
                                   + (uint16_t)*(ptr + 1))
#define HEADER_TO_SEQNUM(ptr)     ((uint16_t)*(ptr        \
                                               + 2) * 256 \
                                   + (uint16_t)*(ptr + 3))
#define HEADER_TO_PACKETS(ptr)    ((uint16_t)*(ptr        \
                                               + 4) * 256 \
                                   + (uint16_t)*(ptr + 5))

/* states of connection */
enum conn_states
{
  conn_read,         /* reading in a command line */
  conn_write,        /* writing out a simple response */
  conn_closing,      /* closing this connection */
};

/* returned states of memcached command */
enum mcd_ret
{
  MCD_SUCCESS,                      /* command success */
  MCD_FAILURE,                      /* command failure */
  MCD_UNKNOWN_READ_FAILURE,         /* unknown read failure */
  MCD_PROTOCOL_ERROR,               /* protocol error */
  MCD_CLIENT_ERROR,                 /* client error, wrong command */
  MCD_SERVER_ERROR,                 /* server error, server run command failed */
  MCD_DATA_EXISTS,                  /* object is existent in server */
  MCD_NOTSTORED,                    /* server doesn't set the object successfully */
  MCD_STORED,                       /* server set the object successfully */
  MCD_NOTFOUND,                     /* server not find the object */
  MCD_END,                          /* end of the response of get command */
  MCD_DELETED,                      /* server delete the object successfully */
  MCD_STAT,                         /* response of stats command */
};

/* used to store the current or previous running command state */
typedef struct cmdstat
{
  int cmd;                  /* command name */
  int retstat;              /* return state of this command */
  bool isfinish;            /* if it read all the response data */
  uint64_t key_prefix;      /* key prefix */
} ms_cmdstat_t;

/* udp packet structure */
typedef struct udppkt
{
  uint8_t *header;          /* udp header of the packet */
  char *data;               /* udp data of the packet */
  int rbytes;               /* number of data in the packet */
  int copybytes;            /* number of copied data in the packet */
} ms_udppkt_t;

/* three protocols supported */
enum protocol
{
  ascii_prot = 3,           /* ASCII protocol */
  binary_prot,              /* binary protocol */
};

/**
 *  concurrency structure
 *
 *  Each thread has a libevent to manage the events of network.
 *  Each thread has one or more self-governed concurrencies;
 *  each concurrency has one or more socket connections. This
 *  concurrency structure includes all the private variables of
 *  the concurrency.
 */
typedef struct conn
{
  uint32_t conn_idx;             /* connection index in the thread */
  int sfd;                  /* current tcp sock handler of the connection structure */
  int udpsfd;               /* current udp sock handler of the connection structure*/
  int state;                /* state of the connection */
  struct event event;       /* event for libevent */
  short ev_flags;           /* event flag for libevent */
  short which;              /* which events were just triggered */
  bool change_sfd;          /* whether change sfd */

  int *tcpsfd;              /* TCP sock array */
  uint32_t total_sfds;           /* how many socks in the tcpsfd array */
  uint32_t alive_sfds;           /* alive socks */
  uint32_t cur_idx;              /* current sock index in tcpsfd array */

  ms_cmdstat_t precmd;      /* previous command state */
  ms_cmdstat_t currcmd;     /* current command state */

  char *rbuf;               /* buffer to read commands into */
  char *rcurr;              /* but if we parsed some already, this is where we stopped */
  int rsize;                /* total allocated size of rbuf */
  int rbytes;               /* how much data, starting from rcur, do we have unparsed */

  bool readval;             /* read value state, read known data size */
  int rvbytes;              /* total value size need to read */

  char *wbuf;               /* buffer to write commands out */
  char *wcurr;              /* for multi-get, where we stopped */
  int wsize;                /* total allocated size of wbuf */
  bool ctnwrite;            /* continue to write */

  /* data for the mwrite state */
  struct iovec *iov;
  int iovsize;              /* number of elements allocated in iov[] */
  int iovused;              /* number of elements used in iov[] */

  struct msghdr *msglist;
  int msgsize;              /* number of elements allocated in msglist[] */
  int msgused;              /* number of elements used in msglist[] */
  int msgcurr;              /* element in msglist[] being transmitted now */
  int msgbytes;             /* number of bytes in current msg */

  /* data for UDP clients */
  bool udp;                          /* is this is a UDP "connection" */
  uint32_t request_id;                   /* UDP request ID of current operation, if this is a UDP "connection" */
  uint8_t *hdrbuf;                  /* udp packet headers */
  int hdrsize;                      /* number of headers' worth of space is allocated */
  struct  sockaddr srv_recv_addr;   /* Sent the most recent request to which server */
  socklen_t srv_recv_addr_size;

  /* udp read buffer */
  char *rudpbuf;                    /* buffer to read commands into for udp */
  int rudpsize;                     /* total allocated size of rudpbuf */
  int rudpbytes;                    /* how much data, starting from rudpbuf */

  /* order udp packet */
  ms_udppkt_t *udppkt;              /* the offset of udp packet in rudpbuf */
  int packets;                      /* number of total packets need to read */
  int recvpkt;                      /* number of received packets */
  int pktcurr;                      /* current packet in rudpbuf being ordered */
  int ordcurr;                      /* current ordered packet */

  ms_task_item_t *item_win;         /* task sequence */
  int win_size;                     /* current task window size */
  uint64_t set_cursor;              /* current set item index in the item window */
  ms_task_t curr_task;              /* current running task */
  ms_mlget_task_t mlget_task;       /* multi-get task */

  int warmup_num;                   /* to run how many warm up operations*/
  int remain_warmup_num;            /* left how many warm up operations to run */
  int64_t exec_num;                 /* to run how many task operations */
  int64_t remain_exec_num;          /* how many remained task operations to run */

  /* response time statistic and time out control */
  struct timeval start_time;        /* start time of current operation(s) */
  struct timeval end_time;          /* end time of current operation(s) */

  /* Binary protocol stuff */
  protocol_binary_response_header binary_header;    /* local temporary binary header */
  enum protocol protocol;                           /* which protocol this connection speaks */
} ms_conn_t;

/* used to generate the key prefix */
uint64_t ms_get_key_prefix(void);


/**
 * setup a connection, each connection structure of each
 * thread must call this function to initialize.
 */
int ms_setup_conn(ms_conn_t *c);


/* after one operation completes, reset the connection */
void ms_reset_conn(ms_conn_t *c, bool timeout);


/**
 *  reconnect several disconnected socks in the connection
 *  structure, the ever-1-second timer of the thread will check
 *  whether some socks in the connections disconnect. if
 *  disconnect, reconnect the sock.
 */
int ms_reconn_socks(ms_conn_t *c);


/* used to send set command to server */
int ms_mcd_set(ms_conn_t *c, ms_task_item_t *item);


/* used to send the get command to server */
int ms_mcd_get(ms_conn_t *c, ms_task_item_t *item);


/* used to send the multi-get command to server */
int ms_mcd_mlget(ms_conn_t *c);


#ifdef __cplusplus
}
#endif

#endif /* end of MS_CONN_H */
