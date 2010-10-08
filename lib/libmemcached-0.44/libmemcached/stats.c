/*
*/

#include "common.h"

static const char *memcached_stat_keys[] = {
  "pid",
  "uptime",
  "time",
  "version",
  "pointer_size",
  "rusage_user",
  "rusage_system",
  "curr_items",
  "total_items",
  "bytes",
  "curr_connections",
  "total_connections",
  "connection_structures",
  "cmd_get",
  "cmd_set",
  "get_hits",
  "get_misses",
  "evictions",
  "bytes_read",
  "bytes_written",
  "limit_maxbytes",
  "threads",
  NULL
};

struct local_context
{
  memcached_stat_fn func;
  void *context;
  const char *args;
};


static memcached_return_t set_data(memcached_stat_st *memc_stat, char *key, char *value)
{

  if (strlen(key) < 1)
  {
    WATCHPOINT_STRING(key);
    return MEMCACHED_UNKNOWN_STAT_KEY;
  }
  else if (!strcmp("pid", key))
  {
    memc_stat->pid= (uint32_t) strtol(value, (char **)NULL, 10);
  }
  else if (!strcmp("uptime", key))
  {
    memc_stat->uptime= (uint32_t) strtol(value, (char **)NULL, 10);
  }
  else if (!strcmp("time", key))
  {
    memc_stat->time= (uint32_t) strtol(value, (char **)NULL, 10);
  }
  else if (!strcmp("version", key))
  {
    memcpy(memc_stat->version, value, strlen(value));
    memc_stat->version[strlen(value)]= 0;
  }
  else if (!strcmp("pointer_size", key))
  {
    memc_stat->pointer_size= (uint32_t) strtol(value, (char **)NULL, 10);
  }
  else if (!strcmp("rusage_user", key))
  {
    char *walk_ptr;
    for (walk_ptr= value; (!ispunct(*walk_ptr)); walk_ptr++);
    *walk_ptr= 0;
    walk_ptr++;
    memc_stat->rusage_user_seconds= (uint32_t) strtol(value, (char **)NULL, 10);
    memc_stat->rusage_user_microseconds= (uint32_t) strtol(walk_ptr, (char **)NULL, 10);
  }
  else if (!strcmp("rusage_system", key))
  {
    char *walk_ptr;
    for (walk_ptr= value; (!ispunct(*walk_ptr)); walk_ptr++);
    *walk_ptr= 0;
    walk_ptr++;
    memc_stat->rusage_system_seconds= (uint32_t) strtol(value, (char **)NULL, 10);
    memc_stat->rusage_system_microseconds= (uint32_t) strtol(walk_ptr, (char **)NULL, 10);
  }
  else if (!strcmp("curr_items", key))
  {
    memc_stat->curr_items= (uint32_t) strtol(value, (char **)NULL, 10);
  }
  else if (!strcmp("total_items", key))
  {
    memc_stat->total_items= (uint32_t) strtol(value, (char **)NULL, 10);
  }
  else if (!strcmp("bytes_read", key))
  {
    memc_stat->bytes_read= (uint32_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("bytes_written", key))
  {
    memc_stat->bytes_written= (uint32_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("bytes", key))
  {
    memc_stat->bytes= (uint32_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("curr_connections", key))
  {
    memc_stat->curr_connections= (uint32_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("total_connections", key))
  {
    memc_stat->total_connections= (uint32_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("connection_structures", key))
  {
    memc_stat->connection_structures= (uint32_t) strtol(value, (char **)NULL, 10);
  }
  else if (!strcmp("cmd_get", key))
  {
    memc_stat->cmd_get= (uint64_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("cmd_set", key))
  {
    memc_stat->cmd_set= (uint64_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("get_hits", key))
  {
    memc_stat->get_hits= (uint64_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("get_misses", key))
  {
    memc_stat->get_misses= (uint64_t)strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("evictions", key))
  {
    memc_stat->evictions= (uint64_t)strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("limit_maxbytes", key))
  {
    memc_stat->limit_maxbytes= (uint64_t) strtoll(value, (char **)NULL, 10);
  }
  else if (!strcmp("threads", key))
  {
    memc_stat->threads= (uint32_t) strtol(value, (char **)NULL, 10);
  }
  else if (!(strcmp("delete_misses", key) == 0 ||/* New stats in the 1.3 beta */
             strcmp("delete_hits", key) == 0 ||/* Just swallow them for now.. */
             strcmp("incr_misses", key) == 0 ||
             strcmp("incr_hits", key) == 0 ||
             strcmp("decr_misses", key) == 0 ||
             strcmp("decr_hits", key) == 0 ||
             strcmp("cas_misses", key) == 0 ||
             strcmp("cas_hits", key) == 0 ||
             strcmp("cas_badval", key) == 0 ||
             strcmp("cmd_flush", key) == 0 ||
             strcmp("accepting_conns", key) == 0 ||
             strcmp("listen_disabled_num", key) == 0 ||
             strcmp("conn_yields", key) == 0 ||
             strcmp("auth_cmds", key) == 0 ||
             strcmp("auth_errors", key) == 0 ||
             strcmp("reclaimed", key) == 0))
  {
    WATCHPOINT_STRING(key);
    return MEMCACHED_UNKNOWN_STAT_KEY;
  }

  return MEMCACHED_SUCCESS;
}

char *memcached_stat_get_value(const memcached_st *ptr, memcached_stat_st *memc_stat,
                               const char *key, memcached_return_t *error)
{
  char buffer[SMALL_STRING_LEN];
  int length;
  char *ret;

  *error= MEMCACHED_SUCCESS;

  if (!memcmp("pid", key, strlen("pid")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->pid);
  else if (!memcmp("uptime", key, strlen("uptime")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->uptime);
  else if (!memcmp("time", key, strlen("time")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->time);
  else if (!memcmp("version", key, strlen("version")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%s", memc_stat->version);
  else if (!memcmp("pointer_size", key, strlen("pointer_size")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->pointer_size);
  else if (!memcmp("rusage_user", key, strlen("rusage_user")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u.%u", memc_stat->rusage_user_seconds, memc_stat->rusage_user_microseconds);
  else if (!memcmp("rusage_system", key, strlen("rusage_system")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u.%u", memc_stat->rusage_system_seconds, memc_stat->rusage_system_microseconds);
  else if (!memcmp("curr_items", key, strlen("curr_items")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->curr_items);
  else if (!memcmp("total_items", key, strlen("total_items")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->total_items);
  else if (!memcmp("curr_connections", key, strlen("curr_connections")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->curr_connections);
  else if (!memcmp("total_connections", key, strlen("total_connections")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->total_connections);
  else if (!memcmp("connection_structures", key, strlen("connection_structures")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->connection_structures);
  else if (!memcmp("cmd_get", key, strlen("cmd_get")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->cmd_get);
  else if (!memcmp("cmd_set", key, strlen("cmd_set")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->cmd_set);
  else if (!memcmp("get_hits", key, strlen("get_hits")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->get_hits);
  else if (!memcmp("get_misses", key, strlen("get_misses")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->get_misses);
  else if (!memcmp("evictions", key, strlen("evictions")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->evictions);
  else if (!memcmp("bytes_read", key, strlen("bytes_read")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->bytes_read);
  else if (!memcmp("bytes_written", key, strlen("bytes_written")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->bytes_written);
  else if (!memcmp("bytes", key, strlen("bytes")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->bytes);
  else if (!memcmp("limit_maxbytes", key, strlen("limit_maxbytes")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->limit_maxbytes);
  else if (!memcmp("threads", key, strlen("threads")))
    length= snprintf(buffer, SMALL_STRING_LEN,"%u", memc_stat->threads);
  else
  {
    *error= MEMCACHED_NOTFOUND;
    return NULL;
  }

  ret= libmemcached_malloc(ptr, (size_t) (length + 1));
  memcpy(ret, buffer, (size_t) length);
  ret[length]= '\0';

  return ret;
}

static memcached_return_t binary_stats_fetch(memcached_stat_st *memc_stat,
                                             const char *args,
                                             memcached_server_write_instance_st instance,
                                             struct local_context *check)
{
  memcached_return_t rc;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  protocol_binary_request_stats request= {.bytes= {0}};
  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_STAT;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;

  if (args != NULL)
  {
    size_t len= strlen(args);

    rc= memcached_validate_key_length(len, true);
    unlikely (rc != MEMCACHED_SUCCESS)
      return rc;

    request.message.header.request.keylen= htons((uint16_t)len);
    request.message.header.request.bodylen= htonl((uint32_t) len);

    struct libmemcached_io_vector_st vector[]=
    {
      { .length= sizeof(request.bytes), .buffer= request.bytes },
      { .length= len, .buffer= args }
    };

    if (memcached_vdo(instance, vector, 2, true) != MEMCACHED_SUCCESS)
    {
      memcached_io_reset(instance);
      return MEMCACHED_WRITE_FAILURE;
    }
  }
  else
  {
    if (memcached_do(instance, request.bytes,
                     sizeof(request.bytes), true) != MEMCACHED_SUCCESS)
    {
      memcached_io_reset(instance);
      return MEMCACHED_WRITE_FAILURE;
    }
  }

  memcached_server_response_decrement(instance);
  do
  {
    rc= memcached_response(instance, buffer, sizeof(buffer), NULL);

    if (rc == MEMCACHED_END)
      break;

    unlikely (rc != MEMCACHED_SUCCESS)
    {
      memcached_io_reset(instance);
      return rc;
    }

    if (memc_stat)
    {
      unlikely((set_data(memc_stat, buffer, buffer + strlen(buffer) + 1)) == MEMCACHED_UNKNOWN_STAT_KEY)
      {
        WATCHPOINT_ERROR(MEMCACHED_UNKNOWN_STAT_KEY);
        WATCHPOINT_ASSERT(0);
      }
    }

    if (check && check->func)
    {
      size_t key_length= strlen(buffer);

      check->func(instance,
                  buffer, key_length,
                  buffer+key_length+1, strlen(buffer+key_length+1),
                  check->context);
    }
  } while (1);

  /* shit... memcached_response will decrement the counter, so I need to
   ** reset it.. todo: look at this and try to find a better solution.
 */
  instance->cursor_active= 0;

  return MEMCACHED_SUCCESS;
}

static memcached_return_t ascii_stats_fetch(memcached_stat_st *memc_stat,
                                            const char *args,
                                            memcached_server_write_instance_st instance,
                                            struct local_context *check)
{
  memcached_return_t rc;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t send_length;

  if (args)
    send_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                   "stats %s\r\n", args);
  else
    send_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                   "stats\r\n");

  if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
    return MEMCACHED_WRITE_FAILURE;

  rc= memcached_do(instance, buffer, send_length, true);
  if (rc != MEMCACHED_SUCCESS)
    goto error;

  while (1)
  {
    rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

    if (rc == MEMCACHED_STAT)
    {
      char *string_ptr, *end_ptr;
      char *key, *value;

      string_ptr= buffer;
      string_ptr+= 5; /* Move past STAT */
      for (end_ptr= string_ptr; isgraph(*end_ptr); end_ptr++);
      key= string_ptr;
      key[(size_t)(end_ptr-string_ptr)]= 0;

      string_ptr= end_ptr + 1;
      for (end_ptr= string_ptr; !(isspace(*end_ptr)); end_ptr++);
      value= string_ptr;
      value[(size_t)(end_ptr-string_ptr)]= 0;
      string_ptr= end_ptr + 2;
      if (memc_stat)
      {
        unlikely((set_data(memc_stat, key, value)) == MEMCACHED_UNKNOWN_STAT_KEY)
        {
          WATCHPOINT_ERROR(MEMCACHED_UNKNOWN_STAT_KEY);
          WATCHPOINT_ASSERT(0);
        }
      }

      if (check && check->func)
      {
        check->func(instance,
                    key, strlen(key),
                    value, strlen(value),
                    check->context);
      }
    }
    else
      break;
  }

error:
  if (rc == MEMCACHED_END)
    return MEMCACHED_SUCCESS;
  else
    return rc;
}

memcached_stat_st *memcached_stat(memcached_st *ptr, char *args, memcached_return_t *error)
{
  memcached_return_t rc;
  memcached_stat_st *stats;

  unlikely (ptr->flags.use_udp)
  {
    *error= MEMCACHED_NOT_SUPPORTED;
    return NULL;
  }

  stats= libmemcached_calloc(ptr, memcached_server_count(ptr), sizeof(memcached_stat_st));

  if (! stats)
  {
    *error= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  rc= MEMCACHED_SUCCESS;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_return_t temp_return;
    memcached_server_write_instance_st instance;
    memcached_stat_st *stat_instance;

    stat_instance= stats + x;

    stat_instance->root= ptr;

    instance= memcached_server_instance_fetch(ptr, x);

    if (ptr->flags.binary_protocol)
    {
      temp_return= binary_stats_fetch(stat_instance, args, instance, NULL);
    }
    else
    {
      temp_return= ascii_stats_fetch(stat_instance, args, instance, NULL);
    }

    if (temp_return != MEMCACHED_SUCCESS)
      rc= MEMCACHED_SOME_ERRORS;
  }

  *error= rc;
  return stats;
}

memcached_return_t memcached_stat_servername(memcached_stat_st *memc_stat, char *args,
                                             const char *hostname, in_port_t port)
{
  memcached_return_t rc;
  memcached_st memc;
  memcached_st *memc_ptr;
  memcached_server_write_instance_st instance;

  memset(memc_stat, 0, sizeof(memcached_stat_st));

  memc_ptr= memcached_create(&memc);
  WATCHPOINT_ASSERT(memc_ptr);

  memcached_server_add(&memc, hostname, port);

  instance= memcached_server_instance_fetch(memc_ptr, 0);

  if (memc.flags.binary_protocol)
  {
    rc= binary_stats_fetch(memc_stat, args, instance, NULL);
  }
  else
  {
    rc= ascii_stats_fetch(memc_stat, args, instance, NULL);
  }

  memcached_free(&memc);

  return rc;
}

/*
  We make a copy of the keys since at some point in the not so distant future
  we will add support for "found" keys.
*/
char ** memcached_stat_get_keys(const memcached_st *ptr,
                                memcached_stat_st *memc_stat,
                                memcached_return_t *error)
{
  char **list;
  size_t length= sizeof(memcached_stat_keys);

  (void)memc_stat;

  list= libmemcached_malloc(ptr, length);

  if (! list)
  {
    *error= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  memcpy(list, memcached_stat_keys, sizeof(memcached_stat_keys));

  *error= MEMCACHED_SUCCESS;

  return list;
}

void memcached_stat_free(const memcached_st *ptr, memcached_stat_st *memc_stat)
{
  if (memc_stat == NULL)
  {
    WATCHPOINT_ASSERT(0); /* Be polite, but when debugging catch this as an error */
    return;
  }

  if (memc_stat->root)
  {
    libmemcached_free(memc_stat->root, memc_stat);
  }
  else if (ptr)
  {
    libmemcached_free(ptr, memc_stat);
  }
  else
  {
    free(memc_stat);
  }
}

static memcached_return_t call_stat_fn(memcached_st *ptr,
                                       memcached_server_write_instance_st instance,
                                       void *context)
{
  memcached_return_t rc;
  struct local_context *check= (struct local_context *)context;

  if (ptr->flags.binary_protocol)
  {
    rc= binary_stats_fetch(NULL, check->args, instance, check);
  }
  else
  {
    rc= ascii_stats_fetch(NULL, check->args, instance, check);
  }

  return rc;
}

memcached_return_t memcached_stat_execute(memcached_st *memc, const char *args,  memcached_stat_fn func, void *context)
{
  memcached_version(memc);

 struct local_context check= { .func= func, .context= context, .args= args };

 return memcached_server_execute(memc, call_stat_fn, (void *)&check);
}
