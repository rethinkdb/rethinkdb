/* LibMemcached
 * Copyright (C) 2006-2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: interface for memcached server
 * Description: main include file for libmemcached
 *
 */
#ifndef LIBMEMCACHED_MEMCACHED_SASL_H
#define LIBMEMCACHED_MEMCACHED_SASL_H

#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
#include <sasl/sasl.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
void memcached_set_sasl_callbacks(memcached_st *ptr,
                                  const sasl_callback_t *callbacks);

LIBMEMCACHED_API
memcached_return_t  memcached_set_sasl_auth_data(memcached_st *ptr,
                                                 const char *username,
                                                 const char *password);

LIBMEMCACHED_API
memcached_return_t memcached_destroy_sasl_auth_data(memcached_st *ptr);


LIBMEMCACHED_API
const sasl_callback_t *memcached_get_sasl_callbacks(memcached_st *ptr);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_clone_sasl(memcached_st *clone, const  memcached_st *source);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_sasl_authenticate_connection(memcached_server_st *server);

#ifdef __cplusplus
}
#endif

#endif /* LIBMEMCACHED_WITH_SASL_SUPPORT */

struct memcached_sasl_st {
#ifdef LIBMEMCACHED_WITH_SASL_SUPPORT
    const sasl_callback_t *callbacks;
#else
    const void *callbacks;
#endif
    /*
    ** Did we allocate data inside the callbacks, or did the user
    ** supply that.
    */
    bool is_allocated;
};



#endif /* LIBMEMCACHED_MEMCACHED_SASL_H */
