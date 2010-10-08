/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#include "libmemcached/protocol/common.h"

#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/*
** **********************************************************************
** INTERNAL INTERFACE
** **********************************************************************
*/

/**
 * Send a preformatted packet back to the client. If the connection is in
 * pedantic mode, it will validate the packet and refuse to send it if it
 * breaks the specification.
 *
 * @param cookie client identification
 * @param request the original request packet
 * @param response the packet to send
 * @return The status of the operation
 */
static protocol_binary_response_status
raw_response_handler(const void *cookie,
                     protocol_binary_request_header *request,
                     protocol_binary_response_header *response)
{
  memcached_protocol_client_st *client= (void*)cookie;

  if (client->root->pedantic &&
      !memcached_binary_protocol_pedantic_check_response(request, response))
  {
    return PROTOCOL_BINARY_RESPONSE_EINVAL;
  }

  if (!client->root->drain(client))
  {
    return PROTOCOL_BINARY_RESPONSE_EIO;
  }

  size_t len= sizeof(*response) + htonl(response->response.bodylen);
  size_t offset= 0;
  char *ptr= (void*)response;

  if (client->output == NULL)
  {
    /* I can write directly to the socket.... */
    do
    {
      size_t num_bytes= len - offset;
      ssize_t nw= client->root->send(client,
                                     client->sock,
                                     ptr + offset,
                                     num_bytes);
      if (nw == -1)
      {
        if (get_socket_errno() == EWOULDBLOCK)
        {
          break;
        }
        else if (get_socket_errno() != EINTR)
        {
          client->error= errno;
          return PROTOCOL_BINARY_RESPONSE_EIO;
        }
      }
      else
      {
        offset += (size_t)nw;
      }
    } while (offset < len);
  }

  return client->root->spool(client, ptr, len - offset);
}

/*
 * Version 0 of the interface is really low level and protocol specific,
 * while the version 1 of the interface is more API focused. We need a
 * way to translate between the command codes on the wire and the
 * application level interface in V1, so let's just use the V0 of the
 * interface as a map instead of creating a huuuge switch :-)
 */

/**
 * Callback for the GET/GETQ/GETK and GETKQ responses
 * @param cookie client identifier
 * @param key the key for the item
 * @param keylen the length of the key
 * @param body the length of the body
 * @param bodylen the length of the body
 * @param flags the flags for the item
 * @param cas the CAS id for the item
 */
