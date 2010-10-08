/* LibMemcached
 * Copyright (C) 2006-2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: 
 *
 */

#include "common.h"
#include <math.h>

/* Protoypes (static) */
static memcached_return_t server_add(memcached_st *ptr, const char *hostname,
                                     in_port_t port,
                                     uint32_t weight,
                                     memcached_connection_t type);
static memcached_return_t update_continuum(memcached_st *ptr);

static int compare_servers(const void *p1, const void *p2)
{
  int return_value;
  memcached_server_instance_st a= (memcached_server_instance_st)p1;
  memcached_server_instance_st b= (memcached_server_instance_st)p2;

  return_value= strcmp(a->hostname, b->hostname);

  if (return_value == 0)
  {
    return_value= (int) (a->port - b->port);
  }

  return return_value;
}

static void sort_hosts(memcached_st *ptr)
{
  if (memcached_server_count(ptr))
  {
    memcached_server_write_instance_st instance;

    qsort(memcached_server_list(ptr), memcached_server_count(ptr), sizeof(memcached_server_st), compare_servers);
    instance= memcached_server_instance_fetch(ptr, 0);
    instance->number_of_hosts= memcached_server_count(ptr);
  }
}


memcached_return_t run_distribution(memcached_st *ptr)
{
  if (ptr->flags.use_sort_hosts)
    sort_hosts(ptr);

  switch (ptr->distribution)
  {
  case MEMCACHED_DISTRIBUTION_CONSISTENT:
  case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA:
  case MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY:
    return update_continuum(ptr);
  case MEMCACHED_DISTRIBUTION_MODULA:
    break;
  case MEMCACHED_DISTRIBUTION_RANDOM:
    srandom((uint32_t) time(NULL));
    break;
  case MEMCACHED_DISTRIBUTION_CONSISTENT_MAX:
  default:
    WATCHPOINT_ASSERT(0); /* We have added a distribution without extending the logic */
  }

  return MEMCACHED_SUCCESS;
}

static uint32_t ketama_server_hash(const char *key, size_t key_length, uint32_t alignment)
{
  unsigned char results[16];

  libhashkit_md5_signature((unsigned char*)key, key_length, results);

  return ((uint32_t) (results[3 + alignment * 4] & 0xFF) << 24)
    | ((uint32_t) (results[2 + alignment * 4] & 0xFF) << 16)
    | ((uint32_t) (results[1 + alignment * 4] & 0xFF) << 8)
    | (results[0 + alignment * 4] & 0xFF);
}

static int continuum_item_cmp(const void *t1, const void *t2)
{
  memcached_continuum_item_st *ct1= (memcached_continuum_item_st *)t1;
  memcached_continuum_item_st *ct2= (memcached_continuum_item_st *)t2;

  /* Why 153? Hmmm... */
  WATCHPOINT_ASSERT(ct1->value != 153);
  if (ct1->value == ct2->value)
    return 0;
  else if (ct1->value > ct2->value)
    return 1;
  else
    return -1;
}

