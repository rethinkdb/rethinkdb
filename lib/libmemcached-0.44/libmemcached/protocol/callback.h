/*
 * Summary: Definition of the callback interface
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Trond Norbye
 */
#ifndef LIBMEMCACHEDPROTOCOL_CALLBACK_H
#define LIBMEMCACHEDPROTOCOL_CALLBACK_H

/**
 * Callback to send data back from a successful GET/GETQ/GETK/GETKQ command
 *
 * @param cookie Just pass along the cookie supplied in the callback
 * @param key What to insert as key in the reply
 * @param keylen The length of the key
 * @param body What to store in the body of the package
 * @param bodylen The number of bytes of the body
 * @param flags The flags stored with the item
 * @param cas The CAS value to insert into the response (should be 0
 *            if you don't care)
 */
typedef protocol_binary_response_status
(*memcached_binary_protocol_get_response_handler)(const void *cookie,
                                                  const void *key,
                                                  uint16_t keylen,
                                                  const void *body,
                                                  uint32_t bodylen,
                                                  uint32_t flags,
                                                  uint64_t cas);
/**
 * Callback to send data back from a STAT command
 *
 * @param cookie Just pass along the cookie supplied in the callback
 * @param key What to insert as key in the reply
 * @param keylen The length of the key
 * @param body What to store in the body of the package
 * @param bodylen The number of bytes of the body
 */
typedef protocol_binary_response_status
(*memcached_binary_protocol_stat_response_handler)(const void *cookie,
                                                   const void *key,
                                                   uint16_t keylen,
                                                   const void *body,
                                                   uint32_t bodylen);
/**
 * Callback to send data back from a VERSION command
 *
 * @param cookie Just pass along the cookie supplied in the callback
 * @param text The version string
 * @param length The number of bytes in the version string
 */
typedef protocol_binary_response_status
(*memcached_binary_protocol_version_response_handler)(const void *cookie,
                                                      const void *text,
                                                      uint32_t length);


/**
 * In the low level interface you need to format the response
 * packet yourself (giving you complete freedom :-)
 *
 * @param cookie Just pass along the cookie supplied in the callback
 * @param request Pointer to the request packet you are sending a reply to
 * @param response Pointer to the response packet to send
 *
 */
typedef protocol_binary_response_status (*memcached_binary_protocol_raw_response_handler)(const void *cookie,
                                                               protocol_binary_request_header *request,
                                                               protocol_binary_response_header *response);

/**
 * In the low lever interface you have to do most of the work by
 * yourself, but it also gives you a lot of freedom :-)
 * @param cookie identification for this connection, just pass it along to
 *               the response handler
 * @param header the command received over the wire. Never try to access
 *               <u>anything</u> outside the command.
 * @param resonse_handler call this function to send data back to the client
 */
typedef protocol_binary_response_status (*memcached_binary_protocol_command_handler)(const void *cookie,
                                                   protocol_binary_request_header *header,
                                                   memcached_binary_protocol_raw_response_handler response_handler);

/**
 * The raw interface to the packets is implemented in version 0. It contains
 * just an array with command handlers. The inxed in the array is the
 * com code.
 */
typedef struct {
   memcached_binary_protocol_command_handler comcode[256];
} memcached_binary_protocol_callback_v0_st;


/**
 * The first version of the callback struct containing all of the
 * documented commands in the initial release of the binary protocol
 * (aka. memcached 1.4.0).
 *
 * You might miss the Q commands (addq etc) but the response function
 * knows how to deal with them so you don't need to worry about that :-)
 */
