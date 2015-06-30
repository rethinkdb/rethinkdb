// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/query_server.hpp"

#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "perfmon/perfmon.hpp"
#include "rdb_protocol/backtrace.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/profile.hpp"
#include "rdb_protocol/query_cache.hpp"
#include "rpc/semilattice/view/field.hpp"

rdb_query_server_t::rdb_query_server_t(const std::set<ip_address_t> &local_addresses,
                                       int port,
                                       rdb_context_t *_rdb_ctx) :
    server(_rdb_ctx, local_addresses, port, this, default_http_timeout_sec),
    rdb_ctx(_rdb_ctx),
    thread_counters(0) { }

http_app_t *rdb_query_server_t::get_http_app() {
    return &server;
}

int rdb_query_server_t::get_port() const {
    return server.get_port();
}

// Predeclaration for run, only used here
namespace ql {
    void run(ql::query_id_t &&query_id,
             protob_t<Query> q,
             Response *response_out,
             ql::query_cache_t *query_cache,
             signal_t *interruptor);
}

void rdb_query_server_t::run_query(ql::query_id_t &&query_id,
                                   const ql::protob_t<Query> &query,
                                   Response *response_out,
                                   ql::query_cache_t *query_cache,
                                   signal_t *interruptor) {
    guarantee(query_cache != NULL);
    guarantee(interruptor != NULL);
    try {
        scoped_perfmon_counter_t client_active(&rdb_ctx->stats.clients_active); // TODO: make this correct for parallelized queries
        guarantee(rdb_ctx->cluster_interface);
        // `ql::run` will set the status code
        ql::run(std::move(query_id), query, response_out, query_cache, interruptor);
    } catch (const interrupted_exc_t &ex) {
        throw; // Interruptions should be handled by our caller, who can provide context
#ifdef NDEBUG // In debug mode we crash, in release we send an error.
    } catch (const std::exception &e) {
        ql::fill_error(response_out,
                       Response::RUNTIME_ERROR,
                       Response::INTERNAL,
                       strprintf("Unexpected exception: %s\n", e.what()),
                       ql::backtrace_registry_t::EMPTY_BACKTRACE);
#endif // NDEBUG
    }

    rdb_ctx->stats.queries_per_sec.record();
    ++rdb_ctx->stats.queries_total;
}
