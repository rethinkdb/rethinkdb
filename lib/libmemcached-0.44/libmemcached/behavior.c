/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Change the behavior of the memcached connection.
 *
 */

#include "common.h"
#include <time.h>
#include <sys/types.h>

static bool set_flag(uint64_t data)
{
  // Wordy :)
  return data ? true : false;
}

/*
  This function is used to modify the behavior of running client.

  We quit all connections so we can reset the sockets.
*/

memcached_return_t memcached_behavior_set(memcached_st *ptr,
                                          const memcached_behavior_t flag,
                                          uint64_t data)
{
  switch (flag)
  {
  case MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS:
    ptr->number_of_replicas= (uint32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK:
    ptr->io_msg_watermark= (uint32_t) data;
    break;
  case MEMCACHED_BEHAVIOR_IO_BYTES_WATERMARK:
    ptr->io_bytes_watermark= (uint32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_IO_KEY_PREFETCH:
    ptr->io_key_prefetch = (uint32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_SND_TIMEOUT:
    ptr->snd_timeout= (int32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_RCV_TIMEOUT:
    ptr->rcv_timeout= (int32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT:
    ptr->server_failure_limit= (uint32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_BINARY_PROTOCOL:
    memcached_quit(ptr); // We need t shutdown all of the connections to make sure we do the correct protocol
    if (data)
    {
      ptr->flags.verify_key= false;
    }
    ptr->flags.binary_protocol= set_flag(data);
    break;
  case MEMCACHED_BEHAVIOR_SUPPORT_CAS:
    ptr->flags.support_cas= set_flag(data);
    break;
  case MEMCACHED_BEHAVIOR_NO_BLOCK:
    ptr->flags.no_block= set_flag(data);
    memcached_quit(ptr);
    break;
  case MEMCACHED_BEHAVIOR_BUFFER_REQUESTS:
    ptr->flags.buffer_requests= set_flag(data);
    memcached_quit(ptr);
    break;
  case MEMCACHED_BEHAVIOR_USE_UDP:
    if (memcached_server_count(ptr))
    {
      return MEMCACHED_FAILURE;
    }
    ptr->flags.use_udp= set_flag(data);
    if (data)
    {
      ptr->flags.no_reply= set_flag(data);
    }
    break;
  case MEMCACHED_BEHAVIOR_TCP_NODELAY:
    ptr->flags.tcp_nodelay= set_flag(data);
    memcached_quit(ptr);
    break;
  case MEMCACHED_BEHAVIOR_TCP_KEEPALIVE:
    ptr->flags.tcp_keepalive= set_flag(data);
    memcached_quit(ptr);
    break;
  case MEMCACHED_BEHAVIOR_DISTRIBUTION:
    return memcached_behavior_set_distribution(ptr, (memcached_server_distribution_t)data);
  case MEMCACHED_BEHAVIOR_KETAMA:
    {
      if (data)
      {
        (void)memcached_behavior_set_key_hash(ptr, MEMCACHED_HASH_MD5);
        (void)memcached_behavior_set_distribution_hash(ptr, MEMCACHED_HASH_MD5);
        (void)memcached_behavior_set_distribution(ptr, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA);
      }
      else
      {
        (void)memcached_behavior_set_key_hash(ptr, MEMCACHED_HASH_DEFAULT);
        (void)memcached_behavior_set_distribution_hash(ptr, MEMCACHED_HASH_DEFAULT);
        (void)memcached_behavior_set_distribution(ptr, MEMCACHED_DISTRIBUTION_MODULA);
      }

      break;
    }
  case MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED:
    {
      (void)memcached_behavior_set_key_hash(ptr, MEMCACHED_HASH_MD5);
      (void)memcached_behavior_set_distribution_hash(ptr, MEMCACHED_HASH_MD5);
      ptr->flags.ketama_weighted= set_flag(data);
      /**
        @note We try to keep the same distribution going. This should be deprecated and rewritten.
      */
      return memcached_behavior_set_distribution(ptr, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA);
    }
  case MEMCACHED_BEHAVIOR_HASH:
    return memcached_behavior_set_key_hash(ptr, (memcached_hash_t)(data));
  case MEMCACHED_BEHAVIOR_KETAMA_HASH:
    return memcached_behavior_set_distribution_hash(ptr, (memcached_hash_t)(data));
  case MEMCACHED_BEHAVIOR_CACHE_LOOKUPS:
    ptr->flags.use_cache_lookups= set_flag(data);
    memcached_quit(ptr);
    break;
  case MEMCACHED_BEHAVIOR_VERIFY_KEY:
    if (ptr->flags.binary_protocol)
      return MEMCACHED_FAILURE;
    ptr->flags.verify_key= set_flag(data);
    break;
  case MEMCACHED_BEHAVIOR_SORT_HOSTS:
    {
      ptr->flags.use_sort_hosts= set_flag(data);
      run_distribution(ptr);

      break;
    }
  case MEMCACHED_BEHAVIOR_POLL_TIMEOUT:
    ptr->poll_timeout= (int32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT:
    ptr->connect_timeout= (int32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_RETRY_TIMEOUT:
    ptr->retry_timeout= (int32_t)data;
    break;
  case MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE:
    ptr->send_size= (int32_t)data;
    memcached_quit(ptr);
    break;
  case MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE:
    ptr->recv_size= (int32_t)data;
    memcached_quit(ptr);
    break;
  case MEMCACHED_BEHAVIOR_TCP_KEEPIDLE:
    ptr->tcp_keepidle= (uint32_t)data;
    memcached_quit(ptr);
    break;
  case MEMCACHED_BEHAVIOR_USER_DATA:
    return MEMCACHED_FAILURE;
  case MEMCACHED_BEHAVIOR_HASH_WITH_PREFIX_KEY:
    ptr->flags.hash_with_prefix_key= set_flag(data);
    break;
  case MEMCACHED_BEHAVIOR_NOREPLY:
    ptr->flags.no_reply= set_flag(data);
    break;
  case MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS:
    ptr->flags.auto_eject_hosts= set_flag(data);
    break;
  case MEMCACHED_BEHAVIOR_RANDOMIZE_REPLICA_READ:
      srandom((uint32_t) time(NULL));
      ptr->flags.randomize_replica_read= set_flag(data);
      break;
  case MEMCACHED_BEHAVIOR_CORK:
      {
        memcached_server_write_instance_st instance;
        bool action= set_flag(data);

        if (action == false)
        {
          ptr->flags.cork= set_flag(false);
          return MEMCACHED_SUCCESS;
        }

        instance= memcached_server_instance_fetch(ptr, 0);
        if (! instance)
          return MEMCACHED_NO_SERVERS;


        /* We just try the first host, and if it is down we return zero */
        memcached_return_t rc;
        rc= memcached_connect(instance);
        if (rc != MEMCACHED_SUCCESS)
        {
          return rc;
        }

        /* Now we test! */
        memcached_ternary_t enabled;
        enabled= test_cork(instance, true);

        switch (enabled)
        {
        case MEM_FALSE:
          return ptr->cached_errno ? MEMCACHED_ERRNO : MEMCACHED_FAILURE ;
        case MEM_TRUE:
          {
            enabled= test_cork(instance, false);

            if (enabled == false) // Possible bug in OS?
            {
              memcached_quit_server(instance, false); // We should reset everything on this error.
              return MEMCACHED_ERRNO;  // Errno will be true because we will have already set it.
            }
            ptr->flags.cork= true;
            ptr->flags.tcp_nodelay= true;
            memcached_quit(ptr); // We go on and reset the connections.
          }
          break;
        case MEM_NOT:
        default:
          return MEMCACHED_NOT_SUPPORTED;
        }
      }
      break;
  case MEMCACHED_BEHAVIOR_MAX:
  default:
    /* Shouldn't get here */
    WATCHPOINT_ASSERT(0);
    return MEMCACHED_FAILURE;
  }

  return MEMCACHED_SUCCESS;
}

bool _is_auto_eject_host(const memcached_st *ptr)
{
  return ptr->flags.auto_eject_hosts;
}

uint64_t memcached_behavior_get(memcached_st *ptr,
                                const memcached_behavior_t flag)
{
  switch (flag)
  {
  case MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS:
    return ptr->number_of_replicas;
  case MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK:
    return ptr->io_msg_watermark;
  case MEMCACHED_BEHAVIOR_IO_BYTES_WATERMARK:
    return ptr->io_bytes_watermark;
  case MEMCACHED_BEHAVIOR_IO_KEY_PREFETCH:
    return ptr->io_key_prefetch;
  case MEMCACHED_BEHAVIOR_BINARY_PROTOCOL:
    return ptr->flags.binary_protocol;
  case MEMCACHED_BEHAVIOR_SUPPORT_CAS:
    return ptr->flags.support_cas;
  case MEMCACHED_BEHAVIOR_CACHE_LOOKUPS:
    return ptr->flags.use_cache_lookups;
  case MEMCACHED_BEHAVIOR_NO_BLOCK:
    return ptr->flags.no_block;
  case MEMCACHED_BEHAVIOR_BUFFER_REQUESTS:
    return ptr->flags.buffer_requests;
  case MEMCACHED_BEHAVIOR_USE_UDP:
    return ptr->flags.use_udp;
  case MEMCACHED_BEHAVIOR_TCP_NODELAY:
    return ptr->flags.tcp_nodelay;
  case MEMCACHED_BEHAVIOR_VERIFY_KEY:
    return ptr->flags.verify_key;
  case MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED:
    return ptr->flags.ketama_weighted;
  case MEMCACHED_BEHAVIOR_DISTRIBUTION:
    return ptr->distribution;
  case MEMCACHED_BEHAVIOR_KETAMA:
    return (ptr->distribution == MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA) ? (uint64_t) 1 : 0;
  case MEMCACHED_BEHAVIOR_HASH:
    return hashkit_get_function(&ptr->hashkit);
  case MEMCACHED_BEHAVIOR_KETAMA_HASH:
    return hashkit_get_function(&ptr->distribution_hashkit);
  case MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT:
    return ptr->server_failure_limit;
  case MEMCACHED_BEHAVIOR_SORT_HOSTS:
    return ptr->flags.use_sort_hosts;
  case MEMCACHED_BEHAVIOR_POLL_TIMEOUT:
    return (uint64_t)ptr->poll_timeout;
  case MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT:
    return (uint64_t)ptr->connect_timeout;
  case MEMCACHED_BEHAVIOR_RETRY_TIMEOUT:
    return (uint64_t)ptr->retry_timeout;
  case MEMCACHED_BEHAVIOR_SND_TIMEOUT:
    return (uint64_t)ptr->snd_timeout;
  case MEMCACHED_BEHAVIOR_RCV_TIMEOUT:
    return (uint64_t)ptr->rcv_timeout;
  case MEMCACHED_BEHAVIOR_TCP_KEEPIDLE:
    return (uint64_t)ptr->tcp_keepidle;
  case MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE:
    {
      int sock_size= 0;
      socklen_t sock_length= sizeof(int);
      memcached_server_write_instance_st instance;

      if (ptr->send_size != -1) // If value is -1 then we are using the default
        return (uint64_t) ptr->send_size;

      instance= memcached_server_instance_fetch(ptr, 0);

      if (instance) // If we have an instance we test, otherwise we just set and pray
      {
        /* REFACTOR */
        /* We just try the first host, and if it is down we return zero */
        if ((memcached_connect(instance)) != MEMCACHED_SUCCESS)
          return 0;

        if (getsockopt(instance->fd, SOL_SOCKET,
                       SO_SNDBUF, &sock_size, &sock_length))
          return 0; /* Zero means error */
      }

      return (uint64_t) sock_size;
    }
  case MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE:
    {
      int sock_size= 0;
      socklen_t sock_length= sizeof(int);
      memcached_server_write_instance_st instance;

      if (ptr->recv_size != -1) // If value is -1 then we are using the default
        return (uint64_t) ptr->recv_size;

      instance= memcached_server_instance_fetch(ptr, 0);

      /**
        @note REFACTOR
      */
      if (instance)
      {
        /* We just try the first host, and if it is down we return zero */
        if ((memcached_connect(instance)) != MEMCACHED_SUCCESS)
          return 0;

        if (getsockopt(instance->fd, SOL_SOCKET,
                       SO_RCVBUF, &sock_size, &sock_length))
          return 0; /* Zero means error */

      }

      return (uint64_t) sock_size;
    }
  case MEMCACHED_BEHAVIOR_USER_DATA:
    return MEMCACHED_FAILURE;
  case MEMCACHED_BEHAVIOR_HASH_WITH_PREFIX_KEY:
    return ptr->flags.hash_with_prefix_key;
  case MEMCACHED_BEHAVIOR_NOREPLY:
    return ptr->flags.no_reply;
  case MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS:
    return ptr->flags.auto_eject_hosts;
  case MEMCACHED_BEHAVIOR_RANDOMIZE_REPLICA_READ:
    return ptr->flags.randomize_replica_read;
  case MEMCACHED_BEHAVIOR_CORK:
    return ptr->flags.cork;
  case MEMCACHED_BEHAVIOR_TCP_KEEPALIVE:
    return ptr->flags.tcp_keepalive;
  case MEMCACHED_BEHAVIOR_MAX:
  default:
    WATCHPOINT_ASSERT(0); /* Programming mistake if it gets this far */
    return 0;
  }

  /* NOTREACHED */
}


memcached_return_t memcached_behavior_set_distribution(memcached_st *ptr, memcached_server_distribution_t type)
{
  if (type < MEMCACHED_DISTRIBUTION_CONSISTENT_MAX)
  {
    ptr->distribution= type;
    run_distribution(ptr);
  }
  else
  {
    return MEMCACHED_FAILURE;
  }

  return MEMCACHED_SUCCESS;
}


memcached_server_distribution_t memcached_behavior_get_distribution(memcached_st *ptr)
{
  return ptr->distribution;
}

memcached_return_t memcached_behavior_set_key_hash(memcached_st *ptr, memcached_hash_t type)
{
  hashkit_return_t rc;
  rc= hashkit_set_function(&ptr->hashkit, (hashkit_hash_algorithm_t)type);

  return rc == HASHKIT_SUCCESS ? MEMCACHED_SUCCESS : MEMCACHED_FAILURE;
}

memcached_hash_t memcached_behavior_get_key_hash(memcached_st *ptr)
{
  return (memcached_hash_t)hashkit_get_function(&ptr->hashkit);
}

memcached_return_t memcached_behavior_set_distribution_hash(memcached_st *ptr, memcached_hash_t type)
{
  hashkit_return_t rc;
  rc= hashkit_set_function(&ptr->distribution_hashkit, (hashkit_hash_algorithm_t)type);

  return rc == HASHKIT_SUCCESS ? MEMCACHED_SUCCESS : MEMCACHED_FAILURE;
}

memcached_hash_t memcached_behavior_get_distribution_hash(memcached_st *ptr)
{
  return (memcached_hash_t)hashkit_get_function(&ptr->distribution_hashkit);
}
