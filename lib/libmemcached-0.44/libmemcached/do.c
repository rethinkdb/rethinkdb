/* LibMemcached
 * Copyright (C) 2006-2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 */

#include "common.h"

memcached_return_t memcached_do(memcached_server_write_instance_st ptr, const void *command,
                                size_t command_length, bool with_flush)
{
  memcached_return_t rc;
  ssize_t sent_length;

  WATCHPOINT_ASSERT(command_length);
  WATCHPOINT_ASSERT(command);

  if ((rc= memcached_connect(ptr)) != MEMCACHED_SUCCESS)
  {
    WATCHPOINT_ERROR(rc);
    return rc;
  }

  /*
  ** Since non buffering ops in UDP mode dont check to make sure they will fit
  ** before they start writing, if there is any data in buffer, clear it out,
  ** otherwise we might get a partial write.
  **/
  if (ptr->type == MEMCACHED_CONNECTION_UDP && with_flush && ptr->write_buffer_offset > UDP_DATAGRAM_HEADER_LENGTH)
  {
    memcached_io_write(ptr, NULL, 0, true);
  }

  sent_length= memcached_io_write(ptr, command, command_length, with_flush);

  if (sent_length == -1 || (size_t)sent_length != command_length)
  {
    rc= MEMCACHED_WRITE_FAILURE;
  }
  else if ((ptr->root->flags.no_reply) == 0)
  {
    memcached_server_response_increment(ptr);
  }

  return rc;
}

memcached_return_t memcached_vdo(memcached_server_write_instance_st ptr,
                                 const struct libmemcached_io_vector_st *vector, size_t count,
                                 bool with_flush)
{
  memcached_return_t rc;
  ssize_t sent_length;

  WATCHPOINT_ASSERT(count);
  WATCHPOINT_ASSERT(vector);

  if ((rc= memcached_connect(ptr)) != MEMCACHED_SUCCESS)
  {
    WATCHPOINT_ERROR(rc);
    return rc;
  }

  /*
  ** Since non buffering ops in UDP mode dont check to make sure they will fit
  ** before they start writing, if there is any data in buffer, clear it out,
  ** otherwise we might get a partial write.
  **/
  if (ptr->type == MEMCACHED_CONNECTION_UDP && with_flush && ptr->write_buffer_offset > UDP_DATAGRAM_HEADER_LENGTH)
  {
    memcached_io_write(ptr, NULL, 0, true);
  }

  sent_length= memcached_io_writev(ptr, vector, count, with_flush);

  size_t command_length= 0;
  for (uint32_t x= 0; x < count; ++x, vector++)
  {
    command_length+= vector->length;
  }

  if (sent_length == -1 || (size_t)sent_length != command_length)
  {
    rc= MEMCACHED_WRITE_FAILURE;
    WATCHPOINT_ERROR(rc);
    WATCHPOINT_ERRNO(errno);
  }
  else if ((ptr->root->flags.no_reply) == 0)
  {
    memcached_server_response_increment(ptr);
  }

  return rc;
}
