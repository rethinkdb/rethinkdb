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
#include "common.h"

void memcached_set_sasl_callbacks(memcached_st *ptr,
                                  const sasl_callback_t *callbacks)
{
  ptr->sasl.callbacks= callbacks;
  ptr->sasl.is_allocated= false;
}

const sasl_callback_t *memcached_get_sasl_callbacks(memcached_st *ptr)
{
  return ptr->sasl.callbacks;
}

/**
 * Resolve the names for both ends of a connection
 * @param fd socket to check
 * @param laddr local address (out)
 * @param raddr remote address (out)
 * @return true on success false otherwise (errno contains more info)
 */
static bool resolve_names(int fd, char *laddr, char *raddr)
{
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
  struct sockaddr_storage saddr;
  socklen_t salen= sizeof(saddr);

  if ((getsockname(fd, (struct sockaddr *)&saddr, &salen) < 0) ||
      (getnameinfo((struct sockaddr *)&saddr, salen, host, sizeof(host),
                   port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) < 0))
  {
    return false;
  }

  (void)sprintf(laddr, "%s;%s", host, port);
  salen= sizeof(saddr);

  if ((getpeername(fd, (struct sockaddr *)&saddr, &salen) < 0) ||
      (getnameinfo((struct sockaddr *)&saddr, salen, host, sizeof(host),
                   port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV) < 0))
  {
    return false;
  }

  (void)sprintf(raddr, "%s;%s", host, port);

  return true;
}

memcached_return_t memcached_sasl_authenticate_connection(memcached_server_st *server)
{
  memcached_return_t rc;

  /* SANITY CHECK: SASL can only be used with the binary protocol */
  unlikely (!server->root->flags.binary_protocol)
    return MEMCACHED_FAILURE;

  /* Try to get the supported mech from the server. Servers without SASL
   * support will return UNKNOWN COMMAND, so we can just treat that
   * as authenticated
   */
  protocol_binary_request_no_extras request= {
    .message.header.request= {
      .magic= PROTOCOL_BINARY_REQ,
      .opcode= PROTOCOL_BINARY_CMD_SASL_LIST_MECHS
    }
  };

  if (memcached_io_write(server, request.bytes,
			 sizeof(request.bytes), 1) != sizeof(request.bytes))
  {
    return MEMCACHED_WRITE_FAILURE;
  }

  memcached_server_response_increment(server);

  char mech[MEMCACHED_MAX_BUFFER];
  rc= memcached_response(server, mech, sizeof(mech), NULL);
  if (rc != MEMCACHED_SUCCESS)
  {
    if (rc == MEMCACHED_PROTOCOL_ERROR)
    {
      /* If the server doesn't support SASL it will return PROTOCOL_ERROR.
       * This error may also be returned for other errors, but let's assume
       * that the server don't support SASL and treat it as success and
       * let the client fail with the next operation if the error was
       * caused by another problem....
       */
      rc= MEMCACHED_SUCCESS;
    }

    return rc;
  }

  /* set ip addresses */
  char laddr[NI_MAXHOST + NI_MAXSERV];
  char raddr[NI_MAXHOST + NI_MAXSERV];

  unlikely (!resolve_names(server->fd, laddr, raddr))
  {
    server->cached_errno= errno;
    return MEMCACHED_ERRNO;
  }

  sasl_conn_t *conn;
  int ret= sasl_client_new("memcached", server->hostname, laddr, raddr,
			   server->root->sasl.callbacks, 0, &conn);
  if (ret != SASL_OK)
  {
    return MEMCACHED_AUTH_PROBLEM;
  }

  const char *data;
  const char *chosenmech;
  unsigned int len;
  ret= sasl_client_start(conn, mech, NULL, &data, &len, &chosenmech);

  if (ret != SASL_OK && ret != SASL_CONTINUE)
  {
    rc= MEMCACHED_AUTH_PROBLEM;
    goto end;
  }

  uint16_t keylen= (uint16_t)strlen(chosenmech);
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_SASL_AUTH;
  request.message.header.request.keylen= htons(keylen);
  request.message.header.request.bodylen= htonl(len + keylen);

  do {
    /* send the packet */

    struct libmemcached_io_vector_st vector[]=
    {
      { .length= sizeof(request.bytes), .buffer= request.bytes },
      { .length= keylen, .buffer= chosenmech },
      { .length= len, .buffer= data }
    };

    if (memcached_io_writev(server, vector, 3, true) == -1)
    {
      rc= MEMCACHED_WRITE_FAILURE;
      goto end;
    }
    memcached_server_response_increment(server);

    /* read the response */
    rc= memcached_response(server, NULL, 0, NULL);
    if (rc != MEMCACHED_AUTH_CONTINUE)
    {
      goto end;
    }

    ret= sasl_client_step(conn, memcached_result_value(&server->root->result),
                          (unsigned int)memcached_result_length(&server->root->result),
                          NULL, &data, &len);

    if (ret != SASL_OK && ret != SASL_CONTINUE)
    {
      rc= MEMCACHED_AUTH_PROBLEM;
      goto end;
    }

    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_SASL_STEP;
    request.message.header.request.bodylen= htonl(len + keylen);
  } while (true);

end:
  /* Release resources */
  sasl_dispose(&conn);

  return rc;
}

static int get_username(void *context, int id, const char **result,
                        unsigned int *len)
{
  if (!context || !result || (id != SASL_CB_USER && id != SASL_CB_AUTHNAME))
  {
    return SASL_BADPARAM;
  }

  *result= context;
  if (len)
  {
    *len= (unsigned int)strlen(*result);
  }

  return SASL_OK;
}

