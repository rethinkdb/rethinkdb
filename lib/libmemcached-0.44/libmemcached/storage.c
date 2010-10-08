/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Storage related functions, aka set, replace,..
 *
 */

#include "common.h"

typedef enum {
  SET_OP,
  REPLACE_OP,
  ADD_OP,
  PREPEND_OP,
  APPEND_OP,
  CAS_OP,
} memcached_storage_action_t;

/* Inline this */
static inline const char *storage_op_string(memcached_storage_action_t verb)
{
  switch (verb)
  {
  case SET_OP:
    return "set ";
  case REPLACE_OP:
    return "replace ";
  case ADD_OP:
    return "add ";
  case PREPEND_OP:
    return "prepend ";
  case APPEND_OP:
    return "append ";
  case CAS_OP:
    return "cas ";
  default:
    return "tosserror"; /* This is impossible, fixes issue for compiler warning in VisualStudio */
  }

  /* NOTREACHED */
}

static memcached_return_t memcached_send_binary(memcached_st *ptr,
                                                memcached_server_write_instance_st server,
                                                uint32_t server_key,
                                                const char *key,
                                                size_t key_length,
                                                const char *value,
                                                size_t value_length,
                                                time_t expiration,
                                                uint32_t flags,
                                                uint64_t cas,
                                                memcached_storage_action_t verb);

static inline memcached_return_t memcached_send(memcached_st *ptr,
                                                const char *master_key, size_t master_key_length,
                                                const char *key, size_t key_length,
                                                const char *value, size_t value_length,
                                                time_t expiration,
                                                uint32_t flags,
                                                uint64_t cas,
                                                memcached_storage_action_t verb)
{
  bool to_write;
  size_t write_length;
  memcached_return_t rc;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  uint32_t server_key;
  memcached_server_write_instance_st instance;

  WATCHPOINT_ASSERT(!(value == NULL && value_length > 0));

  rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  unlikely (memcached_server_count(ptr) == 0)
    return MEMCACHED_NO_SERVERS;

  if (ptr->flags.verify_key && (memcached_key_test((const char **)&key, &key_length, 1) == MEMCACHED_BAD_KEY_PROVIDED))
    return MEMCACHED_BAD_KEY_PROVIDED;

  server_key= memcached_generate_hash_with_redistribution(ptr, master_key, master_key_length);
  instance= memcached_server_instance_fetch(ptr, server_key);

  WATCHPOINT_SET(instance->io_wait_count.read= 0);
  WATCHPOINT_SET(instance->io_wait_count.write= 0);

  if (ptr->flags.binary_protocol)
  {
    rc= memcached_send_binary(ptr, instance, server_key,
                              key, key_length,
                              value, value_length, expiration,
                              flags, cas, verb);
    WATCHPOINT_IF_LABELED_NUMBER(instance->io_wait_count.read > 2, "read IO_WAIT", instance->io_wait_count.read);
    WATCHPOINT_IF_LABELED_NUMBER(instance->io_wait_count.write > 2, "write_IO_WAIT", instance->io_wait_count.write);
  }
  else
  {

    if (cas)
    {
      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      "%s %.*s%.*s %u %llu %zu %llu%s\r\n",
                                      storage_op_string(verb),
                                      (int)ptr->prefix_key_length,
                                      ptr->prefix_key,
                                      (int)key_length, key, flags,
                                      (unsigned long long)expiration, value_length,
                                      (unsigned long long)cas,
                                      (ptr->flags.no_reply) ? " noreply" : "");
    }
    else
    {
      char *buffer_ptr= buffer;
      const char *command= storage_op_string(verb);

      /* Copy in the command, no space needed, we handle that in the command function*/
      memcpy(buffer_ptr, command, strlen(command));

      /* Copy in the key prefix, switch to the buffer_ptr */
      buffer_ptr= memcpy((buffer_ptr + strlen(command)), ptr->prefix_key, ptr->prefix_key_length);

      /* Copy in the key, adjust point if a key prefix was used. */
      buffer_ptr= memcpy(buffer_ptr + (ptr->prefix_key_length ? ptr->prefix_key_length : 0),
                         key, key_length);
      buffer_ptr+= key_length;
      buffer_ptr[0]=  ' ';
      buffer_ptr++;

      write_length= (size_t)(buffer_ptr - buffer);
      write_length+= (size_t) snprintf(buffer_ptr, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                       "%u %llu %zu%s\r\n",
                                       flags,
                                       (unsigned long long)expiration, value_length,
                                       ptr->flags.no_reply ? " noreply" : "");
    }

    if (ptr->flags.use_udp && ptr->flags.buffer_requests)
    {
      size_t cmd_size= write_length + value_length + 2;
      if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
        return MEMCACHED_WRITE_FAILURE;
      if (cmd_size + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
        memcached_io_write(instance, NULL, 0, true);
    }

    if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
    {
      rc= MEMCACHED_WRITE_FAILURE;
    }
    else
    {
      struct libmemcached_io_vector_st vector[]=
      {
        { .length= write_length, .buffer= buffer },
        { .length= value_length, .buffer= value },
        { .length= 2, .buffer= "\r\n" }
      };

      if (ptr->flags.buffer_requests && verb == SET_OP)
      {
        to_write= false;
      }
      else
      {
        to_write= true;
      }

      /* Send command header */
      rc=  memcached_vdo(instance, vector, 3, to_write);
      if (rc == MEMCACHED_SUCCESS)
      {

        if (ptr->flags.no_reply)
        {
          rc= (to_write == false) ? MEMCACHED_BUFFERED : MEMCACHED_SUCCESS;
        }
        else if (to_write == false)
        {
          rc= MEMCACHED_BUFFERED;
        }
        else
        {
          rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

          if (rc == MEMCACHED_STORED)
            rc= MEMCACHED_SUCCESS;
        }
      }
    }

    if (rc == MEMCACHED_WRITE_FAILURE)
      memcached_io_reset(instance);
  }

  WATCHPOINT_IF_LABELED_NUMBER(instance->io_wait_count.read > 2, "read IO_WAIT", instance->io_wait_count.read);
  WATCHPOINT_IF_LABELED_NUMBER(instance->io_wait_count.write > 2, "write_IO_WAIT", instance->io_wait_count.write);

  return rc;
}