typedef struct {
   /**
    * Add an item to the cache
    * @param cookie id of the client receiving the command
    * @param key the key to add
    * @param len the length of the key
    * @param val the value to store for the key (may be NIL)
    * @param vallen the length of the data
    * @param flags the flags to store with the key
    * @param exptime the expiry time for the key-value pair
    * @param cas the resulting cas for the add operation (if success)
    */
   protocol_binary_response_status (*add)(const void *cookie,
                                          const void *key,
                                          uint16_t keylen,
                                          const void* val,
                                          uint32_t vallen,
                                          uint32_t flags,
                                          uint32_t exptime,
                                          uint64_t *cas);

   /**
    * Append data to an <b>existing</b> key-value pair.
    *
    * @param cookie id of the client receiving the command
    * @param key the key to add data to
    * @param len the length of the key
    * @param val the value to append to the value
    * @param vallen the length of the data
    * @param cas the CAS in the request
    * @param result_cas the resulting cas for the append operation
    *
    */
   protocol_binary_response_status (*append)(const void *cookie,
                                             const void *key,
                                             uint16_t keylen,
                                             const void* val,
                                             uint32_t vallen,
                                             uint64_t cas,
                                             uint64_t *result_cas);

   /**
    * Decrement the value for a key
    *
    * @param cookie id of the client receiving the command
    * @param key the key to decrement the value for
    * @param len the length of the key
    * @param delta the amount to decrement
    * @param initial initial value to store (if the key doesn't exist)
    * @param expiration expiration time for the object (if the key doesn't exist)
    * @param cas the CAS in the request
    * @param result the result from the decrement
    * @param result_cas the cas of the item
    *
    */
   protocol_binary_response_status (*decrement)(const void *cookie,
                                                const void *key,
                                                uint16_t keylen,
                                                uint64_t delta,
                                                uint64_t initial,
                                                uint32_t expiration,
                                                uint64_t *result,
                                                uint64_t *result_cas);

   /**
    * Delete an existing key
    *
    * @param cookie id of the client receiving the command
    * @param key the key to delete
    * @param len the length of the key
    * @param cas the CAS in the request
    */
   protocol_binary_response_status (*delete)(const void *cookie,
                                             const void *key,
                                             uint16_t keylen,
                                             uint64_t cas);


   /**
    * Flush the cache
    *
    * @param cookie id of the client receiving the command
    * @param when when the cache should be flushed (0 == immediately)
    */
   protocol_binary_response_status (*flush)(const void *cookie,
                                            uint32_t when);



   /**
    * Get a key-value pair
    *
    * @param cookie id of the client receiving the command
    * @param key the key to get
    * @param len the length of the key
    * @param response_handler to send the result back to the client
    */
   protocol_binary_response_status (*get)(const void *cookie,
                                          const void *key,
                                          uint16_t keylen,
                                          memcached_binary_protocol_get_response_handler response_handler);

   /**
    * Increment the value for a key
    *
    * @param cookie id of the client receiving the command
    * @param key the key to increment the value on
    * @param len the length of the key
    * @param delta the amount to increment
    * @param initial initial value to store (if the key doesn't exist)
    * @param expiration expiration time for the object (if the key doesn't exist)
    * @param cas the CAS in the request
    * @param result the result from the decrement
    * @param result_cas the cas of the item
    *
    */
   protocol_binary_response_status (*increment)(const void *cookie,
                                                const void *key,
                                                uint16_t keylen,
                                                uint64_t delta,
                                                uint64_t initial,
                                                uint32_t expiration,
                                                uint64_t *result,
                                                uint64_t *result_cas);

   /**
    * The noop command was received. This is just a notification callback (the
    * response is automatically created).
    *
    * @param cookie id of the client receiving the command
    */
   protocol_binary_response_status (*noop)(const void *cookie);

   /**
    * Prepend data to an <b>existing</b> key-value pair.
    *
    * @param cookie id of the client receiving the command
    * @param key the key to prepend data to
    * @param len the length of the key
    * @param val the value to prepend to the value
    * @param vallen the length of the data
    * @param cas the CAS in the request
    * @param result-cas the cas id of the item
    *
    */
   protocol_binary_response_status (*prepend)(const void *cookie,
                                              const void *key,
                                              uint16_t keylen,
                                              const void* val,
                                              uint32_t vallen,
                                              uint64_t cas,
                                              uint64_t *result_cas);

   /**
    * The quit command was received. This is just a notification callback (the
    * response is automatically created).
    *
    * @param cookie id of the client receiving the command
    */
   protocol_binary_response_status (*quit)(const void *cookie);


   /**
    * Replace an <b>existing</b> item to the cache
    *
    * @param cookie id of the client receiving the command
    * @param key the key to replace the content for
    * @param len the length of the key
    * @param val the value to store for the key (may be NIL)
    * @param vallen the length of the data
    * @param flags the flags to store with the key
    * @param exptime the expiry time for the key-value pair
    * @param cas the cas id in the request
    * @param result_cas the cas id of the item
    */
   protocol_binary_response_status (*replace)(const void *cookie,
                                              const void *key,
                                              uint16_t keylen,
                                              const void* val,
                                              uint32_t vallen,
                                              uint32_t flags,
                                              uint32_t exptime,
                                              uint64_t cas,
                                              uint64_t *result_cas);


   /**
    * Set a key-value pair in the cache
    *
    * @param cookie id of the client receiving the command
    * @param key the key to insert
    * @param len the length of the key
    * @param val the value to store for the key (may be NIL)
    * @param vallen the length of the data
    * @param flags the flags to store with the key
    * @param exptime the expiry time for the key-value pair
    * @param cas the cas id in the request
    * @param result_cas the cas id of the new item
    */
   protocol_binary_response_status (*set)(const void *cookie,
                                          const void *key,
                                          uint16_t keylen,
                                          const void* val,
                                          uint32_t vallen,
                                          uint32_t flags,
                                          uint32_t exptime,
                                          uint64_t cas,
                                          uint64_t *result_cas);

   /**
    * Get status information
    *
    * @param cookie id of the client receiving the command
    * @param key the key to get status for (or NIL to request all status).
    *            Remember to insert the terminating packet if multiple
    *            packets should be returned.
    * @param keylen the length of the key
    * @param response_handler to send the result back to the client, but
    *                         don't send reply on success!
    *
    */
   protocol_binary_response_status (*stat)(const void *cookie,
                                           const void *key,
                                           uint16_t keylen,
                                           memcached_binary_protocol_stat_response_handler response_handler);

   /**
    * Get the version information
    *
    * @param cookie id of the client receiving the command
    * @param response_handler to send the result back to the client, but
    *                         don't send reply on success!
    *
    */
   protocol_binary_response_status (*version)(const void *cookie,
                                              memcached_binary_protocol_version_response_handler response_handler);
} memcached_binary_protocol_callback_v1_st;


