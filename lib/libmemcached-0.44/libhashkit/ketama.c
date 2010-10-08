/* HashKit
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "common.h"
#include <math.h>

#if 0
static uint32_t ketama_server_hash(const char *key, unsigned int key_length, int alignment)
{
  unsigned char results[16];

  md5_signature((unsigned char*)key, key_length, results);
  return ((uint32_t) (results[3 + alignment * 4] & 0xFF) << 24)
    | ((uint32_t) (results[2 + alignment * 4] & 0xFF) << 16)
    | ((uint32_t) (results[1 + alignment * 4] & 0xFF) << 8)
    | (results[0 + alignment * 4] & 0xFF);
}

static int continuum_points_cmp(const void *t1, const void *t2)
{
  hashkit_continuum_point_st *ct1= (hashkit_continuum_point_st *)t1;
  hashkit_continuum_point_st *ct2= (hashkit_continuum_point_st *)t2;

  if (ct1->value == ct2->value)
    return 0;
  else if (ct1->value > ct2->value)
    return 1;
  else
    return -1;
}

int update_continuum(hashkit_st *hashkit)
{
  uint32_t count;
  uint32_t continuum_index= 0;
  uint32_t value;
  uint32_t points_index;
  uint32_t points_count= 0;
  uint32_t points_per_server;
  uint32_t points_per_hash;
  uint64_t total_weight= 0;
  uint32_t live_servers;
  uint8_t *context;

  if (hashkit->active_fn != NULL || hashkit->weight_fn != NULL)
  {
    live_servers= 0;

    for (count= 0, context= hashkit->list; count < hashkit->list_size;
         count++, context+= hashkit->context_size)
    {
      if (hashkit->active_fn != NULL)
      {
        if (hashkit->active_fn(context))
          live_servers++;
        else
          continue;
      }

      if (hashkit->weight_fn != NULL)
        total_weight+= hashkit->weight_fn(context);
    }
  }

  if (hashkit->active_fn == NULL)
    live_servers= (uint32_t)hashkit->list_size;

  if (live_servers == 0)
    return 0;

  if (hashkit->weight_fn == NULL)
  {
    points_per_server= HASHKIT_POINTS_PER_NODE;
    points_per_hash= 1;
  }
  else
  {
    points_per_server= HASHKIT_POINTS_PER_NODE_WEIGHTED;
    points_per_hash= 4;
  }

  if (live_servers > hashkit->continuum_count)
  {
    hashkit_continuum_point_st *new_continuum;

    new_continuum= realloc(hashkit->continuum,
                           sizeof(hashkit_continuum_point_st) *
                           (live_servers + HASHKIT_CONTINUUM_ADDITION) *
                           points_per_server);

    if (new_continuum == NULL)
      return ENOMEM;

    hashkit->continuum= new_continuum;
    hashkit->continuum_count= live_servers + HASHKIT_CONTINUUM_ADDITION;
  }

  for (count= 0, context= hashkit->list; count < hashkit->list_size;
       count++, context+= hashkit->context_size)
  {
    if (hashkit->active_fn != NULL && hashkit->active_fn(context) == false)
      continue;

    if (hashkit->weight_fn != NULL)
    {
        float pct = (float)hashkit->weight_fn(context) / (float)total_weight;
        points_per_server= (uint32_t) ((floorf((float) (pct * HASHKIT_POINTS_PER_NODE_WEIGHTED / 4 * (float)live_servers + 0.0000000001))) * 4);
    }

    for (points_index= 0;
         points_index < points_per_server / points_per_hash;
         points_index++)
    {
      char sort_host[HASHKIT_CONTINUUM_KEY_SIZE]= "";
      size_t sort_host_length;

      if (hashkit->continuum_key_fn == NULL)
      {
        sort_host_length= (size_t) snprintf(sort_host, HASHKIT_CONTINUUM_KEY_SIZE, "%u",
                                            points_index);
      }
      else
      {
        sort_host_length= hashkit->continuum_key_fn(sort_host, HASHKIT_CONTINUUM_KEY_SIZE,
                                                 points_index, context);
      }

      if (hashkit->weight_fn == NULL)
      {
        if (hashkit->continuum_hash_fn == NULL)
          value= hashkit_default(sort_host, sort_host_length);
        else
          value= hashkit->continuum_hash_fn(sort_host, sort_host_length);

        hashkit->continuum[continuum_index].index= count;
        hashkit->continuum[continuum_index++].value= value;
      }
      else
      {
        unsigned int i;
        for (i = 0; i < points_per_hash; i++)
        {
           value= ketama_server_hash(sort_host, (uint32_t) sort_host_length, (int) i);
           hashkit->continuum[continuum_index].index= count;
           hashkit->continuum[continuum_index++].value= value;
        }
      }
    }

    points_count+= points_per_server;
  }

  hashkit->continuum_points_count= points_count;
  qsort(hashkit->continuum, hashkit->continuum_points_count, sizeof(hashkit_continuum_point_st),
        continuum_points_cmp);

  return 0;
}
#endif