static int get_password(sasl_conn_t *conn, void *context, int id,
                        sasl_secret_t **psecret)
{
  if (!conn || ! psecret || id != SASL_CB_PASS)
  {
    return SASL_BADPARAM;
  }

  *psecret= context;

  return SASL_OK;
}

memcached_return_t memcached_set_sasl_auth_data(memcached_st *ptr,
                                                const char *username,
                                                const char *password)
{
  if (ptr == NULL || username == NULL ||
      password == NULL || ptr->sasl.callbacks != NULL)
  {
    return MEMCACHED_FAILURE;
  }

  sasl_callback_t *cb= libmemcached_calloc(ptr, 4, sizeof(sasl_callback_t));
  char *name= libmemcached_malloc(ptr, strlen(username) + 1);
  sasl_secret_t *secret= libmemcached_malloc(ptr, strlen(password) + 1 + sizeof(*secret))
;
  if (cb == NULL || name == NULL || secret == NULL)
  {
    libmemcached_free(ptr, cb);
    libmemcached_free(ptr, name);
    libmemcached_free(ptr, secret);
    return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
  }

  secret->len= strlen(password);
  strcpy((void*)secret->data, password);

  cb[0].id= SASL_CB_USER;
  cb[0].proc= get_username;
  cb[0].context= strcpy(name, username);
  cb[1].id= SASL_CB_AUTHNAME;
  cb[1].proc= get_username;
  cb[1].context= name;
  cb[2].id= SASL_CB_PASS;
  cb[2].proc= get_password;
  cb[2].context= secret;
  cb[3].id= SASL_CB_LIST_END;

  ptr->sasl.callbacks= cb;
  ptr->sasl.is_allocated= true;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_destroy_sasl_auth_data(memcached_st *ptr)
{
   if (ptr == NULL || ptr->sasl.callbacks == NULL)
   {
     return MEMCACHED_FAILURE;
   }

   if (ptr->sasl.is_allocated)
   {
     libmemcached_free(ptr, ptr->sasl.callbacks[0].context);
     libmemcached_free(ptr, ptr->sasl.callbacks[2].context);
     libmemcached_free(ptr, (void*)ptr->sasl.callbacks);
     ptr->sasl.is_allocated= false;
   }

   ptr->sasl.callbacks= NULL;

   return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_clone_sasl(memcached_st *clone, const  memcached_st *source)
{

  if (source->sasl.callbacks == NULL)
  {
    return MEMCACHED_SUCCESS;
  }

  /* Hopefully we are using our own callback mechanisms.. */
  if (source->sasl.callbacks[0].id == SASL_CB_USER &&
      source->sasl.callbacks[0].proc == get_username &&
      source->sasl.callbacks[1].id == SASL_CB_AUTHNAME &&
      source->sasl.callbacks[1].proc == get_username &&
      source->sasl.callbacks[2].id == SASL_CB_PASS &&
      source->sasl.callbacks[2].proc == get_password &&
      source->sasl.callbacks[3].id == SASL_CB_LIST_END)
  {
    sasl_secret_t *secret= source->sasl.callbacks[2].context;
    return memcached_set_sasl_auth_data(clone,
                                        source->sasl.callbacks[0].context,
                                        (const char*)secret->data);
  }

  /*
   * But we're not. It may work if we know what the user tries to pass
   * into the list, but if we don't know the ID we don't know how to handle
   * the context...
   */
  size_t total= 0;

  while (source->sasl.callbacks[total].id != SASL_CB_LIST_END)
  {
    switch (source->sasl.callbacks[total].id)
    {
    case SASL_CB_USER:
    case SASL_CB_AUTHNAME:
    case SASL_CB_PASS:
       break;
    default:
       /* I don't know how to deal with this... */
       return MEMCACHED_NOT_SUPPORTED;
    }

    ++total;
  }

  sasl_callback_t *cb= libmemcached_calloc(clone, total + 1, sizeof(sasl_callback_t));
  if (cb == NULL)
  {
    return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
  }
  memcpy(cb, source->sasl.callbacks, (total + 1) * sizeof(sasl_callback_t));

  /* Now update the context... */
  for (size_t x= 0; x < total; ++x)
  {
    if (cb[x].id == SASL_CB_USER || cb[x].id == SASL_CB_AUTHNAME)
    {
      cb[x].context= libmemcached_malloc(clone, strlen(source->sasl.callbacks[x].context));

      if (cb[x].context == NULL)
      {
        /* Failed to allocate memory, clean up previously allocated memory */
        for (size_t y= 0; y < x; ++y)
        {
          libmemcached_free(clone, clone->sasl.callbacks[y].context);
        }

        libmemcached_free(clone, cb);
        return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
      }
      strcpy(cb[x].context, source->sasl.callbacks[x].context);
    }
    else
    {
      sasl_secret_t *src = source->sasl.callbacks[x].context;
      sasl_secret_t *n = libmemcached_malloc(clone, src->len + 1 + sizeof(*n));
      if (n == NULL)
      {
        /* Failed to allocate memory, clean up previously allocated memory */
        for (size_t y= 0; y < x; ++y)
        {
          libmemcached_free(clone, clone->sasl.callbacks[y].context);
        }

        libmemcached_free(clone, cb);
        return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
      }
      memcpy(n, src, src->len + 1 + sizeof(*n));
      cb[x].context= n;
    }
  }

  clone->sasl.callbacks= cb;
  clone->sasl.is_allocated= true;

  return MEMCACHED_SUCCESS;
}
