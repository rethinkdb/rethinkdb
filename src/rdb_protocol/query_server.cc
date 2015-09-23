// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/query_server.hpp"

#include "perfmon/perfmon.hpp"
#include "rdb_protocol/backtrace.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/query_cache.hpp"
#include "rdb_protocol/query_params.hpp"
#include "rdb_protocol/response.hpp"

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

void rdb_query_server_t::run_query(ql::query_params_t *query_params,
                                   ql::response_t *response_out,
                                   signal_t *interruptor) {
    guarantee(interruptor != nullptr);
    guarantee(rdb_ctx->cluster_interface != nullptr);
    try {
        // TODO: make this perfmon correct now that we have parallelized queries
        scoped_perfmon_counter_t client_active(&rdb_ctx->stats.clients_active);

        switch (query_params->type) {
        case Query::START: {
            scoped_ptr_t<ql::query_cache_t::ref_t> query_ref =
                query_params->query_cache->create(query_params, interruptor);
            query_ref->fill_response(response_out);
        } break;
        case Query::CONTINUE: {
            scoped_ptr_t<ql::query_cache_t::ref_t> query_ref =
                query_params->query_cache->get(query_params, interruptor);
            query_ref->fill_response(response_out);
        } break;
        case Query::STOP: {
            query_params->query_cache->stop_query(*query_params);
            response_out->set_type(Response::SUCCESS_SEQUENCE);
        } break;
        case Query::NOREPLY_WAIT: {
            query_params->query_cache->noreply_wait(*query_params, interruptor);
            response_out->set_type(Response::WAIT_COMPLETE);
        } break;
        default: unreachable();
        }
    } catch (const ql::bt_exc_t &ex) {
        response_out->fill_error(ex.response_type, ex.error_type,
                                 ex.message, ex.bt_datum);
    } catch (const interrupted_exc_t &ex) {
        throw; // Interruptions should be handled by our caller, who can provide context
#ifdef NDEBUG // In debug mode we crash, in release we send an error.
    } catch (const std::exception &e) {
        response_out->fill_error(Response::RUNTIME_ERROR,
                                 Response::INTERNAL,
                                 strprintf("Unexpected exception: %s\n", e.what()),
                                 ql::backtrace_registry_t::EMPTY_BACKTRACE);
#endif // NDEBUG
    }

    rdb_ctx->stats.queries_per_sec.record();
    ++rdb_ctx->stats.queries_total;
}
