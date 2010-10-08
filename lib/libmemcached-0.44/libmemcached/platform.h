/* LibMemcached
 * Copyright (C) 2006-2010 Brian Aker, Trond Norbye
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Try to hide platform-specific stuff
 *
 */
#ifndef LIBMEMCACHED_PLATFORM_H
#define LIBMEMCACHED_PLATFORM_H 1

#ifdef WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
typedef short in_port_t;
typedef SOCKET memcached_socket_t;
#else
typedef int memcached_socket_t;
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#endif /* WIN32 */


#endif /* LIBMEMCACHED_PLATFORM_H */