static memcached_return_t update_continuum(memcached_st *ptr)
{
  uint32_t host_index;
  uint32_t continuum_index= 0;
  uint32_t value;
  memcached_server_st *list;
  uint32_t pointer_index;
  uint32_t pointer_counter= 0;
  uint32_t pointer_per_server= MEMCACHED_POINTS_PER_SERVER;
  uint32_t pointer_per_hash= 1;
  uint64_t total_weight= 0;
  uint64_t is_ketama_weighted= 0;
  uint64_t is_auto_ejecting= 0;
  uint32_t points_per_server= 0;
  uint32_t live_servers= 0;
  struct timeval now;

  if (gettimeofday(&now, NULL) != 0)
  {
    ptr->cached_errno = errno;
    return MEMCACHED_ERRNO;
  }

  list = memcached_server_list(ptr);

  /* count live servers (those without a retry delay set) */
  is_auto_ejecting= _is_auto_eject_host(ptr);
  if (is_auto_ejecting)
  {
    live_servers= 0;
    ptr->next_distribution_rebuild= 0;
    for (host_index= 0; host_index < memcached_server_count(ptr); ++host_index)
    {
      if (list[host_index].next_retry <= now.tv_sec)
        live_servers++;
      else
      {
        if (ptr->next_distribution_rebuild == 0 || list[host_index].next_retry < ptr->next_distribution_rebuild)
          ptr->next_distribution_rebuild= list[host_index].next_retry;
      }
    }
  }
  else
  {
    live_servers= memcached_server_count(ptr);
  }

  is_ketama_weighted= memcached_behavior_get(ptr, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED);
  points_per_server= (uint32_t) (is_ketama_weighted ? MEMCACHED_POINTS_PER_SERVER_KETAMA : MEMCACHED_POINTS_PER_SERVER);

  if (live_servers == 0)
    return MEMCACHED_SUCCESS;

  if (live_servers > ptr->continuum_count)
  {
    memcached_continuum_item_st *new_ptr;

    new_ptr= libmemcached_realloc(ptr, ptr->continuum,
                                  sizeof(memcached_continuum_item_st) * (live_servers + MEMCACHED_CONTINUUM_ADDITION) * points_per_server);

    if (new_ptr == 0)
      return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

    ptr->continuum= new_ptr;
    ptr->continuum_count= live_servers + MEMCACHED_CONTINUUM_ADDITION;
  }

  if (is_ketama_weighted)
  {
    for (host_index = 0; host_index < memcached_server_count(ptr); ++host_index)
    {
      if (list[host_index].weight == 0)
      {
        list[host_index].weight = 1;
      }
      if (! is_auto_ejecting || list[host_index].next_retry <= now.tv_sec)
        total_weight += list[host_index].weight;
    }
  }

  for (host_index= 0; host_index < memcached_server_count(ptr); ++host_index)
  {
    if (is_auto_ejecting && list[host_index].next_retry > now.tv_sec)
      continue;

    if (is_ketama_weighted)
    {
        float pct = (float)list[host_index].weight / (float)total_weight;
        pointer_per_server= (uint32_t) ((floorf((float) (pct * MEMCACHED_POINTS_PER_SERVER_KETAMA / 4 * (float)live_servers + 0.0000000001))) * 4);
        pointer_per_hash= 4;
#ifdef DEBUG
        printf("ketama_weighted:%s|%d|%llu|%u\n",
               list[host_index].hostname,
               list[host_index].port,
               (unsigned long long)list[host_index].weight,
               pointer_per_server);
#endif
    }


    if (ptr->distribution == MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY)
    {
      for (pointer_index= 0;
           pointer_index < pointer_per_server / pointer_per_hash;
           pointer_index++)
      {
        char sort_host[MEMCACHED_MAX_HOST_SORT_LENGTH]= "";
        size_t sort_host_length;

        // Spymemcached ketema key format is: hostname/ip:port-index
        // If hostname is not available then: /ip:port-index
        sort_host_length= (size_t) snprintf(sort_host, MEMCACHED_MAX_HOST_SORT_LENGTH,
                                            "/%s:%u-%u",
                                            list[host_index].hostname,
                                            (uint32_t)list[host_index].port,
                                            pointer_index);
#ifdef DEBUG
        printf("update_continuum: key is %s\n", sort_host);
#endif

        WATCHPOINT_ASSERT(sort_host_length);

        if (is_ketama_weighted)
        {
          for (uint32_t x= 0; x < pointer_per_hash; x++)
          {
             value= ketama_server_hash(sort_host, sort_host_length, x);
             ptr->continuum[continuum_index].index= host_index;
             ptr->continuum[continuum_index++].value= value;
          }
        }
        else
        {
          value= hashkit_digest(&ptr->distribution_hashkit, sort_host, sort_host_length);
          ptr->continuum[continuum_index].index= host_index;
          ptr->continuum[continuum_index++].value= value;
        }
      }
    }
    else
    {
      for (pointer_index= 1;
           pointer_index <= pointer_per_server / pointer_per_hash;
           pointer_index++)
      {
        char sort_host[MEMCACHED_MAX_HOST_SORT_LENGTH]= "";
        size_t sort_host_length;

        if (list[host_index].port == MEMCACHED_DEFAULT_PORT)
        {
          sort_host_length= (size_t) snprintf(sort_host, MEMCACHED_MAX_HOST_SORT_LENGTH,
                                              "%s-%u",
                                              list[host_index].hostname,
                                              pointer_index - 1);
        }
        else
        {
          sort_host_length= (size_t) snprintf(sort_host, MEMCACHED_MAX_HOST_SORT_LENGTH,
                                              "%s:%u-%u",
                                              list[host_index].hostname,
                                              (uint32_t)list[host_index].port,
                                              pointer_index - 1);
        }

        WATCHPOINT_ASSERT(sort_host_length);

        if (is_ketama_weighted)
        {
          for (uint32_t x = 0; x < pointer_per_hash; x++)
          {
             value= ketama_server_hash(sort_host, sort_host_length, x);
             ptr->continuum[continuum_index].index= host_index;
             ptr->continuum[continuum_index++].value= value;
          }
        }
        else
        {
          value= hashkit_digest(&ptr->distribution_hashkit, sort_host, sort_host_length);
          ptr->continuum[continuum_index].index= host_index;
          ptr->continuum[continuum_index++].value= value;
        }
      }
    }

    pointer_counter+= pointer_per_server;
  }

  WATCHPOINT_ASSERT(ptr);
  WATCHPOINT_ASSERT(ptr->continuum);
  WATCHPOINT_ASSERT(memcached_server_count(ptr) * MEMCACHED_POINTS_PER_SERVER <= MEMCACHED_CONTINUUM_SIZE);
  ptr->continuum_points_counter= pointer_counter;
  qsort(ptr->continuum, ptr->continuum_points_counter, sizeof(memcached_continuum_item_st), continuum_item_cmp);

#ifdef DEBUG
  for (pointer_index= 0; memcached_server_count(ptr) && pointer_index < ((live_servers * MEMCACHED_POINTS_PER_SERVER) - 1); pointer_index++)
  {
    WATCHPOINT_ASSERT(ptr->continuum[pointer_index].value <= ptr->continuum[pointer_index + 1].value);
  }
#endif

  return MEMCACHED_SUCCESS;
}


