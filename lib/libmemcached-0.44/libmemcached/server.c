/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: String structure used for libmemcached.
 *
 */

/*
  This is a partial implementation for fetching/creating memcached_server_st objects.
*/
#include "common.h"

static inline void _server_init(memcached_server_st *self, const memcached_st *root,
                                const char *hostname, in_port_t port,
                                uint32_t weight, memcached_connection_t type)
{
  self->options.sockaddr_inited= false;
  self->options.is_shutting_down= false;
  self->number_of_hosts= 0;
  self->cursor_active= 0;
  self->port= port;
  self->cached_errno= 0;
  self->fd= -1;
  self->io_bytes_sent= 0;
  self->server_failure_counter= 0;
  self->weight= weight;
  self->state.is_corked= false;
  self->state.is_dead= false;
  WATCHPOINT_SET(self->io_wait_count.read= 0);
  WATCHPOINT_SET(self->io_wait_count.write= 0);
  self->major_version= 0;
  self->micro_version= 0;
  self->minor_version= 0;
  self->type= type;
  self->read_ptr= self->read_buffer;
  self->cached_server_error= NULL;
  self->read_buffer_length= 0;
  self->read_data_length= 0;
  self->write_buffer_offset= 0;
  self->address_info= NULL;

  if (root)
  {
    self->next_retry= root->retry_timeout;
  }
  else
  {
    self->next_retry= 0;
  }

  self->root= root;
  self->limit_maxbytes= 0;
  if (hostname == NULL)
    self->hostname[0]= 0;
  else
    strncpy(self->hostname, hostname, NI_MAXHOST - 1);
}

static memcached_server_st *_server_create(memcached_server_st *self, const memcached_st *memc)
{
  if (self == NULL)
  {
   self= (memcached_server_st *)libmemcached_malloc(memc, sizeof(memcached_server_st));

    if (! self)
      return NULL; /*  MEMCACHED_MEMORY_ALLOCATION_FAILURE */

    self->options.is_allocated= true;
  }
  else
  {
    self->options.is_allocated= false;
  }

  self->options.is_initialized= true;

  return self;
}

memcached_server_st *memcached_server_create_with(const memcached_st *memc,
                                                  memcached_server_write_instance_st self,
                                                  const char *hostname, in_port_t port,
                                                  uint32_t weight, memcached_connection_t type)
{
  self= _server_create(self, memc);

  if (self == NULL)
    return NULL;

  _server_init(self, memc, hostname, port, weight, type);


  if (type == MEMCACHED_CONNECTION_UDP)
  {
    self->write_buffer_offset= UDP_DATAGRAM_HEADER_LENGTH;
    memcached_io_init_udp_header(self, 0);
  }

  return self;
}

void memcached_server_free(memcached_server_st *self)
{
  memcached_quit_server(self, false);

  if (self->cached_server_error)
    free(self->cached_server_error);

  if (self->address_info)
    freeaddrinfo(self->address_info);

  if (memcached_is_allocated(self))
  {
    libmemcached_free(self->root, self);
  }
  else
  {
    self->options.is_initialized= false;
  }
}

/*
  If we do not have a valid object to clone from, we toss an error.
*/
memcached_server_st *memcached_server_clone(memcached_server_st *destination,
                                            const memcached_server_st *source)
{
  /* We just do a normal create if source is missing */
  if (source == NULL)
    return NULL;

  destination= memcached_server_create_with(source->root, destination,
                                            source->hostname, source->port, source->weight,
                                            source->type);
  if (destination != NULL)
  {
    destination->cached_errno= source->cached_errno;

    if (source->cached_server_error)
      destination->cached_server_error= strdup(source->cached_server_error);
  }

  return destination;

}

memcached_return_t memcached_server_cursor(const memcached_st *ptr,
                                           const memcached_server_fn *callback,
                                           void *context,
                                           uint32_t number_of_callbacks)
{
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(ptr, x);

    for (uint32_t y= 0; y < number_of_callbacks; y++)
    {
      unsigned int iferror;

      iferror= (*callback[y])(ptr, instance, context);

      if (iferror)
        continue;
    }
  }

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_server_execute(memcached_st *ptr,
                                            memcached_server_execute_fn callback,
                                            void *context)
{
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    unsigned int iferror;

    iferror= (*callback)(ptr, instance, context);

    if (iferror)
      continue;
  }

  return MEMCACHED_SUCCESS;
}

memcached_server_instance_st memcached_server_by_key(const memcached_st *ptr,
                                                     const char *key,
                                                     size_t key_length,
                                                     memcached_return_t *error)
{
  uint32_t server_key;
  memcached_server_instance_st instance;

  *error= memcached_validate_key_length(key_length,
                                        ptr->flags.binary_protocol);
  unlikely (*error != MEMCACHED_SUCCESS)
    return NULL;

  unlikely (memcached_server_count(ptr) == 0)
  {
    *error= MEMCACHED_NO_SERVERS;
    return NULL;
  }

  if (ptr->flags.verify_key && (memcached_key_test((const char **)&key, &key_length, 1) == MEMCACHED_BAD_KEY_PROVIDED))
  {
    *error= MEMCACHED_BAD_KEY_PROVIDED;
    return NULL;
  }

  server_key= memcached_generate_hash(ptr, key, key_length);
  instance= memcached_server_instance_by_position(ptr, server_key);

  return instance;

}

void memcached_server_error_reset(memcached_server_st *ptr)
{
  ptr->cached_server_error[0]= 0;
}

memcached_server_instance_st memcached_server_get_last_disconnect(const memcached_st *ptr)
{
  return ptr->last_disconnected_server;
}

void memcached_server_list_free(memcached_server_list_st self)
{
  if (self == NULL)
    return;

  const memcached_st *root= self->root;

  for (uint32_t x= 0; x < memcached_server_list_count(self); x++)
  {
    if (self[x].address_info)
    {
      freeaddrinfo(self[x].address_info);
      self[x].address_info= NULL;
    }
  }

  if (root)
  {
    libmemcached_free(root, self);
  }
  else
  {
    free(self);
  }
}

uint32_t memcached_servers_set_count(memcached_server_st *servers, uint32_t count)
{
  return servers->number_of_hosts= count;
}

uint32_t memcached_server_count(const memcached_st *self)
{
  return self->number_of_hosts;
}

const char *memcached_server_name(memcached_server_instance_st self)
{
  return self->hostname;
}

in_port_t memcached_server_port(memcached_server_instance_st self)
{
  return self->port;
}

uint32_t memcached_server_response_count(memcached_server_instance_st self)
{
  return self->cursor_active;
}

const char *memcached_server_error(memcached_server_instance_st ptr)
{
  return ptr
    ?  ptr->cached_server_error
    : NULL;
}

