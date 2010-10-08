/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
#ifndef LIBMEMCACHED_PROTOCOL_BINARY_HANDLER_H
#define LIBMEMCACHED_PROTOCOL_BINARY_HANDLER_H

LIBMEMCACHED_LOCAL
bool memcached_binary_protocol_pedantic_check_request(const protocol_binary_request_header *request);

LIBMEMCACHED_LOCAL
bool memcached_binary_protocol_pedantic_check_response(const protocol_binary_request_header *request,
                                                       const protocol_binary_response_header *response);

LIBMEMCACHED_LOCAL
memcached_protocol_event_t memcached_binary_protocol_process_data(memcached_protocol_client_st *client, ssize_t *length, void **endptr);

#endif
