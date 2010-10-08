/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/**
 * This file contains an implementation of the callback interface for level 1
 * in the protocol library. If you compare the implementation with the one
 * in interface_v0.c you will see that this implementation is much easier and
 * hides all of the protocol logic and let you focus on the application
 * logic. One "problem" with this layer is that it is synchronous, so that
 * you will not receive the next command before a answer to the previous
 * command is being sent.
 */
#include "config.h"
#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <libmemcached/protocol_handler.h>
#include <libmemcached/byteorder.h>
#include "storage.h"

static protocol_binary_response_status add_handler(const void *cookie,
                                                   const void *key,
                                                   uint16_t keylen,
                                                   const void *data,
                                                   uint32_t datalen,
                                                   uint32_t flags,
                                                   uint32_t exptime,
                                                   uint64_t *cas)
{
  (void)cookie;
  protocol_binary_response_status rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;
  struct item* item= get_item(key, keylen);
  if (item == NULL)
  {
    item= create_item(key, keylen, data, datalen, flags, (time_t)exptime);
    if (item == 0)
    {
      rval= PROTOCOL_BINARY_RESPONSE_ENOMEM;
    }
    else
    {
      put_item(item);
      *cas= item->cas;
      release_item(item);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS;
  }

  return rval;
}

static protocol_binary_response_status append_handler(const void *cookie,
                                                      const void *key,
                                                      uint16_t keylen,
                                                      const void* val,
                                                      uint32_t vallen,
                                                      uint64_t cas,
                                                      uint64_t *result_cas)
{
  (void)cookie;
  protocol_binary_response_status rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;

  struct item *item= get_item(key, keylen);
  struct item *nitem;

  if (item == NULL)
  {
    rval= PROTOCOL_BINARY_RESPONSE_KEY_ENOENT;
  }
  else if (cas != 0 && cas != item->cas)
  {
    rval= PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS;
  }
  else if ((nitem= create_item(key, keylen, NULL, item->size + vallen,
                               item->flags, item->exp)) == NULL)
  {
    release_item(item);
    rval= PROTOCOL_BINARY_RESPONSE_ENOMEM;
  }
  else
  {
    memcpy(nitem->data, item->data, item->size);
    memcpy(((char*)(nitem->data)) + item->size, val, vallen);
    release_item(item);
    delete_item(key, keylen);
    put_item(nitem);
    *result_cas= nitem->cas;
    release_item(nitem);
  }

  return rval;
}

static protocol_binary_response_status decrement_handler(const void *cookie,
                                                         const void *key,
                                                         uint16_t keylen,
                                                         uint64_t delta,
                                                         uint64_t initial,
                                                         uint32_t expiration,
                                                         uint64_t *result,
                                                         uint64_t *result_cas) {
  (void)cookie;
  protocol_binary_response_status rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;
  uint64_t val= initial;
  struct item *item= get_item(key, keylen);

  if (item != NULL)
  {
    if (delta > *(uint64_t*)item->data)
      val= 0;
    else
      val= *(uint64_t*)item->data - delta;

    expiration= (uint32_t)item->exp;
    release_item(item);
    delete_item(key, keylen);
  }

  item= create_item(key, keylen, NULL, sizeof(initial), 0, (time_t)expiration);
  if (item == 0)
  {
    rval= PROTOCOL_BINARY_RESPONSE_ENOMEM;
  }
  else
  {
    memcpy(item->data, &val, sizeof(val));
    put_item(item);
    *result= val;
    *result_cas= item->cas;
    release_item(item);
  }

  return rval;
}

static protocol_binary_response_status delete_handler(const void *cookie,
                                                      const void *key,
                                                      uint16_t keylen,
                                                      uint64_t cas) {
  (void)cookie;
  protocol_binary_response_status rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;

  if (cas != 0)
  {
    struct item *item= get_item(key, keylen);
    if (item != NULL)
    {
      if (item->cas != cas)
      {
        release_item(item);
        return PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS;
      }
      release_item(item);
    }
  }

  if (!delete_item(key, keylen))
  {
    rval= PROTOCOL_BINARY_RESPONSE_KEY_ENOENT;
  }

  return rval;
}


static protocol_binary_response_status flush_handler(const void *cookie,
                                                     uint32_t when) {

  (void)cookie;
  flush(when);
  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

static protocol_binary_response_status get_handler(const void *cookie,
                                                   const void *key,
                                                   uint16_t keylen,
                                                   memcached_binary_protocol_get_response_handler response_handler) {
  struct item *item= get_item(key, keylen);

  if (item == NULL)
  {
    return PROTOCOL_BINARY_RESPONSE_KEY_ENOENT;
  }

  protocol_binary_response_status rc;
  rc= response_handler(cookie, key, (uint16_t)keylen,
                          item->data, (uint32_t)item->size, item->flags,
                          item->cas);
  release_item(item);
  return rc;
}

static protocol_binary_response_status increment_handler(const void *cookie,
                                                         const void *key,
                                                         uint16_t keylen,
                                                         uint64_t delta,
                                                         uint64_t initial,
                                                         uint32_t expiration,
                                                         uint64_t *result,
                                                         uint64_t *result_cas) {
  (void)cookie;
  protocol_binary_response_status rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;
  uint64_t val= initial;
  struct item *item= get_item(key, keylen);

  if (item != NULL)
  {
    val= (*(uint64_t*)item->data) + delta;
    expiration= (uint32_t)item->exp;
    release_item(item);
    delete_item(key, keylen);
  }

  item= create_item(key, keylen, NULL, sizeof(initial), 0, (time_t)expiration);
  if (item == NULL)
  {
    rval= PROTOCOL_BINARY_RESPONSE_ENOMEM;
  }
  else
  {
    char buffer[1024] = {0};
    memcpy(buffer, key, keylen);
    memcpy(item->data, &val, sizeof(val));
    put_item(item);
    *result= val;
    *result_cas= item->cas;
    release_item(item);
  }

  return rval;
}

static protocol_binary_response_status noop_handler(const void *cookie) {
  (void)cookie;
  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

static protocol_binary_response_status prepend_handler(const void *cookie,
                                                       const void *key,
                                                       uint16_t keylen,
                                                       const void* val,
                                                       uint32_t vallen,
                                                       uint64_t cas,
                                                       uint64_t *result_cas) {
  (void)cookie;
  protocol_binary_response_status rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;

  struct item *item= get_item(key, keylen);
  struct item *nitem= NULL;

  if (item == NULL)
  {
    rval= PROTOCOL_BINARY_RESPONSE_KEY_ENOENT;
  }
  else if (cas != 0 && cas != item->cas)
  {
    rval= PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS;
  }
  else if ((nitem= create_item(key, keylen, NULL, item->size + vallen,
                                 item->flags, item->exp)) == NULL)
  {
    rval= PROTOCOL_BINARY_RESPONSE_ENOMEM;
  }
  else
  {
    memcpy(nitem->data, val, vallen);
    memcpy(((char*)(nitem->data)) + vallen, item->data, item->size);
    release_item(item);
    item= NULL;
    delete_item(key, keylen);
    put_item(nitem);
    *result_cas= nitem->cas;
  }

  if (item)
    release_item(item);

  if (nitem)
    release_item(nitem);

  return rval;
}

static protocol_binary_response_status quit_handler(const void *cookie) {
  (void)cookie;
  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

static protocol_binary_response_status replace_handler(const void *cookie,
                                                       const void *key,
                                                       uint16_t keylen,
                                                       const void* data,
                                                       uint32_t datalen,
                                                       uint32_t flags,
                                                       uint32_t exptime,
                                                       uint64_t cas,
                                                       uint64_t *result_cas) {
  (void)cookie;
  protocol_binary_response_status rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;
  struct item* item= get_item(key, keylen);

  if (item == NULL)
  {
    rval= PROTOCOL_BINARY_RESPONSE_KEY_ENOENT;
  }
  else if (cas == 0 || cas == item->cas)
  {
    release_item(item);
    delete_item(key, keylen);
    item= create_item(key, keylen, data, datalen, flags, (time_t)exptime);
    if (item == 0)
    {
      rval= PROTOCOL_BINARY_RESPONSE_ENOMEM;
    }
    else
    {
      put_item(item);
      *result_cas= item->cas;
      release_item(item);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS;
    release_item(item);
  }

  return rval;
}

static protocol_binary_response_status set_handler(const void *cookie,
                                                   const void *key,
                                                   uint16_t keylen,
                                                   const void* data,
                                                   uint32_t datalen,
                                                   uint32_t flags,
                                                   uint32_t exptime,
                                                   uint64_t cas,
                                                   uint64_t *result_cas) {
  (void)cookie;
  protocol_binary_response_status rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;

  if (cas != 0)
  {
    struct item* item= get_item(key, keylen);
    if (item != NULL && cas != item->cas)
    {
      /* Invalid CAS value */
      release_item(item);
      return PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS;
    }
  }

  delete_item(key, keylen);
  struct item* item= create_item(key, keylen, data, datalen, flags, (time_t)exptime);
  if (item == 0)
  {
    rval= PROTOCOL_BINARY_RESPONSE_ENOMEM;
  }
  else
  {
    put_item(item);
    *result_cas= item->cas;
    release_item(item);
  }

  return rval;
}

static protocol_binary_response_status stat_handler(const void *cookie,
                                                    const void *key,
                                                    uint16_t keylen,
                                                    memcached_binary_protocol_stat_response_handler response_handler) {
  (void)key;
  (void)keylen;
  /* Just return an empty packet */
  return response_handler(cookie, NULL, 0, NULL, 0);
}

static protocol_binary_response_status version_handler(const void *cookie,
                                                       memcached_binary_protocol_version_response_handler response_handler) {
  const char *version= "0.1.1";
  return response_handler(cookie, version, (uint32_t)strlen(version));
}

memcached_binary_protocol_callback_st interface_v1_impl= {
  .interface_version= MEMCACHED_PROTOCOL_HANDLER_V1,
  .interface.v1= {
    .add= add_handler,
    .append= append_handler,
    .decrement= decrement_handler,
    .delete= delete_handler,
    .flush= flush_handler,
    .get= get_handler,
    .increment= increment_handler,
    .noop= noop_handler,
    .prepend= prepend_handler,
    .quit= quit_handler,
    .replace= replace_handler,
    .set= set_handler,
    .stat= stat_handler,
    .version= version_handler
  }
};
