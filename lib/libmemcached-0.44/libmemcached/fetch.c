#include "common.h"

char *memcached_fetch(memcached_st *ptr, char *key, size_t *key_length, 
                      size_t *value_length, 
                      uint32_t *flags,
                      memcached_return_t *error)
{
  memcached_result_st *result_buffer= &ptr->result;

  unlikely (ptr->flags.use_udp)
  {
    *error= MEMCACHED_NOT_SUPPORTED;
    return NULL;
  }

  result_buffer= memcached_fetch_result(ptr, result_buffer, error);

  if (result_buffer == NULL || *error != MEMCACHED_SUCCESS)
  {
    WATCHPOINT_ASSERT(result_buffer == NULL);
    *value_length= 0;
    return NULL;
  }

  *value_length= memcached_string_length(&result_buffer->value);

  if (key)
  {
    if (result_buffer->key_length > MEMCACHED_MAX_KEY)
    {
      *error= MEMCACHED_KEY_TOO_BIG;
      *value_length= 0;

      return NULL;
    }
    strncpy(key, result_buffer->item_key, result_buffer->key_length); // For the binary protocol we will cut off the key :(
    *key_length= result_buffer->key_length;
  }

  *flags= result_buffer->item_flags;

  return memcached_string_c_copy(&result_buffer->value);
}

memcached_result_st *memcached_fetch_result(memcached_st *ptr,
                                            memcached_result_st *result,
                                            memcached_return_t *error)
{
  memcached_server_st *server;

  unlikely (ptr->flags.use_udp)
  {
    *error= MEMCACHED_NOT_SUPPORTED;
    return NULL;
  }

  if (result == NULL)
    if ((result= memcached_result_create(ptr, NULL)) == NULL)
      return NULL;

  while ((server= memcached_io_get_readable_server(ptr)) != NULL) 
  {
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
    *error= memcached_response(server, buffer, sizeof(buffer), result);

    if (*error == MEMCACHED_SUCCESS)
      return result;
    else if (*error == MEMCACHED_END)
      memcached_server_response_reset(server);
    else if (*error != MEMCACHED_NOTFOUND)
      break;
  }

  /* We have completed reading data */
  if (memcached_is_allocated(result))
  {
    memcached_result_free(result);
  }
  else
  {
    memcached_string_reset(&result->value);
  }

  return NULL;
}

memcached_return_t memcached_fetch_execute(memcached_st *ptr, 
                                           memcached_execute_fn *callback,
                                           void *context,
                                           uint32_t number_of_callbacks)
{
  memcached_result_st *result= &ptr->result;
  memcached_return_t rc= MEMCACHED_FAILURE;

  while ((result= memcached_fetch_result(ptr, result, &rc)) != NULL) 
  {
    if (rc == MEMCACHED_SUCCESS)
    {
      for (uint32_t x= 0; x < number_of_callbacks; x++)
      {
        rc= (*callback[x])(ptr, result, context);
        if (rc != MEMCACHED_SUCCESS)
          break;
      }
    }
  }
  return rc;
}
