#include "common.h"

memcached_return_t memcached_flush_buffers(memcached_st *memc)
{
  memcached_return_t ret= MEMCACHED_SUCCESS;

  for (uint32_t x= 0; x < memcached_server_count(memc); ++x)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(memc, x);

    if (instance->write_buffer_offset != 0) 
    {
      if (instance->fd == -1 &&
          (ret= memcached_connect(instance)) != MEMCACHED_SUCCESS)
      {
        WATCHPOINT_ERROR(ret);
        return ret;
      }
      if (memcached_io_write(instance, NULL, 0, true) == -1)
        ret= MEMCACHED_SOME_ERRORS;
    }
  }

  return ret;
}
