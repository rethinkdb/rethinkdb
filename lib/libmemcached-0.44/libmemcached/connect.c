/* LibMemcached
 * Copyright (C) 2006-2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Server IO, Not public!
 *
 */

#include "common.h"
#include <sys/time.h>
#include <time.h>

static memcached_return_t connect_poll(memcached_server_st *ptr)
{
  struct pollfd fds[1];
  fds[0].fd = ptr->fd;
  fds[0].events = POLLOUT;

  int timeout= ptr->root->connect_timeout;
  if (ptr->root->flags.no_block == true)
    timeout= -1;

  int error;
  size_t loop_max= 5;

  while (--loop_max) // Should only loop on cases of ERESTART or EINTR
  {
    error= poll(fds, 1, timeout);

    switch (error)
    {
    case 1:
      {
        int err;
        socklen_t len= sizeof (err);
        (void)getsockopt(ptr->fd, SOL_SOCKET, SO_ERROR, &err, &len);

        // We check the value to see what happened wth the socket.
        if (err == 0)
        {
          return MEMCACHED_SUCCESS;
        }
        else
        {
          ptr->cached_errno= errno;

          return MEMCACHED_ERRNO;
        }
      }
    case 0:
      return MEMCACHED_TIMEOUT;
    default: // A real error occurred and we need to completely bail
      WATCHPOINT_ERRNO(get_socket_errno());
      switch (get_socket_errno())
      {
#ifdef TARGET_OS_LINUX
      case ERESTART:
#endif
      case EINTR:
        continue;
      default:
        if (fds[0].revents & POLLERR)
        {
          int err;
          socklen_t len= sizeof (err);
          (void)getsockopt(ptr->fd, SOL_SOCKET, SO_ERROR, &err, &len);
          ptr->cached_errno= (err == 0) ? get_socket_errno() : err;
        }
        else
        {
          ptr->cached_errno= get_socket_errno();
        }

        (void)closesocket(ptr->fd);
        ptr->fd= INVALID_SOCKET;

        return MEMCACHED_ERRNO;
      }
    }
  }

  // This should only be possible from ERESTART or EINTR;
  ptr->cached_errno= get_socket_errno();

  return MEMCACHED_ERRNO;
}

static memcached_return_t set_hostinfo(memcached_server_st *server)
{
  struct addrinfo *ai;
  struct addrinfo hints;
  char str_port[NI_MAXSERV];
  uint32_t counter= 5;

  snprintf(str_port, NI_MAXSERV, "%u", (uint32_t)server->port);

  memset(&hints, 0, sizeof(hints));

 // hints.ai_family= AF_INET;
  if (server->type == MEMCACHED_CONNECTION_UDP)
  {
    hints.ai_protocol= IPPROTO_UDP;
    hints.ai_socktype= SOCK_DGRAM;
  }
  else
  {
    hints.ai_socktype= SOCK_STREAM;
    hints.ai_protocol= IPPROTO_TCP;
  }

  while (--counter)
  {
    int e= getaddrinfo(server->hostname, str_port, &hints, &ai);

    if (e == 0)
    {
      break;
    }
    else if (e == EAI_AGAIN)
    {
#ifndef WIN32
      struct timespec dream, rem;

      dream.tv_nsec= 1000;
      dream.tv_sec= 0;

      nanosleep(&dream, &rem);
#endif
      continue;
    }
    else
    {
      WATCHPOINT_STRING(server->hostname);
      WATCHPOINT_STRING(gai_strerror(e));
      return MEMCACHED_HOST_LOOKUP_FAILURE;
    }
  }

  if (server->address_info)
  {
    freeaddrinfo(server->address_info);
    server->address_info= NULL;
  }
  server->address_info= ai;

  return MEMCACHED_SUCCESS;
}

