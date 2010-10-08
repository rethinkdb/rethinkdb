#include "common.h"

struct context_st 
{
  size_t length;
  const char *buffer;
};

static memcached_return_t _set_verbosity(const memcached_st *ptr __attribute__((unused)),
                                         const memcached_server_st *server,
                                         void *context)
{
  memcached_return_t rc;
  memcached_st local_memc;
  memcached_st *memc_ptr;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

  struct context_st *execute= (struct context_st *)context;

  memc_ptr= memcached_create(&local_memc);

  rc= memcached_server_add(memc_ptr, memcached_server_name(server), memcached_server_port(server));

  if (rc == MEMCACHED_SUCCESS)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(memc_ptr, 0);

    rc= memcached_do(instance, execute->buffer, execute->length, true);

    if (rc == MEMCACHED_SUCCESS)
    {
      rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
    }
  }

  memcached_free(memc_ptr);

  return rc;
}

memcached_return_t memcached_verbosity(memcached_st *ptr, uint32_t verbosity)
{
  size_t send_length;
  memcached_server_fn callbacks[1];

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

  send_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, 
                                 "verbosity %u\r\n", verbosity);
  unlikely (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
    return MEMCACHED_WRITE_FAILURE;

  struct context_st context = { .length= send_length, .buffer= buffer };

  callbacks[0]= _set_verbosity;

  return memcached_server_cursor(ptr, callbacks, &context, 1);
}