/**
 * The version numbers for the different callback structures.
 */
typedef enum {
   /** Version 0 is a lowlevel interface that tries to maximize your freedom */
   MEMCACHED_PROTOCOL_HANDLER_V0= 0,
   /**
    * Version 1 abstracts more of the protocol details, and let you work at
    * a logical level
    */
   MEMCACHED_PROTOCOL_HANDLER_V1= 1,
} memcached_protocol_interface_version_t;

/**
 * Definition of the protocol callback structure.
 */
typedef struct {
   /**
    * The interface version you provide callbacks for.
    */
   memcached_protocol_interface_version_t interface_version;

   /**
    * Callback fired just before the command will be executed.
    *
    * @param cookie id of the client receiving the command
    * @param header the command header as received on the wire. If you look
    *               at the content you <b>must</b> ensure that you don't
    *               try to access beyond the end of the message.
    */
   void (*pre_execute)(const void *cookie,
                       protocol_binary_request_header *header);
   /**
    * Callback fired just after the command was exected (please note
    * that the data transfer back to the client is not finished at this
    * time).
    *
    * @param cookie id of the client receiving the command
    * @param header the command header as received on the wire. If you look
    *               at the content you <b>must</b> ensure that you don't
    *               try to access beyond the end of the message.
    */
   void (*post_execute)(const void *cookie,
                        protocol_binary_request_header *header);

   /**
    * Callback fired if no specialized callback is registered for this
    * specific command code.
    *
    * @param cookie id of the client receiving the command
    * @param header the command header as received on the wire. You <b>must</b>
    *               ensure that you don't try to access beyond the end of the
    *               message.
    * @param response_handler The response handler to send data back.
    */
   protocol_binary_response_status (*unknown)(const void *cookie,
                                              protocol_binary_request_header *header,
                                              memcached_binary_protocol_raw_response_handler response_handler);

   /**
    * The different interface levels we support. A pointer is used so the
    * size of the structure is fixed. You must ensure that the memory area
    * passed as the pointer is valid as long as you use the protocol handler.
    */
   union {
      memcached_binary_protocol_callback_v0_st v0;

      /**
       * The first version of the callback struct containing all of the
       * documented commands in the initial release of the binary protocol
       * (aka. memcached 1.4.0).
       */
      memcached_binary_protocol_callback_v1_st v1;
   } interface;
} memcached_binary_protocol_callback_st;

#endif