memcached_return_t memcached_set(memcached_st *ptr, const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_SET_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, SET_OP);
  LIBMEMCACHED_MEMCACHED_SET_END();
  return rc;
}

memcached_return_t memcached_add(memcached_st *ptr,
                                 const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_ADD_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, ADD_OP);
  LIBMEMCACHED_MEMCACHED_ADD_END();
  return rc;
}

memcached_return_t memcached_replace(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const char *value, size_t value_length,
                                     time_t expiration,
                                     uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_REPLACE_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, REPLACE_OP);
  LIBMEMCACHED_MEMCACHED_REPLACE_END();
  return rc;
}

memcached_return_t memcached_prepend(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const char *value, size_t value_length,
                                     time_t expiration,
                                     uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, PREPEND_OP);
  return rc;
}

memcached_return_t memcached_append(memcached_st *ptr,
                                    const char *key, size_t key_length,
                                    const char *value, size_t value_length,
                                    time_t expiration,
                                    uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, APPEND_OP);
  return rc;
}

memcached_return_t memcached_cas(memcached_st *ptr,
                                 const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags,
                                 uint64_t cas)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, cas, CAS_OP);
  return rc;
}

memcached_return_t memcached_set_by_key(memcached_st *ptr,
                                        const char *master_key __attribute__((unused)),
                                        size_t master_key_length __attribute__((unused)),
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_SET_START();
  rc= memcached_send(ptr, master_key, master_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, SET_OP);
  LIBMEMCACHED_MEMCACHED_SET_END();
  return rc;
}

memcached_return_t memcached_add_by_key(memcached_st *ptr,
                                        const char *master_key, size_t master_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_ADD_START();
  rc= memcached_send(ptr, master_key, master_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, ADD_OP);
  LIBMEMCACHED_MEMCACHED_ADD_END();
  return rc;
}

memcached_return_t memcached_replace_by_key(memcached_st *ptr,
                                            const char *master_key, size_t master_key_length,
                                            const char *key, size_t key_length,
                                            const char *value, size_t value_length,
                                            time_t expiration,
                                            uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_REPLACE_START();
  rc= memcached_send(ptr, master_key, master_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, REPLACE_OP);
  LIBMEMCACHED_MEMCACHED_REPLACE_END();
  return rc;
}

memcached_return_t memcached_prepend_by_key(memcached_st *ptr,
                                            const char *master_key, size_t master_key_length,
                                            const char *key, size_t key_length,
                                            const char *value, size_t value_length,
                                            time_t expiration,
                                            uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, master_key, master_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, PREPEND_OP);
  return rc;
}

memcached_return_t memcached_append_by_key(memcached_st *ptr,
                                           const char *master_key, size_t master_key_length,
                                           const char *key, size_t key_length,
                                           const char *value, size_t value_length,
                                           time_t expiration,
                                           uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, master_key, master_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, APPEND_OP);
  return rc;
}

memcached_return_t memcached_cas_by_key(memcached_st *ptr,
                                        const char *master_key, size_t master_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags,
                                        uint64_t cas)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, master_key, master_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, cas, CAS_OP);
  return rc;
}

