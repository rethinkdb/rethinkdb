/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Definition of the callback interface to the protocol handler
 *
 * Author: Trond Norbye
 *
 */

#ifndef __LIBMEMCACHED_PROTOCOL_H__
#define __LIBMEMCACHED_PROTOCOL_H__

#include <sys/types.h>
#if !defined(__cplusplus)
# include <stdbool.h>
#endif

#include <libmemcached/platform.h>
#include <libmemcached/memcached/protocol_binary.h>
#include <libmemcached/visibility.h>
#include <libmemcached/protocol/callback.h>

/* Forward declarations */
/*
 * You should only access memcached_protocol_st from one thread!,
 * and never assume anything about the internal layout / sizes of the
 * structures.
 */
typedef struct memcached_protocol_st memcached_protocol_st;
typedef struct memcached_protocol_client_st memcached_protocol_client_st;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function the protocol handler should call to receive data.
 * This function should behave exactly like read(2)
 *
 * @param cookie a cookie used to represent a given client
 * @param fd the filedescriptor associated with the client
 * @param buf destination buffer
 * @param nbuf number of bytes to receive
 * @return the number of bytes copied into buf
 *         or -1 upon error (errno should contain more information)
 */
typedef ssize_t (*memcached_protocol_recv_func)(const void *cookie,
                                                memcached_socket_t fd,
                                                void *buf,
                                                size_t nbuf);

/**
 * Function the protocol handler should call to send data.
 * This function should behave exactly like write(2)
 *
 * @param cookie a cookie used to represent a given client
 * @param fd the filedescriptor associated with the client
 * @param buf the source buffer
 * @param nbuf number of bytes to send
 * @return the number of bytes sent
 *         or -1 upon error (errno should contain more information)
 */
typedef ssize_t (*memcached_protocol_send_func)(const void *cookie,
                                                memcached_socket_t fd,
                                                const void *buf,
                                                size_t nbuf);

/**
 * Create an instance of the protocol handler
 *
 * @return NULL if allocation of an instance fails
 */
LIBMEMCACHED_API
memcached_protocol_st *memcached_protocol_create_instance(void);

/**
 * Get the callbacks associated with a protocol handler instance
 * @return the callbacks currently used
 */
LIBMEMCACHED_API
memcached_binary_protocol_callback_st *memcached_binary_protocol_get_callbacks(memcached_protocol_st *instance);

/**
 * Set the callbacks to be used by the given protocol handler instance
 * @param instance the instance to update
 * @param callback the callbacks to use
 */
LIBMEMCACHED_API
void memcached_binary_protocol_set_callbacks(memcached_protocol_st *instance, memcached_binary_protocol_callback_st *callback);

/**
 * Should the library inspect the packages being sent and received and verify
 * that they are according to the specification? If it encounters an invalid
 * packet, it will return an EINVAL packet.
 *
 * @param instance the instance to update
 * @param enable true if you want the library to check packages, false otherwise
 */
LIBMEMCACHED_API
void memcached_binary_protocol_set_pedantic(memcached_protocol_st *instance, bool enable);

/**
 * Is the library inpecting each package?
 * @param instance the instance to check
 * @return true it the library is inspecting each package, false otherwise
 */
LIBMEMCACHED_API
bool memcached_binary_protocol_get_pedantic(memcached_protocol_st *instance);

/**
 * Destroy an instance of the protocol handler
 *
 * @param instance The instance to destroy
 */
LIBMEMCACHED_API
void memcached_protocol_destroy_instance(memcached_protocol_st *instance);

/**
 * Set the IO functions used by the instance to send and receive data. The
 * functions should behave like recv(3socket) and send(3socket).
 *
 * @param instance the instance to specify the IO functions for
 * @param recv the function to call for reciving data
 * @param send the function to call for sending data
 */
LIBMEMCACHED_API
void memached_protocol_set_io_functions(memcached_protocol_st *instance,
                                        memcached_protocol_recv_func recv,
                                        memcached_protocol_send_func send);


/**
 * Create a new client instance and associate it with a socket
 * @param instance the protocol instance to bind the client to
 * @param sock the client socket
 * @return NULL if allocation fails, otherwise an instance
 */
LIBMEMCACHED_API
memcached_protocol_client_st *memcached_protocol_create_client(memcached_protocol_st *instance, memcached_socket_t sock);

/**
 * Destroy a client handle.
 * The caller needs to close the socket accociated with the client
 * <b>before</b> calling this function. This function invalidates the
 * client memory area.
 *
 * @param client the client to destroy
 */
LIBMEMCACHED_API
void memcached_protocol_client_destroy(memcached_protocol_client_st *client);

/**
 * Error event means that the client encountered an error with the
 * connection so you should shut it down
 */
#define MEMCACHED_PROTOCOL_ERROR_EVENT 1
/**
 * Please notify when there is more data available to read
 */
#define MEMCACHED_PROTOCOL_READ_EVENT 2
/**
 * Please notify when it is possible to send more data
 */
#define MEMCACHED_PROTOCOL_WRITE_EVENT 4
/**
 * Backed paused the execution for this client
 */
#define MEMCACHED_PROTOCOL_PAUSE_EVENT 8

/**
 * The different events the client is interested in. This is a bitmask of
 * the constants defined above.
 */
typedef uint32_t memcached_protocol_event_t;

/**
 * Let the client do some work. This might involve reading / sending data
 * to/from the client, or perform callbacks to execute a command.
 * @param client the client structure to work on
 * @return The next event the protocol handler will be notified for
 */
LIBMEMCACHED_API
memcached_protocol_event_t memcached_protocol_client_work(memcached_protocol_client_st *client);

/**
 * Get the socket attached to a client handle
 * @param client the client to query
 * @return the socket handle
 */
LIBMEMCACHED_API
memcached_socket_t memcached_protocol_client_get_socket(memcached_protocol_client_st *client);

/**
 * Get the error id socket attached to a client handle
 * @param client the client to query for an error code
 * @return the OS error code from the client
 */
LIBMEMCACHED_API
int memcached_protocol_client_get_errno(memcached_protocol_client_st *client);

/**
 * Get a raw response handler for the given cookie
 * @param cookie the cookie passed along into the callback
 * @return the raw reponse handler you may use if you find
 *         the generic callback too limiting
 */
LIBMEMCACHED_API
memcached_binary_protocol_raw_response_handler memcached_binary_protocol_get_raw_response_handler(const void *cookie);

#ifdef __cplusplus
}
#endif

#endif
