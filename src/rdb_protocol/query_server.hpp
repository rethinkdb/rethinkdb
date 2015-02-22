// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QUERY_SERVER_HPP_
#define RDB_PROTOCOL_QUERY_SERVER_HPP_

#include <set>
#include <string>

#include "arch/address.hpp"
#include "protob/protob.hpp"
#include "concurrency/one_per_thread.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {
template <class> class protob_t;
class query_id_t;
class query_cache_t;
}
class rdb_context_t;

// Overloads used by protob_server_t.
void make_empty_protob_bearer(ql::protob_t<Query> *request);
Query *underlying_protob_value(ql::protob_t<Query> *request);

class rdb_query_server_t : public query_handler_t {
public:
    rdb_query_server_t(const std::set<ip_address_t> &local_addresses,
                       int port,
                       rdb_context_t *_rdb_ctx,
                       uint32_t http_timeout_sec);

    http_app_t *get_http_app();
    int get_port() const;

    MUST_USE bool run_query(const ql::query_id_t &query_id,
                            const ql::protob_t<Query> &query,
                            Response *response_out,
                            ql::query_cache_t *query_cache,
                            signal_t *interruptor);
public:
    query_server_t server;
    rdb_context_t *rdb_ctx;
    one_per_thread_t<int> thread_counters;

    DISABLE_COPYING(rdb_query_server_t);
};

#endif /* RDB_PROTOCOL_QUERY_SERVER_HPP_ */
