#include "common.h"
#include "memcached/protocol_binary.h"

memcached_return_t memcached_delete(memcached_st *ptr, const char *key, size_t key_length,
                                    time_t expiration)
{
  return memcached_delete_by_key(ptr, key, key_length,
                                 key, key_length, expiration);
}

static inline memcached_return_t binary_delete(memcached_st *ptr,
                                               uint32_t server_key,
                                               const char *key,
                                               size_t key_length,
                                               bool flush);

memcached_return_t memcached_delete_by_key(memcached_st *ptr,
                                           const char *master_key, size_t master_key_length,
                                           const char *key, size_t key_length,
                                           time_t expiration)
{
  bool to_write;
  size_t send_length;
  memcached_return_t rc;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  uint32_t server_key;
  memcached_server_write_instance_st instance;

  LIBMEMCACHED_MEMCACHED_DELETE_START();

  rc= memcached_validate_key_length(key_length,
                                    ptr->flags.binary_protocol);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  unlikely (memcached_server_count(ptr) == 0)
    return MEMCACHED_NO_SERVERS;

  server_key= memcached_generate_hash_with_redistribution(ptr, master_key, master_key_length);
  instance= memcached_server_instance_fetch(ptr, server_key);

  to_write= (ptr->flags.buffer_requests) ? false : true;

  bool no_reply= (ptr->flags.no_reply);

  if (ptr->flags.binary_protocol)
  {
    likely (! expiration)
    {
      rc= binary_delete(ptr, server_key, key, key_length, to_write);
    }
    else
    {
      rc= MEMCACHED_INVALID_ARGUMENTS;
    }
  }
  else
  {
    unlikely (expiration)
    {
       if ((instance->major_version == 1 &&
            instance->minor_version > 2) ||
           instance->major_version > 1)
       {
         rc= MEMCACHED_INVALID_ARGUMENTS;
         goto error;
       }
       else
       {
          /* ensure that we are connected, otherwise we might bump the
           * command counter before connection */
          if ((rc= memcached_connect(instance)) != MEMCACHED_SUCCESS)
          {
            WATCHPOINT_ERROR(rc);
            return rc;
          }

          if (instance->minor_version == 0)
          {
             if (no_reply || ! to_write)
             {
                /* We might get out of sync with the server if we
                 * send this command to a server newer than 1.2.x..
                 * disable no_reply and buffered mode.
                 */
                to_write= true;
                if (no_reply)
                   memcached_server_response_increment(instance);
                no_reply= false;
             }
          }
          send_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                         "delete %.*s%.*s %u%s\r\n",
                                         (int)ptr->prefix_key_length,
                                         ptr->prefix_key,
                                         (int) key_length, key,
                                         (uint32_t)expiration,
                                         no_reply ? " noreply" :"" );
       }
    }
    else
    {
      send_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     "delete %.*s%.*s%s\r\n",
                                     (int)ptr->prefix_key_length,
                                     ptr->prefix_key,
                                     (int)key_length, key, no_reply ? " noreply" :"");
    }

    if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
    {
      rc= MEMCACHED_WRITE_FAILURE;
      goto error;
    }

    if (ptr->flags.use_udp && ! to_write)
    {
      if (send_length > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
        return MEMCACHED_WRITE_FAILURE;
      if (send_length + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
        memcached_io_write(instance, NULL, 0, true);
    }

    rc= memcached_do(instance, buffer, send_length, to_write);
  }

  if (rc != MEMCACHED_SUCCESS)
    goto error;

  if (! to_write)
  {
    rc= MEMCACHED_BUFFERED;
  }
  else if (!no_reply)
  {
    rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
    if (rc == MEMCACHED_DELETED)
      rc= MEMCACHED_SUCCESS;
  }

  if (rc == MEMCACHED_SUCCESS && ptr->delete_trigger)
    ptr->delete_trigger(ptr, key, key_length);

error:
  LIBMEMCACHED_MEMCACHED_DELETE_END();
  return rc;
}

static inline memcached_return_t binary_delete(memcached_st *ptr,
                                               uint32_t server_key,
                                               const char *key,
                                               size_t key_length,
                                               bool flush)
{
  memcached_server_write_instance_st instance;
  protocol_binary_request_delete request= {.bytes= {0}};

  instance= memcached_server_instance_fetch(ptr, server_key);

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  if (ptr->flags.no_reply)
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETEQ;
  else
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETE;
  request.message.header.request.keylen= htons((uint16_t)(key_length + ptr->prefix_key_length));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl((uint32_t)(key_length + ptr->prefix_key_length));

  if (ptr->flags.use_udp && ! flush)
  {
    size_t cmd_size= sizeof(request.bytes) + key_length;
    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
      return MEMCACHED_WRITE_FAILURE;
    if (cmd_size + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
      memcached_io_write(instance, NULL, 0, true);
  }

  struct libmemcached_io_vector_st vector[]=
  {
    { .length= sizeof(request.bytes), .buffer= request.bytes},
    { .length= ptr->prefix_key_length, .buffer= ptr->prefix_key },
    { .length= key_length, .buffer= key },
  };

  memcached_return_t rc= MEMCACHED_SUCCESS;

  if ((rc= memcached_vdo(instance, vector,  3, flush)) != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(instance);
    rc= (rc == MEMCACHED_SUCCESS) ? MEMCACHED_WRITE_FAILURE : rc;
  }

  unlikely (ptr->number_of_replicas > 0)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETEQ;

    for (uint32_t x= 0; x < ptr->number_of_replicas; ++x)
    {
      memcached_server_write_instance_st replica;

      ++server_key;
      if (server_key == memcached_server_count(ptr))
        server_key= 0;

      replica= memcached_server_instance_fetch(ptr, server_key);

      if (memcached_vdo(replica, vector, 3, flush) != MEMCACHED_SUCCESS)
      {
        memcached_io_reset(replica);
      }
      else
      {
        memcached_server_response_decrement(replica);
      }
    }
  }

  return rc;
}
