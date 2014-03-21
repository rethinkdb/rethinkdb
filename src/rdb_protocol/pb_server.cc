// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/pb_server.hpp"

#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/profile.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rpc/semilattice/view/field.hpp"

Response on_unparsable_query2(ql::protob_t<Query> q, std::string msg) {
    Response res;
    res.set_token((q.has() && q->has_token()) ? q->token() : -1);
    ql::fill_error(&res, Response::CLIENT_ERROR, msg);
    return res;
}

query2_server_t::query2_server_t(const std::set<ip_address_t> &local_addresses,
                                 int port,
                                 rdb_protocol_t::context_t *_ctx) :
    server(local_addresses,
           port,
           boost::bind(&query2_server_t::handle, this, _1, _2, _3),
           &on_unparsable_query2,
           _ctx->auth_metadata,
           INLINE),
    ctx(_ctx), parser_id(generate_uuid()), thread_counters(0)
{ }

http_app_t *query2_server_t::get_http_app() {
    return &server;
}

int query2_server_t::get_port() const {
    return server.get_port();
}

namespace ql {
    // Predeclaration for run, only used here
    void run(protob_t<Query> q,
             rdb_protocol_t::context_t *ctx,
             signal_t *interruptor,
             Response *res,
             stream_cache2_t *stream_cache2);
}

class scoped_ops_running_stat_t {
public:
    explicit scoped_ops_running_stat_t(perfmon_counter_t *_counter)
        : counter(_counter) {
        ++(*counter);
    }
    ~scoped_ops_running_stat_t() {
        --(*counter);
    }
private:
    perfmon_counter_t *counter;
    DISABLE_COPYING(scoped_ops_running_stat_t);
};

bool query2_server_t::handle(ql::protob_t<Query> q,
                             Response *response_out,
                             context_t *query2_context) {
    ql::stream_cache2_t *stream_cache2 = &query2_context->stream_cache2;
    signal_t *interruptor = query2_context->interruptor;
    guarantee(interruptor);
    response_out->set_token(q->token());

    counted_t<const ql::datum_t> noreply = static_optarg("noreply", q);
    bool response_needed = !(noreply.has() &&
         noreply->get_type() == ql::datum_t::type_t::R_BOOL &&
         noreply->as_bool());
    try {
        scoped_ops_running_stat_t stat(&ctx->ql_ops_running);
        guarantee(ctx->directory_read_manager);
        // `ql::run` will set the status code
        ql::run(q, ctx, interruptor, response_out, stream_cache2);
    } catch (const ql::exc_t &e) {
        fill_error(response_out, Response::COMPILE_ERROR, e.what(), e.backtrace());
    } catch (const ql::datum_exc_t &e) {
        fill_error(response_out, Response::COMPILE_ERROR, e.what(), ql::backtrace_t());
    } catch (const interrupted_exc_t &e) {
        ql::fill_error(response_out, Response::RUNTIME_ERROR,
                       "Query interrupted.  Did you shut down the server?");
    } catch (const std::exception &e) {
        ql::fill_error(response_out, Response::RUNTIME_ERROR,
                       strprintf("Unexpected exception: %s\n", e.what()));
    }

    return response_needed;
}

void make_empty_protob_bearer(ql::protob_t<Query> *request) {
    *request = ql::make_counted_query();
}

Query *underlying_protob_value(ql::protob_t<Query> *request) {
    return request->get();
}