static inline memcached_return_t set_socket_nonblocking(memcached_server_st *ptr)
{
#ifdef WIN32
  u_long arg = 1;
  if (ioctlsocket(ptr->fd, FIONBIO, &arg) == SOCKET_ERROR)
  {
    ptr->cached_errno= get_socket_errno();
    return MEMCACHED_CONNECTION_FAILURE;
  }
#else
  int flags;

  do
    flags= fcntl(ptr->fd, F_GETFL, 0);
  while (flags == -1 && (errno == EINTR || errno == EAGAIN));

  unlikely (flags == -1)
  {
    ptr->cached_errno= errno;
    return MEMCACHED_CONNECTION_FAILURE;
  }
  else if ((flags & O_NONBLOCK) == 0)
  {
    int rval;

    do
      rval= fcntl(ptr->fd, F_SETFL, flags | O_NONBLOCK);
    while (rval == -1 && (errno == EINTR || errno == EAGAIN));

    unlikely (rval == -1)
    {
      ptr->cached_errno= errno;
      return MEMCACHED_CONNECTION_FAILURE;
    }
  }
#endif
  return MEMCACHED_SUCCESS;
}

static memcached_return_t set_socket_options(memcached_server_st *ptr)
{
  WATCHPOINT_ASSERT(ptr->fd != -1);

  if (ptr->type == MEMCACHED_CONNECTION_UDP)
    return MEMCACHED_SUCCESS;

#ifdef HAVE_SNDTIMEO
  if (ptr->root->snd_timeout)
  {
    int error;
    struct timeval waittime;

    waittime.tv_sec= 0;
    waittime.tv_usec= ptr->root->snd_timeout;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_SNDTIMEO,
                      &waittime, (socklen_t)sizeof(struct timeval));
    WATCHPOINT_ASSERT(error == 0);
    if (error)
      return MEMCACHED_FAILURE;
  }
#endif

#ifdef HAVE_RCVTIMEO
  if (ptr->root->rcv_timeout)
  {
    int error;
    struct timeval waittime;

    waittime.tv_sec= 0;
    waittime.tv_usec= ptr->root->rcv_timeout;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_RCVTIMEO,
                      &waittime, (socklen_t)sizeof(struct timeval));
    WATCHPOINT_ASSERT(error == 0);
    if (error)
      return MEMCACHED_FAILURE;
  }
#endif


