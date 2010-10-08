/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Methods for adding or decrementing values from an object in memcached
 *
 */

#include "common.h"

static memcached_return_t text_incr_decr(memcached_st *ptr,
                                         const char *verb,
                                         const char *master_key, size_t master_key_length,
                                         const char *key, size_t key_length,
                                         uint64_t offset,
                                         uint64_t *value)
{
  size_t send_length;
  memcached_return_t rc;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  uint32_t server_key;
  memcached_server_write_instance_st instance;
  bool no_reply= ptr->flags.no_reply;

  unlikely (memcached_server_count(ptr) == 0)
    return MEMCACHED_NO_SERVERS;

  if (ptr->flags.verify_key && (memcached_key_test((const char **)&key, &key_length, 1) == MEMCACHED_BAD_KEY_PROVIDED))
    return MEMCACHED_BAD_KEY_PROVIDED;

  server_key= memcached_generate_hash_with_redistribution(ptr, master_key, master_key_length);
  instance= memcached_server_instance_fetch(ptr, server_key);

  send_length= (size_t)snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                "%s %.*s%.*s %" PRIu64 "%s\r\n", verb,
                                (int)ptr->prefix_key_length,
                                ptr->prefix_key,
                                (int)key_length, key,
                                offset, no_reply ? " noreply" : "");
  unlikely (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
    return MEMCACHED_WRITE_FAILURE;

  rc= memcached_do(instance, buffer, send_length, true);
  if (no_reply || rc != MEMCACHED_SUCCESS)
    return rc;

  rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

  /*
    So why recheck responce? Because the protocol is brain dead :)
    The number returned might end up equaling one of the string
    values. Less chance of a mistake with strncmp() so we will
    use it. We still called memcached_response() though since it
    worked its magic for non-blocking IO.
  */
  if (! strncmp(buffer, "ERROR\r\n", 7))
  {
    *value= 0;
    rc= MEMCACHED_PROTOCOL_ERROR;
  }
  else if (! strncmp(buffer, "CLIENT_ERROR\r\n", 14))
  {
    *value= 0;
    rc= MEMCACHED_PROTOCOL_ERROR;
  }
  else if (!strncmp(buffer, "NOT_FOUND\r\n", 11))
  {
    *value= 0;
    rc= MEMCACHED_NOTFOUND;
  }
  else
  {
    *value= strtoull(buffer, (char **)NULL, 10);
    rc= MEMCACHED_SUCCESS;
  }

  return rc;
}

static memcached_return_t binary_incr_decr(memcached_st *ptr, uint8_t cmd,
                                           const char *master_key, size_t master_key_length,
                                           const char *key, size_t key_length,
                                           uint64_t offset, uint64_t initial,
                                           uint32_t expiration,
                                           uint64_t *value)
{
  uint32_t server_key;
  memcached_server_write_instance_st instance;
  bool no_reply= ptr->flags.no_reply;

  unlikely (memcached_server_count(ptr) == 0)
    return MEMCACHED_NO_SERVERS;

  server_key= memcached_generate_hash_with_redistribution(ptr, master_key, master_key_length);
  instance= memcached_server_instance_fetch(ptr, server_key);

  if (no_reply)
  {
    if(cmd == PROTOCOL_BINARY_CMD_DECREMENT)
      cmd= PROTOCOL_BINARY_CMD_DECREMENTQ;
    if(cmd == PROTOCOL_BINARY_CMD_INCREMENT)
      cmd= PROTOCOL_BINARY_CMD_INCREMENTQ;
  }
  protocol_binary_request_incr request= {.bytes= {0}};

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= cmd;
  request.message.header.request.keylen= htons((uint16_t)(key_length + ptr->prefix_key_length));
  request.message.header.request.extlen= 20;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl((uint32_t)(key_length + ptr->prefix_key_length +  request.message.header.request.extlen));
  request.message.body.delta= htonll(offset);
  request.message.body.initial= htonll(initial);
  request.message.body.expiration= htonl((uint32_t) expiration);

  struct libmemcached_io_vector_st vector[]=
  {
    { .length= sizeof(request.bytes), .buffer= request.bytes },
    { .length= ptr->prefix_key_length, .buffer= ptr->prefix_key },
    { .length= key_length, .buffer= key }
  };

  memcached_return_t rc;
  if ((rc= memcached_vdo(instance, vector, 3, true)) != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(instance);
    return (rc == MEMCACHED_SUCCESS) ? MEMCACHED_WRITE_FAILURE : rc;
  }

  if (no_reply)
    return MEMCACHED_SUCCESS;
  return memcached_response(instance, (char*)value, sizeof(*value), NULL);
}

