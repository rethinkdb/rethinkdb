// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLIENT_PROTOCOL_JSON_HPP_
#define CLIENT_PROTOCOL_JSON_HPP_

#include <stdint.h>

#include "arch/types.hpp"
#include "containers/scoped.hpp"
#include "rapidjson/stringbuffer.h"

class signal_t;

namespace ql {
class response_t;
class query_cache_t;
class query_params_t;
}

// This is a class rather than a namespace so we can templatize the connection loop on
// the protocol type.
class json_protocol_t {
public:
    static scoped_ptr_t<ql::query_params_t> parse_query_from_buffer(
            scoped_array_t<char> &&mutable_buffer, size_t offset,
            ql::query_cache_t *query_cache, int64_t token,
            ql::response_t *error_out);

    static scoped_ptr_t<ql::query_params_t> parse_query(tcp_conn_t *conn,
                                                        signal_t *interruptor,
                                                        ql::query_cache_t *query_cache);

    // Used by the HTTP ReQL server to write the query response into the HTTP response
    static void write_response_to_buffer(ql::response_t *response,
                                         rapidjson::StringBuffer *buffer_out);

    static void send_response(ql::response_t *response,
                              int64_t token,
                              tcp_conn_t *conn,
                              signal_t *interruptor);
};

#endif // CLIENT_PROTOCOL_JSON_HPP_
