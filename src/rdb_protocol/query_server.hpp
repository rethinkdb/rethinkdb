// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QUERY_SERVER_HPP_
#define RDB_PROTOCOL_QUERY_SERVER_HPP_

#include <set>

#include "arch/address.hpp"
#include "concurrency/one_per_thread.hpp"
#include "client_protocol/server.hpp"

namespace ql {
class query_params_t;
class query_cache_t;
class response_t;
}

class rdb_context_t;

class rdb_query_server_t : public query_handler_t {
public:
    rdb_query_server_t(const std::set<ip_address_t> &local_addresses,
                       int port,
                       rdb_context_t *_rdb_ctx);

    http_app_t *get_http_app();
    int get_port() const;

    void run_query(ql::query_params_t *query_params,
                   ql::response_t *response_out,
                   signal_t *interruptor);
public:
    static const uint32_t default_http_timeout_sec = 300;

    query_server_t server;
    rdb_context_t *rdb_ctx;
    one_per_thread_t<int> thread_counters;

    DISABLE_COPYING(rdb_query_server_t);
};

#endif /* RDB_PROTOCOL_QUERY_SERVER_HPP_ */
