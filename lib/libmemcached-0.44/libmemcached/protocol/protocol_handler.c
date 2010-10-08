/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#include "libmemcached/protocol/common.h"

#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>

/*
** **********************************************************************
** INTERNAL INTERFACE
** **********************************************************************
*/

/**
 * The default function to receive data from the client. This function
 * just wraps the recv function to receive from a socket.
 * See man -s3socket recv for more information.
 *
 * @param cookie cookie indentifying a client, not used
 * @param sock socket to read from
 * @param buf the destination buffer
 * @param nbytes the number of bytes to read
 * @return the number of bytes transferred of -1 upon error
 */
static ssize_t default_recv(const void *cookie,
                            memcached_socket_t sock,
                            void *buf,
                            size_t nbytes)
{
  (void)cookie;
  return recv(sock, buf, nbytes, 0);
}

/**
 * The default function to send data to the server. This function
 * just wraps the send function to send through a socket.
 * See man -s3socket send for more information.
 *
 * @param cookie cookie indentifying a client, not used
 * @param sock socket to send to
 * @param buf the source buffer
 * @param nbytes the number of bytes to send
 * @return the number of bytes transferred of -1 upon error
 */
static ssize_t default_send(const void *cookie,
                            memcached_socket_t fd,
                            const void *buf,
                            size_t nbytes)
{
  (void)cookie;
  return send(fd, buf, nbytes, 0);
}

/**
 * Try to drain the output buffers without blocking
 *
 * @param client the client to drain
 * @return false if an error occured (connection should be shut down)
 *         true otherwise (please note that there may be more data to
 *              left in the buffer to send)
 */
static bool drain_output(struct memcached_protocol_client_st *client)
{
  ssize_t len;

  /* Do we have pending data to send? */
  while (client->output != NULL)
  {
    len= client->root->send(client,
                            client->sock,
                            client->output->data + client->output->offset,
                            client->output->nbytes - client->output->offset);

    if (len == -1)
    {
      if (get_socket_errno() == EWOULDBLOCK)
      {
        return true;
      }
      else if (get_socket_errno() != EINTR)
      {
        client->error= get_socket_errno();
        return false;
      }
    }
    else
    {
      client->output->offset += (size_t)len;
      if (client->output->offset == client->output->nbytes)
      {
        /* This was the complete buffer */
        struct chunk_st *old= client->output;
        client->output= client->output->next;
        if (client->output == NULL)
        {
          client->output_tail= NULL;
        }
        cache_free(client->root->buffer_cache, old);
      }
    }
  }

  return true;
}

/**
 * Allocate an output buffer and chain it into the output list
 *
 * @param client the client that needs the buffer
 * @return pointer to the new chunk if the allocation succeeds, NULL otherwise
 */
static struct chunk_st *allocate_output_chunk(struct memcached_protocol_client_st *client)
{
  struct chunk_st *ret= cache_alloc(client->root->buffer_cache);

  if (ret == NULL)
  {
    return NULL;
  }

  ret->offset= ret->nbytes= 0;
  ret->next= NULL;
  ret->size= CHUNK_BUFFERSIZE;
  ret->data= (void*)(ret + 1);
  if (client->output == NULL)
  {
    client->output= client->output_tail= ret;
  }
  else
  {
    client->output_tail->next= ret;
    client->output_tail= ret;
  }

  return ret;
}

/**
 * Spool data into the send-buffer for a client.
 *
 * @param client the client to spool the data for
 * @param data the data to spool
 * @param length the number of bytes of data to spool
 * @return PROTOCOL_BINARY_RESPONSE_SUCCESS if success,
 *         PROTOCOL_BINARY_RESPONSE_ENOMEM if we failed to allocate memory
 */
static protocol_binary_response_status spool_output(struct memcached_protocol_client_st *client,
                                                    const void *data,
                                                    size_t length)
{
  if (client->mute)
  {
    return PROTOCOL_BINARY_RESPONSE_SUCCESS;
  }

  size_t offset= 0;

  struct chunk_st *chunk= client->output;
  while (offset < length)
  {
    if (chunk == NULL || (chunk->size - chunk->nbytes) == 0)
    {
      if ((chunk= allocate_output_chunk(client)) == NULL)
      {
        return PROTOCOL_BINARY_RESPONSE_ENOMEM;
      }
    }

    size_t bulk= length - offset;
    if (bulk > chunk->size - chunk->nbytes)
    {
      bulk= chunk->size - chunk->nbytes;
    }

    memcpy(chunk->data + chunk->nbytes, data, bulk);
    chunk->nbytes += bulk;
    offset += bulk;
  }

  return PROTOCOL_BINARY_RESPONSE_SUCCESS;
}

/**
 * Try to determine the protocol used on this connection.
 * If the first byte contains the magic byte PROTOCOL_BINARY_REQ we should
 * be using the binary protocol on the connection. I implemented the support
 * for the ASCII protocol by wrapping into the simple interface (aka v1),
 * so the implementors needs to provide an implementation of that interface
 *
 */
