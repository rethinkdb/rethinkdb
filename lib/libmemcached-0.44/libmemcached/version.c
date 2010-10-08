#include "common.h"

const char * memcached_lib_version(void) 
{
  return LIBMEMCACHED_VERSION_STRING;
}

static inline memcached_return_t memcached_version_binary(memcached_st *ptr);
static inline memcached_return_t memcached_version_textual(memcached_st *ptr);

memcached_return_t memcached_version(memcached_st *ptr)
{
  if (ptr->flags.use_udp)
    return MEMCACHED_NOT_SUPPORTED;

  bool was_blocking= ptr->flags.no_block;
  memcached_return_t rc;

  ptr->flags.no_block= false;

  if (ptr->flags.binary_protocol)
    rc= memcached_version_binary(ptr);
  else
    rc= memcached_version_textual(ptr);      

  ptr->flags.no_block= was_blocking;

  return rc;
}

static inline memcached_return_t memcached_version_textual(memcached_st *ptr)
{
  size_t send_length;
  memcached_return_t rc;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  char *response_ptr;
  const char *command= "version\r\n";

  send_length= strlen(command);

  rc= MEMCACHED_SUCCESS;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_return_t rrc;
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    rrc= memcached_do(instance, command, send_length, true);
    if (rrc != MEMCACHED_SUCCESS)
    {
      rc= MEMCACHED_SOME_ERRORS;
      continue;
    }

    rrc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
    if (rrc != MEMCACHED_SUCCESS)
    {
      rc= MEMCACHED_SOME_ERRORS;
      continue;
    }

    /* Find the space, and then move one past it to copy version */
    response_ptr= index(buffer, ' ');
    response_ptr++;

    instance->major_version= (uint8_t)strtol(response_ptr, (char **)NULL, 10);
    response_ptr= index(response_ptr, '.');
    response_ptr++;
    instance->minor_version= (uint8_t)strtol(response_ptr, (char **)NULL, 10);
    response_ptr= index(response_ptr, '.');
    response_ptr++;
    instance->micro_version= (uint8_t)strtol(response_ptr, (char **)NULL, 10);
  }

  return rc;
}

static inline memcached_return_t memcached_version_binary(memcached_st *ptr)
{
  memcached_return_t rc;
  protocol_binary_request_version request= { .bytes= {0}};
  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_VERSION;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;

  rc= MEMCACHED_SUCCESS;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++) 
  {
    memcached_return_t rrc;

    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    rrc= memcached_do(instance, request.bytes, sizeof(request.bytes), true);
    if (rrc != MEMCACHED_SUCCESS) 
    {
      memcached_io_reset(instance);
      rc= MEMCACHED_SOME_ERRORS;
      continue;
    }
  }

  for (uint32_t x= 0; x < memcached_server_count(ptr); x++) 
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    if (memcached_server_response_count(instance) > 0) 
    {
      memcached_return_t rrc;
      char buffer[32];
      char *p;

      rrc= memcached_response(instance, buffer, sizeof(buffer), NULL);
      if (rrc != MEMCACHED_SUCCESS) 
      {
        memcached_io_reset(instance);
        rc= MEMCACHED_SOME_ERRORS;
        continue;
      }

      instance->major_version= (uint8_t)strtol(buffer, &p, 10);
      instance->minor_version= (uint8_t)strtol(p + 1, &p, 10);
      instance->micro_version= (uint8_t)strtol(p + 1, NULL, 10);
    }
  }

  return rc;
}
