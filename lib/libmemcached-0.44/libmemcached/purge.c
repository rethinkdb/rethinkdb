#include "common.h"

memcached_return_t memcached_purge(memcached_server_write_instance_st ptr)
{
  uint32_t x;
  memcached_return_t ret= MEMCACHED_SUCCESS;
  memcached_st *root= (memcached_st *)ptr->root;

  if (memcached_is_purging(ptr->root) || /* already purging */
      (memcached_server_response_count(ptr) < ptr->root->io_msg_watermark &&
       ptr->io_bytes_sent < ptr->root->io_bytes_watermark) ||
      (ptr->io_bytes_sent >= ptr->root->io_bytes_watermark &&
       memcached_server_response_count(ptr) < 2))
  {
    return MEMCACHED_SUCCESS;
  }

  /* memcached_io_write and memcached_response may call memcached_purge
    so we need to be able stop any recursion.. */
  memcached_set_purging(root, true);

  WATCHPOINT_ASSERT(ptr->fd != -1);
  /* Force a flush of the buffer to ensure that we don't have the n-1 pending
    requests buffered up.. */
  if (memcached_io_write(ptr, NULL, 0, true) == -1)
  {
    memcached_set_purging(root, true);

    return MEMCACHED_WRITE_FAILURE;
  }
  WATCHPOINT_ASSERT(ptr->fd != -1);

  uint32_t no_msg= memcached_server_response_count(ptr) - 1;
  if (no_msg > 0)
  {
    memcached_result_st result;
    memcached_result_st *result_ptr;
    char buffer[SMALL_STRING_LEN];

    /*
     * We need to increase the timeout, because we might be waiting for
     * data to be sent from the server (the commands was in the output buffer
     * and just flushed
   */
    const int32_t timeo= ptr->root->poll_timeout;
    root->poll_timeout= 2000;

    result_ptr= memcached_result_create(root, &result);
    WATCHPOINT_ASSERT(result_ptr);

    for (x= 0; x < no_msg; x++)
    {
      memcached_result_reset(result_ptr);
      memcached_return_t rc= memcached_read_one_response(ptr, buffer,
                                                         sizeof (buffer),
                                                         result_ptr);
      /*
       * Purge doesn't care for what kind of command results that is received.
       * The only kind of errors I care about if is I'm out of sync with the
       * protocol or have problems reading data from the network..
     */
      if (rc== MEMCACHED_PROTOCOL_ERROR || rc == MEMCACHED_UNKNOWN_READ_FAILURE)
      {
        WATCHPOINT_ERROR(rc);
        ret = rc;
        memcached_io_reset(ptr);
      }

      if (ptr->root->callbacks != NULL)
      {
        memcached_callback_st cb = *ptr->root->callbacks;
        if (rc == MEMCACHED_SUCCESS)
        {
          for (uint32_t y= 0; y < cb.number_of_callback; y++)
          {
            rc = (*cb.callback[y])(ptr->root, result_ptr, cb.context);
            if (rc != MEMCACHED_SUCCESS)
              break;
          }
        }
      }
    }

    memcached_result_free(result_ptr);
    root->poll_timeout= timeo;
  }
  memcached_set_purging(root, false);

  return ret;
}