static memcached_protocol_event_t determine_protocol(struct memcached_protocol_client_st *client, ssize_t *length, void **endptr)
{
  if (*client->root->input_buffer == (uint8_t)PROTOCOL_BINARY_REQ)
  {
    client->work= memcached_binary_protocol_process_data;
  }
  else if (client->root->callback->interface_version == 1)
  {
    /*
     * The ASCII protocol can only be used if the implementors provide
     * an implementation for the version 1 of the interface..
     *
     * @todo I should allow the implementors to provide an implementation
     *       for version 0 and 1 at the same time and set the preferred
     *       interface to use...
     */
    client->work= memcached_ascii_protocol_process_data;
  }
  else
  {
    /* Let's just output a warning the way it is supposed to look like
     * in the ASCII protocol...
     */
    const char *err= "CLIENT_ERROR: Unsupported protocol\r\n";
    client->root->spool(client, err, strlen(err));
    client->root->drain(client);
    return MEMCACHED_PROTOCOL_ERROR_EVENT; /* Unsupported protocol */
  }

  return client->work(client, length, endptr);
}

/*
** **********************************************************************
** * PUBLIC INTERFACE
** * See protocol_handler.h for function description
** **********************************************************************
*/
struct memcached_protocol_st *memcached_protocol_create_instance(void)
{
  struct memcached_protocol_st *ret= calloc(1, sizeof(*ret));
  if (ret != NULL)
  {
    ret->recv= default_recv;
    ret->send= default_send;
    ret->drain= drain_output;
    ret->spool= spool_output;
    ret->input_buffer_size= 1 * 1024 * 1024;
    ret->input_buffer= malloc(ret->input_buffer_size);
    if (ret->input_buffer == NULL)
    {
      free(ret);
      ret= NULL;
      return NULL;
    }

    ret->buffer_cache= cache_create("protocol_handler",
                                     CHUNK_BUFFERSIZE + sizeof(struct chunk_st),
                                     0, NULL, NULL);
    if (ret->buffer_cache == NULL)
    {
      free(ret->input_buffer);
      free(ret);
    }
  }

  return ret;
}

void memcached_protocol_destroy_instance(struct memcached_protocol_st *instance)
{
  cache_destroy(instance->buffer_cache);
  free(instance->input_buffer);
  free(instance);
}

struct memcached_protocol_client_st *memcached_protocol_create_client(struct memcached_protocol_st *instance, memcached_socket_t sock)
{
  struct memcached_protocol_client_st *ret= calloc(1, sizeof(*ret));
  if (ret != NULL)
  {
    ret->root= instance;
    ret->sock= sock;
    ret->work= determine_protocol;
  }

  return ret;
}

void memcached_protocol_client_destroy(struct memcached_protocol_client_st *client)
{
  free(client);
}

memcached_protocol_event_t memcached_protocol_client_work(struct memcached_protocol_client_st *client)
{
  /* Try to send data and read from the socket */
  bool more_data= true;
  do
  {
    ssize_t len= client->root->recv(client,
                                    client->sock,
                                    client->root->input_buffer + client->input_buffer_offset,
                                    client->root->input_buffer_size - client->input_buffer_offset);

    if (len > 0)
    {
      /* Do we have the complete packet? */
      if (client->input_buffer_offset > 0)
      {
        memcpy(client->root->input_buffer, client->input_buffer,
               client->input_buffer_offset);
        len += (ssize_t)client->input_buffer_offset;

        /* @todo use buffer-cache! */
        free(client->input_buffer);
        client->input_buffer_offset= 0;
      }

      void *endptr;
      memcached_protocol_event_t events= client->work(client, &len, &endptr);
      if (events == MEMCACHED_PROTOCOL_ERROR_EVENT)
      {
        return MEMCACHED_PROTOCOL_ERROR_EVENT;
      }

      if (len > 0)
      {
        /* save the data for later on */
        /* @todo use buffer-cache */
        client->input_buffer= malloc((size_t)len);
        if (client->input_buffer == NULL)
        {
          client->error= ENOMEM;
          return MEMCACHED_PROTOCOL_ERROR_EVENT;
        }
        memcpy(client->input_buffer, endptr, (size_t)len);
        client->input_buffer_offset= (size_t)len;
        more_data= false;
      }
    }
    else if (len == 0)
    {
      /* Connection closed */
      drain_output(client);
      return MEMCACHED_PROTOCOL_ERROR_EVENT;
    }
    else
    {
      if (get_socket_errno() != EWOULDBLOCK)
      {
        client->error= get_socket_errno();
        /* mark this client as terminated! */
        return MEMCACHED_PROTOCOL_ERROR_EVENT;
      }
      more_data= false;
    }
  } while (more_data);

  if (!drain_output(client))
  {
    return MEMCACHED_PROTOCOL_ERROR_EVENT;
  }

  memcached_protocol_event_t ret= MEMCACHED_PROTOCOL_READ_EVENT;
  if (client->output)
    ret|= MEMCACHED_PROTOCOL_READ_EVENT;

  return ret;
}