memcached_return_t memcached_server_push(memcached_st *ptr, const memcached_server_list_st list)
{
  uint32_t count;
  memcached_server_st *new_host_list;

  if (! list)
    return MEMCACHED_SUCCESS;

  count= memcached_server_list_count(list);
  new_host_list= libmemcached_realloc(ptr, memcached_server_list(ptr),
                                      sizeof(memcached_server_st) * (count + memcached_server_count(ptr)));

  if (! new_host_list)
    return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

  memcached_server_list_set(ptr, new_host_list);

  for (uint32_t x= 0; x < count; x++)
  {
    memcached_server_write_instance_st instance;

    if ((ptr->flags.use_udp && list[x].type != MEMCACHED_CONNECTION_UDP)
            || ((list[x].type == MEMCACHED_CONNECTION_UDP)
            && ! (ptr->flags.use_udp)) )
      return MEMCACHED_INVALID_HOST_PROTOCOL;

    WATCHPOINT_ASSERT(list[x].hostname[0] != 0);

    instance= memcached_server_instance_fetch(ptr, memcached_server_count(ptr));

    /* TODO check return type */
    (void)memcached_server_create_with(ptr, instance, list[x].hostname,
                                       list[x].port, list[x].weight, list[x].type);
    ptr->number_of_hosts++;
  }

  // Provides backwards compatibility with server list.
  {
    memcached_server_write_instance_st instance;
    instance= memcached_server_instance_fetch(ptr, 0);
    instance->number_of_hosts= memcached_server_count(ptr);
  }

  return run_distribution(ptr);
}

memcached_return_t memcached_server_add_unix_socket(memcached_st *ptr,
                                                    const char *filename)
{
  return memcached_server_add_unix_socket_with_weight(ptr, filename, 0);
}

memcached_return_t memcached_server_add_unix_socket_with_weight(memcached_st *ptr,
                                                                const char *filename,
                                                                uint32_t weight)
{
  if (! filename)
    return MEMCACHED_FAILURE;

  return server_add(ptr, filename, 0, weight, MEMCACHED_CONNECTION_UNIX_SOCKET);
}

memcached_return_t memcached_server_add_udp(memcached_st *ptr,
                                            const char *hostname,
                                            in_port_t port)
{
  return memcached_server_add_udp_with_weight(ptr, hostname, port, 0);
}

memcached_return_t memcached_server_add_udp_with_weight(memcached_st *ptr,
                                                        const char *hostname,
                                                        in_port_t port,
                                                        uint32_t weight)
{
  if (! port)
    port= MEMCACHED_DEFAULT_PORT;

  if (! hostname)
    hostname= "localhost";

  return server_add(ptr, hostname, port, weight, MEMCACHED_CONNECTION_UDP);
}

memcached_return_t memcached_server_add(memcached_st *ptr,
                                        const char *hostname,
                                        in_port_t port)
{
  return memcached_server_add_with_weight(ptr, hostname, port, 0);
}

memcached_return_t memcached_server_add_with_weight(memcached_st *ptr,
                                                    const char *hostname,
                                                    in_port_t port,
                                                    uint32_t weight)
{
  if (! port)
    port= MEMCACHED_DEFAULT_PORT;

  if (! hostname)
    hostname= "localhost";

  return server_add(ptr, hostname, port, weight, MEMCACHED_CONNECTION_TCP);
}

static memcached_return_t server_add(memcached_st *ptr, const char *hostname,
                                     in_port_t port,
                                     uint32_t weight,
                                     memcached_connection_t type)
{
  memcached_server_st *new_host_list;
  memcached_server_write_instance_st instance;

  if ( (ptr->flags.use_udp && type != MEMCACHED_CONNECTION_UDP)
      || ( (type == MEMCACHED_CONNECTION_UDP) && (! ptr->flags.use_udp) ) )
    return MEMCACHED_INVALID_HOST_PROTOCOL;

  new_host_list= libmemcached_realloc(ptr, memcached_server_list(ptr),
                                      sizeof(memcached_server_st) * (ptr->number_of_hosts + 1));

  if (new_host_list == NULL)
    return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

  memcached_server_list_set(ptr, new_host_list);

  /* TODO: Check return type */
  instance= memcached_server_instance_fetch(ptr, memcached_server_count(ptr));
  (void)memcached_server_create_with(ptr, instance, hostname, port, weight, type);
  ptr->number_of_hosts++;

  instance= memcached_server_instance_fetch(ptr, 0);
  memcached_servers_set_count(instance, memcached_server_count(ptr));

  return run_distribution(ptr);
}