static protocol_binary_response_status
get_response_handler(const void *cookie,
                     const void *key,
                     uint16_t keylen,
                     const void *body,
                     uint32_t bodylen,
                     uint32_t flags,
                     uint64_t cas) {

  memcached_protocol_client_st *client= (void*)cookie;
  uint8_t opcode= client->current_command->request.opcode;

  if (opcode == PROTOCOL_BINARY_CMD_GET || opcode == PROTOCOL_BINARY_CMD_GETQ)
  {
    keylen= 0;
  }

  protocol_binary_response_get response= {
    .message.header.response= {
      .magic= PROTOCOL_BINARY_RES,
      .opcode= opcode,
      .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
      .opaque= client->current_command->request.opaque,
      .cas= htonll(cas),
      .keylen= htons(keylen),
      .extlen= 4,
      .bodylen= htonl(bodylen + keylen + 4),
    },
  };

  response.message.body.flags= htonl(flags);

  protocol_binary_response_status rval;
  const protocol_binary_response_status success= PROTOCOL_BINARY_RESPONSE_SUCCESS;
  if ((rval= client->root->spool(client, response.bytes, sizeof(response.bytes))) != success ||
      (rval= client->root->spool(client, key, keylen)) != success ||
      (rval= client->root->spool(client, body, bodylen)) != success)
  {
    return rval;
  }

  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

/**
 * Callback for the STAT responses
 * @param cookie client identifier
 * @param key the key for the item
 * @param keylen the length of the key
 * @param body the length of the body
 * @param bodylen the length of the body
 */
static protocol_binary_response_status
stat_response_handler(const void *cookie,
                     const void *key,
                     uint16_t keylen,
                     const void *body,
                     uint32_t bodylen) {

  memcached_protocol_client_st *client= (void*)cookie;

  protocol_binary_response_no_extras response= {
    .message.header.response= {
      .magic= PROTOCOL_BINARY_RES,
      .opcode= client->current_command->request.opcode,
      .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
      .opaque= client->current_command->request.opaque,
      .keylen= htons(keylen),
      .bodylen= htonl(bodylen + keylen),
    },
  };

  protocol_binary_response_status rval;
  const protocol_binary_response_status success= PROTOCOL_BINARY_RESPONSE_SUCCESS;
  if ((rval= client->root->spool(client, response.bytes, sizeof(response.bytes))) != success ||
      (rval= client->root->spool(client, key, keylen)) != success ||
      (rval= client->root->spool(client, body, bodylen)) != success)
  {
    return rval;
  }

  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

/**
 * Callback for the VERSION responses
 * @param cookie client identifier
 * @param text the length of the body
 * @param textlen the length of the body
 */
static protocol_binary_response_status
version_response_handler(const void *cookie,
                         const void *text,
                         uint32_t textlen) {

  memcached_protocol_client_st *client= (void*)cookie;

  protocol_binary_response_no_extras response= {
    .message.header.response= {
      .magic= PROTOCOL_BINARY_RES,
      .opcode= client->current_command->request.opcode,
      .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
      .opaque= client->current_command->request.opaque,
      .bodylen= htonl(textlen),
    },
  };

  protocol_binary_response_status rval;
  const protocol_binary_response_status success= PROTOCOL_BINARY_RESPONSE_SUCCESS;
  if ((rval= client->root->spool(client, response.bytes, sizeof(response.bytes))) != success ||
      (rval= client->root->spool(client, text, textlen)) != success)
  {
    return rval;
  }

  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

/**
 * Callback for ADD and ADDQ
 * @param cookie the calling client
 * @param header the add/addq command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
add_command_handler(const void *cookie,
                    protocol_binary_request_header *header,
                    memcached_binary_protocol_raw_response_handler response_handler)
{
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.add != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    uint32_t datalen= ntohl(header->request.bodylen) - keylen - 8;
    protocol_binary_request_add *request= (void*)header;
    uint32_t flags= ntohl(request->message.body.flags);
    uint32_t timeout= ntohl(request->message.body.expiration);
    char *key= ((char*)header) + sizeof(*header) + 8;
    char *data= key + keylen;
    uint64_t cas;

    rval= client->root->callback->interface.v1.add(cookie, key, keylen,
                                                   data, datalen, flags,
                                                   timeout, &cas);

    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_ADD)
    {
      /* Send a positive request */
      protocol_binary_response_no_extras response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_ADD,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
            .cas= ntohll(cas)
          }
        }
      };
      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for DECREMENT and DECREMENTQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
decrement_command_handler(const void *cookie,
                          protocol_binary_request_header *header,
                          memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.decrement != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    protocol_binary_request_decr *request= (void*)header;
    uint64_t init= ntohll(request->message.body.initial);
    uint64_t delta= ntohll(request->message.body.delta);
    uint32_t timeout= ntohl(request->message.body.expiration);
    void *key= request->bytes + sizeof(request->bytes);
    uint64_t result;
    uint64_t cas;

    rval= client->root->callback->interface.v1.decrement(cookie, key, keylen,
                                                         delta, init, timeout,
                                                         &result, &cas);
    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_DECREMENT)
    {
      /* Send a positive request */
      protocol_binary_response_decr response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_DECREMENT,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
            .cas= ntohll(cas),
            .bodylen= htonl(8)
          },
          .body.value= htonll(result)
        }
      };
      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for DELETE and DELETEQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
delete_command_handler(const void *cookie,
                       protocol_binary_request_header *header,
                       memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.delete != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    void *key= (header + 1);
    uint64_t cas= ntohll(header->request.cas);
    rval= client->root->callback->interface.v1.delete(cookie, key, keylen, cas);
    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_DELETE)
    {
      /* Send a positive request */
      protocol_binary_response_no_extras response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_DELETE,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
          }
        }
      };
      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for FLUSH and FLUSHQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
