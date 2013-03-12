// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/pb_server.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rpc/semilattice/view/field.hpp"

query_server_t::query_server_t(const std::set<ip_address_t> &local_addresses,
                               int port,
                               rdb_protocol_t::context_t *_ctx) :
    server(local_addresses, port, boost::bind(&query_server_t::handle, this, _1, _2),
           &on_unparsable_query, INLINE),
    ctx(_ctx), parser_id(generate_uuid()), thread_counters(0)
{ }

http_app_t *query_server_t::get_http_app() {
    return &server;
}

int query_server_t::get_port() const {
    return server.get_port();
}

static void put_backtrace(const query_language::backtrace_t &bt, Response3 *res_out) {
    std::vector<std::string> frames = bt.get_frames();
    for (size_t i = 0; i < frames.size(); ++i) {
        res_out->mutable_backtrace()->add_frame(frames[i]);
    }
}

Response3 on_unparsable_query(Query3 *q, std::string msg) {
    Response3 res;
    res.set_status_code(Response3::BROKEN_CLIENT);
    res.set_token( (q && q->has_token()) ? q->token() : -1);
    res.set_error_message(msg);
    return res;
}

Response3 query_server_t::handle(Query3 *q, context_t *query_context) {
    stream_cache_t *stream_cache = &query_context->stream_cache;
    signal_t *interruptor = query_context->interruptor;
    guarantee(interruptor);
    Response3 res;
    res.set_token(q->token());

    query_language::type_checking_environment_t type_environment;

    query_language::backtrace_t root_backtrace;
    bool is_deterministic;

    try {
        query_language::check_query_type(
            q, &type_environment, &is_deterministic, root_backtrace);
        boost::shared_ptr<js::runner_t> js_runner = boost::make_shared<js::runner_t>();
        int thread = get_thread_id();
        query_language::runtime_environment_t runtime_environment(
            ctx->pool_group, ctx->ns_repo,
            ctx->cross_thread_namespace_watchables[thread]->get_watchable(),
            ctx->cross_thread_database_watchables[thread]->get_watchable(),
            ctx->semilattice_metadata,
            ctx->directory_read_manager,
            js_runner, interruptor, ctx->machine_id);
        //[execute_query] will set the status code unless it throws
        execute_query(q, &runtime_environment, &res, scopes_t(),
                      root_backtrace, stream_cache);
    } catch (const query_language::broken_client_exc_t &e) {
        res.set_status_code(Response3::BROKEN_CLIENT);
        res.set_error_message(e.message);
        return res;
    } catch (const query_language::bad_query_exc_t &e) {
        res.set_status_code(Response3::BAD_QUERY);
        res.set_error_message(e.message);
        put_backtrace(e.backtrace, &res);
        return res;
    } catch (const query_language::runtime_exc_t &e) {
        res.set_status_code(Response3::RUNTIME_ERROR);
        res.set_error_message(e.message);
        put_backtrace(e.backtrace, &res);
    } catch (const interrupted_exc_t &e) {
        res.set_status_code(Response3::RUNTIME_ERROR);
        res.set_error_message("Query3 interrupted.  Did you shut down the server?");
    }

    return res;
}

query2_server_t::query2_server_t(const std::set<ip_address_t> &local_addresses,
                                 int port,
                                 rdb_protocol_t::context_t *_ctx) :
    server(local_addresses, port, boost::bind(&query2_server_t::handle, this, _1, _2),
           &on_unparsable_query2, INLINE),
    ctx(_ctx), parser_id(generate_uuid()), thread_counters(0)
{ }

http_app_t *query2_server_t::get_http_app() {
    return &server;
}

int query2_server_t::get_port() const {
    return server.get_port();
}

Response on_unparsable_query2(Query *q, std::string msg) {
    Response res;
    res.set_token( (q && q->has_token()) ? q->token() : -1);
    ql::fill_error(&res, Response::CLIENT_ERROR, msg);
    return res;
}

Response query2_server_t::handle(Query *q, context_t *query2_context) {
    ql::stream_cache2_t *stream_cache2 = &query2_context->stream_cache2;
    signal_t *interruptor = query2_context->interruptor;
    guarantee(interruptor);
    Response res;
    res.set_token(q->token());

    try {
        boost::shared_ptr<js::runner_t> js_runner = boost::make_shared<js::runner_t>();
        int thread = get_thread_id();
        guarantee(ctx->directory_read_manager);
        scoped_ptr_t<ql::env_t> env(
            new ql::env_t(
                ctx->pool_group, ctx->ns_repo,
                ctx->cross_thread_namespace_watchables[thread]->get_watchable(),
                ctx->cross_thread_database_watchables[thread]->get_watchable(),
                ctx->semilattice_metadata, ctx->directory_read_manager,
                js_runner, interruptor, ctx->machine_id));
        // `ql::run` will set the status code
        ql::run(q, &env, &res, stream_cache2);
    } catch (const interrupted_exc_t &e) {
        ql::fill_error(&res, Response::RUNTIME_ERROR,
                       "Query interrupted.  Did you shut down the server?");
    } catch (const std::exception &e) {
        ql::fill_error(&res, Response::RUNTIME_ERROR,
                       strprintf("Unexpected exception: %s\n", e.what()));
    }

    return res;
}