memcached_return_t memcached_increment(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       uint32_t offset,
                                       uint64_t *value)
{
  return memcached_increment_by_key(ptr, key, key_length, key, key_length, offset, value);
}

memcached_return_t memcached_decrement(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       uint32_t offset,
                                       uint64_t *value)
{
  return memcached_decrement_by_key(ptr, key, key_length, key, key_length, offset, value);
}

memcached_return_t memcached_increment_by_key(memcached_st *ptr,
                                              const char *master_key, size_t master_key_length,
                                              const char *key, size_t key_length,
                                              uint64_t offset,
                                              uint64_t *value)
{
  memcached_return_t rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  LIBMEMCACHED_MEMCACHED_INCREMENT_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_INCREMENT,
                         master_key, master_key_length, key, key_length,
                         (uint64_t)offset, 0, MEMCACHED_EXPIRATION_NOT_ADD,
                         value);
  }
  else
  {
     rc= text_incr_decr(ptr, "incr", master_key, master_key_length, key, key_length, offset, value);
  }

  LIBMEMCACHED_MEMCACHED_INCREMENT_END();

  return rc;
}

memcached_return_t memcached_decrement_by_key(memcached_st *ptr,
                                              const char *master_key, size_t master_key_length,
                                              const char *key, size_t key_length,
                                              uint64_t offset,
                                              uint64_t *value)
{
  memcached_return_t rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  LIBMEMCACHED_MEMCACHED_DECREMENT_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_DECREMENT,
                         master_key, master_key_length, key, key_length,
                         (uint64_t)offset, 0, MEMCACHED_EXPIRATION_NOT_ADD,
                         value);
  }
  else
  {
    rc= text_incr_decr(ptr, "decr", master_key, master_key_length, key, key_length, offset, value);
  }

  LIBMEMCACHED_MEMCACHED_DECREMENT_END();

  return rc;
}

memcached_return_t memcached_increment_with_initial(memcached_st *ptr,
                                                    const char *key,
                                                    size_t key_length,
                                                    uint64_t offset,
                                                    uint64_t initial,
                                                    time_t expiration,
                                                    uint64_t *value)
{
  return memcached_increment_with_initial_by_key(ptr, key, key_length,
                                                 key, key_length,
                                                 offset, initial, expiration, value);
}

memcached_return_t memcached_increment_with_initial_by_key(memcached_st *ptr,
                                                         const char *master_key,
                                                         size_t master_key_length,
                                                         const char *key,
                                                         size_t key_length,
                                                         uint64_t offset,
                                                         uint64_t initial,
                                                         time_t expiration,
                                                         uint64_t *value)
{
  memcached_return_t rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  LIBMEMCACHED_MEMCACHED_INCREMENT_WITH_INITIAL_START();
  if (ptr->flags.binary_protocol)
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_INCREMENT,
                         master_key, master_key_length, key, key_length,
                         offset, initial, (uint32_t)expiration,
                         value);
  else
    rc= MEMCACHED_PROTOCOL_ERROR;

  LIBMEMCACHED_MEMCACHED_INCREMENT_WITH_INITIAL_END();

  return rc;
}

memcached_return_t memcached_decrement_with_initial(memcached_st *ptr,
                                                    const char *key,
                                                    size_t key_length,
                                                    uint64_t offset,
                                                    uint64_t initial,
                                                    time_t expiration,
                                                    uint64_t *value)
{
  return memcached_decrement_with_initial_by_key(ptr, key, key_length,
                                                 key, key_length,
                                                 offset, initial, expiration, value);
}

memcached_return_t memcached_decrement_with_initial_by_key(memcached_st *ptr,
                                                           const char *master_key,
                                                           size_t master_key_length,
                                                           const char *key,
                                                           size_t key_length,
                                                           uint64_t offset,
                                                           uint64_t initial,
                                                           time_t expiration,
                                                           uint64_t *value)
{
  memcached_return_t rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  LIBMEMCACHED_MEMCACHED_INCREMENT_WITH_INITIAL_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_DECREMENT,
                         master_key, master_key_length, key, key_length,
                         offset, initial, (uint32_t)expiration,
                         value);
  }
  else
  {
    rc= MEMCACHED_PROTOCOL_ERROR;
  }

  LIBMEMCACHED_MEMCACHED_INCREMENT_WITH_INITIAL_END();

  return rc;
}