flush_command_handler(const void *cookie,
                      protocol_binary_request_header *header,
                      memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.flush != NULL)
  {
    protocol_binary_request_flush *flush= (void*)header;
    uint32_t timeout= 0;
    if (htonl(header->request.bodylen) == 4)
    {
      timeout= ntohl(flush->message.body.expiration);
    }

    rval= client->root->callback->interface.v1.flush(cookie, timeout);
    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_FLUSH)
    {
      /* Send a positive request */
      protocol_binary_response_no_extras response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_FLUSH,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
          }
        }
      };
      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for GET, GETK, GETQ, GETKQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
get_command_handler(const void *cookie,
                    protocol_binary_request_header *header,
                    memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.get != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    void *key= (header + 1);
    rval= client->root->callback->interface.v1.get(cookie, key, keylen,
                                                   get_response_handler);

    if (rval == PROTOCOL_BINARY_RESPONSE_KEY_ENOENT &&
        (header->request.opcode == PROTOCOL_BINARY_CMD_GETQ ||
         header->request.opcode == PROTOCOL_BINARY_CMD_GETKQ))
    {
      /* Quiet commands shouldn't respond on cache misses */
      rval= PROTOCOL_BINARY_RESPONSE_SUCCESS;
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for INCREMENT and INCREMENTQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
increment_command_handler(const void *cookie,
                          protocol_binary_request_header *header,
                          memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.increment != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    protocol_binary_request_incr *request= (void*)header;
    uint64_t init= ntohll(request->message.body.initial);
    uint64_t delta= ntohll(request->message.body.delta);
    uint32_t timeout= ntohl(request->message.body.expiration);
    void *key= request->bytes + sizeof(request->bytes);
    uint64_t cas;
    uint64_t result;

    rval= client->root->callback->interface.v1.increment(cookie, key, keylen,
                                                         delta, init, timeout,
                                                         &result, &cas);
    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_INCREMENT)
    {
      /* Send a positive request */
      protocol_binary_response_incr response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_INCREMENT,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
            .cas= ntohll(cas),
            .bodylen= htonl(8)
          },
          .body.value= htonll(result)
        }
      };

      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for noop. Inform the v1 interface about the noop packet, and
 * create and send a packet back to the client
 *
 * @param cookie the calling client
 * @param header the command
 * @param response_handler the response handler
 * @return the result of the operation
 */
static protocol_binary_response_status
noop_command_handler(const void *cookie,
                     protocol_binary_request_header *header,
                     memcached_binary_protocol_raw_response_handler response_handler)
{
  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.noop != NULL)
  {
    client->root->callback->interface.v1.noop(cookie);
  }

  protocol_binary_response_no_extras response= {
    .message= {
      .header.response= {
        .magic= PROTOCOL_BINARY_RES,
        .opcode= PROTOCOL_BINARY_CMD_NOOP,
        .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
        .opaque= header->request.opaque,
      }
    }
  };

  return response_handler(cookie, header, (void*)&response);
}

/**
 * Callback for APPEND and APPENDQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
append_command_handler(const void *cookie,
                       protocol_binary_request_header *header,
                       memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.append != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    uint32_t datalen= ntohl(header->request.bodylen) - keylen;
    char *key= (void*)(header + 1);
    char *data= key + keylen;
    uint64_t cas= ntohll(header->request.cas);
    uint64_t result_cas;

    rval= client->root->callback->interface.v1.append(cookie, key, keylen,
                                                      data, datalen, cas,
                                                      &result_cas);
    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_APPEND)
    {
      /* Send a positive request */
      protocol_binary_response_no_extras response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_APPEND,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
            .cas= ntohll(result_cas),
          },
        }
      };
      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for PREPEND and PREPENDQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