#if defined(__MACH__) && defined(__APPLE__) || defined(__FreeBSD__)
  {
    int set = 1;
    int error= setsockopt(ptr->fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

    // This is not considered a fatal error
    if (error == -1)
    {
      WATCHPOINT_ERRNO(get_socket_errno());
      perror("setsockopt(SO_NOSIGPIPE)");
    }
  }
#endif

  if (ptr->root->flags.no_block)
  {
    int error;
    struct linger linger;

    linger.l_onoff= 1;
    linger.l_linger= 0; /* By default on close() just drop the socket */
    error= setsockopt(ptr->fd, SOL_SOCKET, SO_LINGER,
                      &linger, (socklen_t)sizeof(struct linger));
    WATCHPOINT_ASSERT(error == 0);
    if (error)
      return MEMCACHED_FAILURE;
  }

  if (ptr->root->flags.tcp_nodelay)
  {
    int flag= 1;
    int error;

    error= setsockopt(ptr->fd, IPPROTO_TCP, TCP_NODELAY,
                      &flag, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
    if (error)
      return MEMCACHED_FAILURE;
  }

  if (ptr->root->flags.tcp_keepalive)
  {
    int flag= 1;
    int error;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_KEEPALIVE,
                      &flag, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
    if (error)
      return MEMCACHED_FAILURE;
  }

#ifdef TCP_KEEPIDLE
  if (ptr->root->tcp_keepidle > 0)
  {
    int error;

    error= setsockopt(ptr->fd, IPPROTO_TCP, TCP_KEEPIDLE,
                      &ptr->root->tcp_keepidle, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
    if (error)
      return MEMCACHED_FAILURE;
  }
#endif

  if (ptr->root->send_size > 0)
  {
    int error;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_SNDBUF,
                      &ptr->root->send_size, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
    if (error)
      return MEMCACHED_FAILURE;
  }

  if (ptr->root->recv_size > 0)
  {
    int error;

    error= setsockopt(ptr->fd, SOL_SOCKET, SO_RCVBUF,
                      &ptr->root->recv_size, (socklen_t)sizeof(int));
    WATCHPOINT_ASSERT(error == 0);
    if (error)
      return MEMCACHED_FAILURE;
  }


  /* libmemcached will always use nonblocking IO to avoid write deadlocks */
  return set_socket_nonblocking(ptr);
}

static memcached_return_t unix_socket_connect(memcached_server_st *ptr)
{
#ifndef WIN32
  struct sockaddr_un servAddr;

  WATCHPOINT_ASSERT(ptr->fd == -1);

  if ((ptr->fd= socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
  {
    ptr->cached_errno= errno;
    return MEMCACHED_CONNECTION_SOCKET_CREATE_FAILURE;
  }

  memset(&servAddr, 0, sizeof (struct sockaddr_un));
  servAddr.sun_family= AF_UNIX;
  strcpy(servAddr.sun_path, ptr->hostname); /* Copy filename */

test_connect:
  if (connect(ptr->fd,
              (struct sockaddr *)&servAddr,
              sizeof(servAddr)) < 0)
  {
    switch (errno)
    {
    case EINPROGRESS:
    case EALREADY:
    case EINTR:
      goto test_connect;
    case EISCONN: /* We were spinning waiting on connect */
      break;
    default:
      WATCHPOINT_ERRNO(errno);
      ptr->cached_errno= errno;
      return MEMCACHED_ERRNO;
    }
  }

  WATCHPOINT_ASSERT(ptr->fd != -1);

  return MEMCACHED_SUCCESS;
#else
  (void)ptr;
  return MEMCACHED_NOT_SUPPORTED;
#endif
}

static memcached_return_t network_connect(memcached_server_st *ptr)
{
  bool timeout_error_occured= false;


  WATCHPOINT_ASSERT(ptr->fd == INVALID_SOCKET);
  WATCHPOINT_ASSERT(ptr->cursor_active == 0);

  if (! ptr->options.sockaddr_inited || (!(ptr->root->flags.use_cache_lookups)))
  {
    memcached_return_t rc;

    rc= set_hostinfo(ptr);
    if (rc != MEMCACHED_SUCCESS)
      return rc;
    ptr->options.sockaddr_inited= true;
  }

  struct addrinfo *use= ptr->address_info;
  /* Create the socket */
  while (use != NULL)
  {
    /* Memcache server does not support IPV6 in udp mode, so skip if not ipv4 */
    if (ptr->type == MEMCACHED_CONNECTION_UDP && use->ai_family != AF_INET)
    {
      use= use->ai_next;
      continue;
    }

    if ((ptr->fd= socket(use->ai_family,
                         use->ai_socktype,
                         use->ai_protocol)) < 0)
    {
      ptr->cached_errno= get_socket_errno();
      WATCHPOINT_ERRNO(get_socket_errno());
      return MEMCACHED_CONNECTION_SOCKET_CREATE_FAILURE;
    }

    (void)set_socket_options(ptr);

    /* connect to server */
    if ((connect(ptr->fd, use->ai_addr, use->ai_addrlen) != SOCKET_ERROR))
    {
      break; // Success
    }

    /* An error occurred */
    ptr->cached_errno= get_socket_errno();
    if (ptr->cached_errno == EWOULDBLOCK ||
        ptr->cached_errno == EINPROGRESS || /* nonblocking mode - first return, */
        ptr->cached_errno == EALREADY) /* nonblocking mode - subsequent returns */
    {
      memcached_return_t rc;
      rc= connect_poll(ptr);

      if (rc == MEMCACHED_TIMEOUT)
        timeout_error_occured= true;

      if (rc == MEMCACHED_SUCCESS)
        break;
    }
    else if (get_socket_errno() == EISCONN) /* we are connected :-) */
    {
      break;
    }
    else if (get_socket_errno() == EINTR) // Special case, we retry ai_addr
    {
      (void)closesocket(ptr->fd);
      ptr->fd= INVALID_SOCKET;
      continue;
    }

    (void)closesocket(ptr->fd);
    ptr->fd= INVALID_SOCKET;
    use= use->ai_next;
  }

  if (ptr->fd == INVALID_SOCKET)
  {
    WATCHPOINT_STRING("Never got a good file descriptor");

    /* Failed to connect. schedule next retry */
    if (ptr->root->retry_timeout)
    {
      struct timeval next_time;

      if (gettimeofday(&next_time, NULL) == 0)
        ptr->next_retry= next_time.tv_sec + ptr->root->retry_timeout;
    }

    if (timeout_error_occured)
      return MEMCACHED_TIMEOUT;

    return MEMCACHED_ERRNO; /* The last error should be from connect() */
  }

  return MEMCACHED_SUCCESS; /* The last error should be from connect() */
}

void set_last_disconnected_host(memcached_server_write_instance_st ptr)
{
  // const_cast
  memcached_st *root= (memcached_st *)ptr->root;

#if 0
  WATCHPOINT_STRING(ptr->hostname);
  WATCHPOINT_NUMBER(ptr->port);
  WATCHPOINT_ERRNO(ptr->cached_errno);
#endif
  if (root->last_disconnected_server)
    memcached_server_free(root->last_disconnected_server);
  root->last_disconnected_server= memcached_server_clone(NULL, ptr);
}

memcached_return_t memcached_connect(memcached_server_write_instance_st ptr)
{
  memcached_return_t rc= MEMCACHED_NO_SERVERS;

  if (ptr->fd != INVALID_SOCKET)
    return MEMCACHED_SUCCESS;

  LIBMEMCACHED_MEMCACHED_CONNECT_START();

  /* both retry_timeout and server_failure_limit must be set in order to delay retrying a server on error. */
  WATCHPOINT_ASSERT(ptr->root);
  if (ptr->root->retry_timeout && ptr->next_retry)
  {
    struct timeval curr_time;

    gettimeofday(&curr_time, NULL);

    // We should optimize this to remove the allocation if the server was
    // the last server to die
    if (ptr->next_retry > curr_time.tv_sec)
    {
      set_last_disconnected_host(ptr);

      return MEMCACHED_SERVER_MARKED_DEAD;
    }
  }

  // If we are over the counter failure, we just fail. Reject host only
  // works if you have a set number of failures.
  if (ptr->root->server_failure_limit && ptr->server_failure_counter >= ptr->root->server_failure_limit)
  {
    set_last_disconnected_host(ptr);

    // @todo fix this by fixing behavior to no longer make use of
    // memcached_st
    if (_is_auto_eject_host(ptr->root))
    {
      run_distribution((memcached_st *)ptr->root);
    }

    return MEMCACHED_SERVER_MARKED_DEAD;
  }

  /* We need to clean up the multi startup piece */
  switch (ptr->type)
  {
  case MEMCACHED_CONNECTION_UNKNOWN:
    WATCHPOINT_ASSERT(0);
    rc= MEMCACHED_NOT_SUPPORTED;
    break;
  case MEMCACHED_CONNECTION_UDP:
  case MEMCACHED_CONNECTION_TCP:
    rc= network_connect(ptr);
#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
    if (ptr->fd != INVALID_SOCKET && ptr->root->sasl.callbacks)
    {
      rc= memcached_sasl_authenticate_connection(ptr);
      if (rc != MEMCACHED_SUCCESS)
      {
        (void)closesocket(ptr->fd);
        ptr->fd= INVALID_SOCKET;
      }
    }
#endif
    break;
  case MEMCACHED_CONNECTION_UNIX_SOCKET:
    rc= unix_socket_connect(ptr);
    break;
  case MEMCACHED_CONNECTION_MAX:
  default:
    WATCHPOINT_ASSERT(0);
  }

  if (rc == MEMCACHED_SUCCESS)
  {
    ptr->server_failure_counter= 0;
    ptr->next_retry= 0;
  }
  else
  {
    ptr->server_failure_counter++;

    set_last_disconnected_host(ptr);
  }

  LIBMEMCACHED_MEMCACHED_CONNECT_END();

  return rc;
}