static inline uint8_t get_com_code(memcached_storage_action_t verb, bool noreply)
{
  /* 0 isn't a value we want, but GCC 4.2 seems to think ret can otherwise
   * be used uninitialized in this function. FAIL */
  uint8_t ret= 0;

  if (noreply)
    switch (verb)
    {
    case SET_OP:
      ret=PROTOCOL_BINARY_CMD_SETQ;
      break;
    case ADD_OP:
      ret=PROTOCOL_BINARY_CMD_ADDQ;
      break;
    case CAS_OP: /* FALLTHROUGH */
    case REPLACE_OP:
      ret=PROTOCOL_BINARY_CMD_REPLACEQ;
      break;
    case APPEND_OP:
      ret=PROTOCOL_BINARY_CMD_APPENDQ;
      break;
    case PREPEND_OP:
      ret=PROTOCOL_BINARY_CMD_PREPENDQ;
      break;
    default:
      WATCHPOINT_ASSERT(verb);
      break;
    }
  else
    switch (verb)
    {
    case SET_OP:
      ret=PROTOCOL_BINARY_CMD_SET;
      break;
    case ADD_OP:
      ret=PROTOCOL_BINARY_CMD_ADD;
      break;
    case CAS_OP: /* FALLTHROUGH */
    case REPLACE_OP:
      ret=PROTOCOL_BINARY_CMD_REPLACE;
      break;
    case APPEND_OP:
      ret=PROTOCOL_BINARY_CMD_APPEND;
      break;
    case PREPEND_OP:
      ret=PROTOCOL_BINARY_CMD_PREPEND;
      break;
    default:
      WATCHPOINT_ASSERT(verb);
      break;
    }

  return ret;
}



static memcached_return_t memcached_send_binary(memcached_st *ptr,
                                                memcached_server_write_instance_st server,
                                                uint32_t server_key,
                                                const char *key,
                                                size_t key_length,
                                                const char *value,
                                                size_t value_length,
                                                time_t expiration,
                                                uint32_t flags,
                                                uint64_t cas,
                                                memcached_storage_action_t verb)
{
  bool flush;
  protocol_binary_request_set request= {.bytes= {0}};
  size_t send_length= sizeof(request.bytes);

  bool noreply= server->root->flags.no_reply;

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= get_com_code(verb, noreply);
  request.message.header.request.keylen= htons((uint16_t)(key_length + ptr->prefix_key_length));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  if (verb == APPEND_OP || verb == PREPEND_OP)
    send_length -= 8; /* append & prepend does not contain extras! */
  else
  {
    request.message.header.request.extlen= 8;
    request.message.body.flags= htonl(flags);
    request.message.body.expiration= htonl((uint32_t)expiration);
  }

  request.message.header.request.bodylen= htonl((uint32_t) (key_length + ptr->prefix_key_length + value_length +
                                                            request.message.header.request.extlen));

  if (cas)
    request.message.header.request.cas= htonll(cas);

  flush= (bool) ((server->root->flags.buffer_requests && verb == SET_OP) ? 0 : 1);

  if (server->root->flags.use_udp && ! flush)
  {
    size_t cmd_size= send_length + key_length + value_length;

    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return MEMCACHED_WRITE_FAILURE;
    }
    if (cmd_size + server->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(server, NULL, 0, true);
    }
  }

  struct libmemcached_io_vector_st vector[]=
  {
    { .length= send_length, .buffer= request.bytes },
    { .length= ptr->prefix_key_length, .buffer= ptr->prefix_key },
    { .length= key_length, .buffer= key },
    { .length= value_length, .buffer= value }
  };

  /* write the header */
  memcached_return_t rc;
  if ((rc= memcached_vdo(server, vector, 4, flush)) != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(server);
    return (rc == MEMCACHED_SUCCESS) ? MEMCACHED_WRITE_FAILURE : rc;
  }

  if (verb == SET_OP && ptr->number_of_replicas > 0)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_SETQ;
    WATCHPOINT_STRING("replicating");

    for (uint32_t x= 0; x < ptr->number_of_replicas; x++)
    {
      memcached_server_write_instance_st instance;

      ++server_key;
      if (server_key == memcached_server_count(ptr))
        server_key= 0;

      instance= memcached_server_instance_fetch(ptr, server_key);

      if (memcached_vdo(instance, vector, 4, false) != MEMCACHED_SUCCESS)
      {
        memcached_io_reset(instance);
      }
      else
      {
        memcached_server_response_decrement(instance);
      }
    }
  }

  if (flush == false)
  {
    return MEMCACHED_BUFFERED;
  }

  if (noreply)
  {
    return MEMCACHED_SUCCESS;
  }

  return memcached_response(server, NULL, 0, NULL);
}