prepend_command_handler(const void *cookie,
                        protocol_binary_request_header *header,
                        memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.prepend != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    uint32_t datalen= ntohl(header->request.bodylen) - keylen;
    char *key= (char*)(header + 1);
    char *data= key + keylen;
    uint64_t cas= ntohll(header->request.cas);
    uint64_t result_cas;
    rval= client->root->callback->interface.v1.prepend(cookie, key, keylen,
                                                       data, datalen, cas,
                                                       &result_cas);
    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_PREPEND)
    {
      /* Send a positive request */
      protocol_binary_response_no_extras response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_PREPEND,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
            .cas= ntohll(result_cas),
          },
        }
      };
      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for QUIT and QUITQ. Notify the client and shut down the connection
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
quit_command_handler(const void *cookie,
                     protocol_binary_request_header *header,
                     memcached_binary_protocol_raw_response_handler response_handler)
{
  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.quit != NULL)
  {
    client->root->callback->interface.v1.quit(cookie);
  }

  protocol_binary_response_no_extras response= {
    .message= {
      .header.response= {
        .magic= PROTOCOL_BINARY_RES,
        .opcode= PROTOCOL_BINARY_CMD_QUIT,
        .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
        .opaque= header->request.opaque
      }
    }
  };

  if (header->request.opcode == PROTOCOL_BINARY_CMD_QUIT)
  {
    response_handler(cookie, header, (void*)&response);
  }

  /* I need a better way to signal to close the connection */
  return PROTOCOL_BINARY_RESPONSE_EIO;
}

