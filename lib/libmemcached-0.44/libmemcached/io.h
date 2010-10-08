/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Server IO, Not public!
 *
 */

#ifndef __LIBMEMCACHED_IO_H__
#define __LIBMEMCACHED_IO_H__

#if defined(BUILDING_LIBMEMCACHED)

#include "libmemcached/memcached.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_UDP_DATAGRAM_LENGTH 1400
#define UDP_DATAGRAM_HEADER_LENGTH 8
#define UDP_REQUEST_ID_MSG_SIG_DIGITS 10
#define UDP_REQUEST_ID_THREAD_MASK 0xFFFF << UDP_REQUEST_ID_MSG_SIG_DIGITS
#define get_udp_datagram_request_id(A) ntohs((A)->request_id)
#define get_udp_datagram_seq_num(A) ntohs((A)->sequence_number)
#define get_udp_datagram_num_datagrams(A) ntohs((A)->num_datagrams)
#define get_msg_num_from_request_id(A) ( (A) & (~(UDP_REQUEST_ID_THREAD_MASK)) )
#define get_thread_id_from_request_id(A) ( (A) & (UDP_REQUEST_ID_THREAD_MASK) ) >> UDP_REQUEST_ID_MSG_SIG_DIGITS
#define generate_udp_request_thread_id(A) (A) << UDP_REQUEST_ID_MSG_SIG_DIGITS
#define UDP_REQUEST_ID_MAX_THREAD_ID get_thread_id_from_request_id(0xFFFF)

struct udp_datagram_header_st
{
  uint16_t request_id;
  uint16_t sequence_number;
  uint16_t num_datagrams;
  uint16_t reserved;
};

struct libmemcached_io_vector_st
{
  size_t length;
  const void *buffer;
};

LIBMEMCACHED_LOCAL
ssize_t memcached_io_writev(memcached_server_write_instance_st ptr,
                            const struct libmemcached_io_vector_st *vector,
                            size_t number_of, bool with_flush);

LIBMEMCACHED_LOCAL
ssize_t memcached_io_write(memcached_server_write_instance_st ptr,
                           const void *buffer, size_t length, bool with_flush);

LIBMEMCACHED_LOCAL
void memcached_io_reset(memcached_server_write_instance_st ptr);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_io_read(memcached_server_write_instance_st ptr,
                                     void *buffer, size_t length, ssize_t *nread);

/* Read a line (terminated by '\n') into the buffer */
LIBMEMCACHED_LOCAL
memcached_return_t memcached_io_readline(memcached_server_write_instance_st ptr,
                                         char *buffer_ptr,
                                         size_t size);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_io_close(memcached_server_write_instance_st ptr);

/* Read n bytes of data from the server and store them in dta */
LIBMEMCACHED_LOCAL
memcached_return_t memcached_safe_read(memcached_server_write_instance_st ptr,
                                       void *dta,
                                       size_t size);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_io_init_udp_header(memcached_server_write_instance_st ptr,
                                                uint16_t thread_id);

LIBMEMCACHED_LOCAL
memcached_server_write_instance_st memcached_io_get_readable_server(memcached_st *memc);

#ifdef __cplusplus
}
#endif

#endif /* BUILDING_LIBMEMCACHED */

#endif /* __LIBMEMCACHED_IO_H__ */
