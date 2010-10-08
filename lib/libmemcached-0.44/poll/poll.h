/* LibMemcached
 * Copyright (C) 2010 Brian Aker, Trond Norbye
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Implementation of poll by using select
 *
 */
#ifndef POLL_POLL_H
#define POLL_POLL_H 1

#ifdef WIN32
#include <winsock2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pollfd
{
#ifdef WIN32
  SOCKET fd;
#else
  int fd;
#endif
  short events;
  short revents;
} pollfd_t;

typedef int nfds_t;

#define POLLIN 0x0001
#define POLLOUT 0x0004
#define POLLERR 0x0008

int poll(struct pollfd fds[], nfds_t nfds, int tmo);

#ifdef __cplusplus
}
#endif

#endif