/**
 * Callback for REPLACE and REPLACEQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
replace_command_handler(const void *cookie,
                        protocol_binary_request_header *header,
                        memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.replace != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    uint32_t datalen= ntohl(header->request.bodylen) - keylen - 8;
    protocol_binary_request_replace *request= (void*)header;
    uint32_t flags= ntohl(request->message.body.flags);
    uint32_t timeout= ntohl(request->message.body.expiration);
    char *key= ((char*)header) + sizeof(*header) + 8;
    char *data= key + keylen;
    uint64_t cas= ntohll(header->request.cas);
    uint64_t result_cas;

    rval= client->root->callback->interface.v1.replace(cookie, key, keylen,
                                                       data, datalen, flags,
                                                       timeout, cas,
                                                       &result_cas);
    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_REPLACE)
    {
      /* Send a positive request */
      protocol_binary_response_no_extras response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_REPLACE,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
            .cas= ntohll(result_cas),
          },
        }
      };
      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for SET and SETQ
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
set_command_handler(const void *cookie,
                    protocol_binary_request_header *header,
                    memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.set != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);
    uint32_t datalen= ntohl(header->request.bodylen) - keylen - 8;
    protocol_binary_request_replace *request= (void*)header;
    uint32_t flags= ntohl(request->message.body.flags);
    uint32_t timeout= ntohl(request->message.body.expiration);
    char *key= ((char*)header) + sizeof(*header) + 8;
    char *data= key + keylen;
    uint64_t cas= ntohll(header->request.cas);
    uint64_t result_cas;


    rval= client->root->callback->interface.v1.set(cookie, key, keylen,
                                                   data, datalen, flags,
                                                   timeout, cas, &result_cas);
    if (rval == PROTOCOL_BINARY_RESPONSE_SUCCESS &&
        header->request.opcode == PROTOCOL_BINARY_CMD_SET)
    {
      /* Send a positive request */
      protocol_binary_response_no_extras response= {
        .message= {
          .header.response= {
            .magic= PROTOCOL_BINARY_RES,
            .opcode= PROTOCOL_BINARY_CMD_SET,
            .status= htons(PROTOCOL_BINARY_RESPONSE_SUCCESS),
            .opaque= header->request.opaque,
            .cas= ntohll(result_cas),
          },
        }
      };
      rval= response_handler(cookie, header, (void*)&response);
    }
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for STAT
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
stat_command_handler(const void *cookie,
                     protocol_binary_request_header *header,
                     memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.stat != NULL)
  {
    uint16_t keylen= ntohs(header->request.keylen);

    rval= client->root->callback->interface.v1.stat(cookie,
                                                    (void*)(header + 1),
                                                    keylen,
                                                    stat_response_handler);
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * Callback for VERSION
 * @param cookie the calling client
 * @param header the command
 * @param response_handler not used
 * @return the result of the operation
 */
static protocol_binary_response_status
version_command_handler(const void *cookie,
                        protocol_binary_request_header *header,
                        memcached_binary_protocol_raw_response_handler response_handler)
{
  (void)response_handler;
  (void)header;
  protocol_binary_response_status rval;

  memcached_protocol_client_st *client= (void*)cookie;
  if (client->root->callback->interface.v1.version != NULL)
  {
    rval= client->root->callback->interface.v1.version(cookie,
                                                       version_response_handler);
  }
  else
  {
    rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  }

  return rval;
}

/**
 * The map to remap between the com codes and the v1 logical setting
 */
static memcached_binary_protocol_command_handler comcode_v0_v1_remap[256]= {
  [PROTOCOL_BINARY_CMD_ADDQ]= add_command_handler,
  [PROTOCOL_BINARY_CMD_ADD]= add_command_handler,
  [PROTOCOL_BINARY_CMD_APPENDQ]= append_command_handler,
  [PROTOCOL_BINARY_CMD_APPEND]= append_command_handler,
  [PROTOCOL_BINARY_CMD_DECREMENTQ]= decrement_command_handler,
  [PROTOCOL_BINARY_CMD_DECREMENT]= decrement_command_handler,
  [PROTOCOL_BINARY_CMD_DELETEQ]= delete_command_handler,
  [PROTOCOL_BINARY_CMD_DELETE]= delete_command_handler,
  [PROTOCOL_BINARY_CMD_FLUSHQ]= flush_command_handler,
  [PROTOCOL_BINARY_CMD_FLUSH]= flush_command_handler,
  [PROTOCOL_BINARY_CMD_GETKQ]= get_command_handler,
  [PROTOCOL_BINARY_CMD_GETK]= get_command_handler,
  [PROTOCOL_BINARY_CMD_GETQ]= get_command_handler,
  [PROTOCOL_BINARY_CMD_GET]= get_command_handler,
  [PROTOCOL_BINARY_CMD_INCREMENTQ]= increment_command_handler,
  [PROTOCOL_BINARY_CMD_INCREMENT]= increment_command_handler,
  [PROTOCOL_BINARY_CMD_NOOP]= noop_command_handler,
  [PROTOCOL_BINARY_CMD_PREPENDQ]= prepend_command_handler,
  [PROTOCOL_BINARY_CMD_PREPEND]= prepend_command_handler,
  [PROTOCOL_BINARY_CMD_QUITQ]= quit_command_handler,
  [PROTOCOL_BINARY_CMD_QUIT]= quit_command_handler,
  [PROTOCOL_BINARY_CMD_REPLACEQ]= replace_command_handler,
  [PROTOCOL_BINARY_CMD_REPLACE]= replace_command_handler,
  [PROTOCOL_BINARY_CMD_SETQ]= set_command_handler,
  [PROTOCOL_BINARY_CMD_SET]= set_command_handler,
  [PROTOCOL_BINARY_CMD_STAT]= stat_command_handler,
  [PROTOCOL_BINARY_CMD_VERSION]= version_command_handler,
};

/**
 * Try to execute a command. Fire the pre/post functions and the specialized
 * handler function if it's set. If not, the unknown probe should be fired
 * if it's present.
 * @param client the client connection to operate on
 * @param header the command to execute
 * @return true if success or false if a fatal error occured so that the
 *         connection should be shut down.
 */
static protocol_binary_response_status execute_command(memcached_protocol_client_st *client, protocol_binary_request_header *header)
{
  if (client->root->pedantic &&
      memcached_binary_protocol_pedantic_check_request(header))
  {
      /* @todo return invalid command packet */
  }

  /* we got all data available, execute the callback! */
  if (client->root->callback->pre_execute != NULL)
  {
    client->root->callback->pre_execute(client, header);
  }

  protocol_binary_response_status rval;
  rval= PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND;
  uint8_t cc= header->request.opcode;

  switch (client->root->callback->interface_version)
  {
  case 0:
    if (client->root->callback->interface.v0.comcode[cc] != NULL) {
      rval= client->root->callback->interface.v0.comcode[cc](client, header, raw_response_handler);
    }
    break;
  case 1:
    if (comcode_v0_v1_remap[cc] != NULL) {
      rval= comcode_v0_v1_remap[cc](client, header, raw_response_handler);
    }
    break;
  default:
    /* Unknown interface.
     * It should be impossible to get here so I'll just call abort
     * to avoid getting a compiler warning :-)
     */
    abort();
  }


  if (rval == PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND &&
      client->root->callback->unknown != NULL)
  {
    rval= client->root->callback->unknown(client, header, raw_response_handler);
  }

  if (rval != PROTOCOL_BINARY_RESPONSE_SUCCESS &&
      rval != PROTOCOL_BINARY_RESPONSE_EIO &&
      rval != PROTOCOL_BINARY_RESPONSE_PAUSE)
  {
    protocol_binary_response_no_extras response= {
      .message= {
        .header.response= {
          .magic= PROTOCOL_BINARY_RES,
          .opcode= cc,
          .status= htons(rval),
          .opaque= header->request.opaque,
        },
      }
    };
    rval= raw_response_handler(client, header, (void*)&response);
  }

  if (client->root->callback->post_execute != NULL)
  {
    client->root->callback->post_execute(client, header);
  }

  return rval;
}

/*
** **********************************************************************
** "PROTOECTED" INTERFACE
** **********************************************************************
*/
memcached_protocol_event_t memcached_binary_protocol_process_data(memcached_protocol_client_st *client, ssize_t *length, void **endptr)
{
  /* try to parse all of the received packets */
  protocol_binary_request_header *header;
  header= (void*)client->root->input_buffer;
  if (header->request.magic != (uint8_t)PROTOCOL_BINARY_REQ)
  {
    client->error= EINVAL;
    return MEMCACHED_PROTOCOL_ERROR_EVENT;
  }
  ssize_t len= *length;

  while (len >= (ssize_t)sizeof(*header) &&
         (len >= (ssize_t)(sizeof(*header) + ntohl(header->request.bodylen))))
  {
    /* I have the complete package */
    client->current_command= header;
    protocol_binary_response_status rv= execute_command(client, header);

    if (rv == PROTOCOL_BINARY_RESPONSE_EIO)
    {
      *length= len;
      *endptr= (void*)header;
      return MEMCACHED_PROTOCOL_ERROR_EVENT;
    } else if (rv == PROTOCOL_BINARY_RESPONSE_PAUSE)
      return MEMCACHED_PROTOCOL_PAUSE_EVENT;

    ssize_t total= (ssize_t)(sizeof(*header) + ntohl(header->request.bodylen));
    len -= total;
    if (len > 0)
    {
      intptr_t ptr= (intptr_t)header;
      ptr += total;
      if ((ptr % 8) == 0)
      {
        header= (void*)ptr;
      }
      else
      {
        /* Fix alignment */
        memmove(client->root->input_buffer, (void*)ptr, (size_t)len);
        header= (void*)client->root->input_buffer;
      }
    }
    *length= len;
    *endptr= (void*)header;
  }

  return MEMCACHED_PROTOCOL_READ_EVENT;
}

/*
** **********************************************************************
** PUBLIC INTERFACE
** **********************************************************************
*/
memcached_binary_protocol_callback_st *memcached_binary_protocol_get_callbacks(memcached_protocol_st *instance)
{
  return instance->callback;
}

void memcached_binary_protocol_set_callbacks(memcached_protocol_st *instance, memcached_binary_protocol_callback_st *callback)
{
  instance->callback= callback;
}

memcached_binary_protocol_raw_response_handler memcached_binary_protocol_get_raw_response_handler(const void *cookie)
{
  (void)cookie;
  return raw_response_handler;
}

void memcached_binary_protocol_set_pedantic(memcached_protocol_st *instance, bool enable)
{
  instance->pedantic= enable;
}

bool memcached_binary_protocol_get_pedantic(memcached_protocol_st *instance)
{
  return instance->pedantic;
}

