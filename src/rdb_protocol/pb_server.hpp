// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PB_SERVER_HPP_
#define RDB_PROTOCOL_PB_SERVER_HPP_

#include <map>
#include <set>
#include <string>

#include "protob/protob.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/ql2.hpp"
#include "rdb_protocol/stream_cache.hpp"

namespace ql { template <class> class protob_t; }

// Overloads used by protob_server_t.
void make_empty_protob_bearer(ql::protob_t<Query> *request);
Query *underlying_protob_value(ql::protob_t<Query> *request);

class query2_server_t {
public:
    query2_server_t(const std::set<ip_address_t> &local_addresses, int port,
                    rdb_protocol_t::context_t *_ctx);

    http_app_t *get_http_app();

    int get_port() const;

    struct context_t {
        context_t() : interruptor(0) { }
        static const int32_t no_auth_magic_number = VersionDummy::V0_1;
        static const int32_t auth_magic_number = VersionDummy::V0_2;
        ql::stream_cache2_t stream_cache2;
        signal_t *interruptor;
    };
private:
    MUST_USE bool handle(ql::protob_t<Query> q,
                         Response *response_out,
                         context_t *query2_context);
    protob_server_t<ql::protob_t<Query>, Response, context_t> server;
    rdb_protocol_t::context_t *ctx;
    uuid_u parser_id;
    one_per_thread_t<int> thread_counters;

    DISABLE_COPYING(query2_server_t);
};


#endif /* RDB_PROTOCOL_PB_SERVER_HPP_ */
