#include "rdb_protocol/pb_server.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "query_measure.hpp"

query_server_t::query_server_t(int port, rdb_protocol_t::context_t *_ctx) :
    server(port, boost::bind(&query_server_t::handle, this, _1, _2),
           &on_unparsable_query, INLINE),
    ctx(_ctx), parser_id(generate_uuid()), thread_counters(0)
{ }

http_app_t *query_server_t::get_http_app() {
    return &server;
}

static void put_backtrace(const query_language::backtrace_t &bt, Response *res_out) {
    std::vector<std::string> frames = bt.get_frames();
    for (size_t i = 0; i < frames.size(); ++i) {
        res_out->mutable_backtrace()->add_frame(frames[i]);
    }
}

Response on_unparsable_query(Query *q, std::string msg) {
    Response res;
    res.set_status_code(Response::BROKEN_CLIENT);
    res.set_token( (q && q->has_token()) ? q->token() : -1);
    res.set_error_message(msg);
    return res;
}

Response query_server_t::handle(Query *q, context_t *query_context) {
    TICKVAR(qt_A);

    stream_cache_t *stream_cache = &query_context->stream_cache;
    signal_t *interruptor = query_context->interruptor;
    guarantee(interruptor);
    Response res;
    res.set_token(q->token());

    TICKVAR(qt_B);

    query_language::type_checking_environment_t type_environment;

    TICKVAR(qt_C);

    query_language::backtrace_t root_backtrace;

    TICKVAR(qt_D);

    bool is_deterministic;
    try {
        TICKVAR(qt_E);

        query_language::check_query_type(
            q, &type_environment, &is_deterministic, root_backtrace);

        TICKVAR(qt_F);

        boost::shared_ptr<js::runner_t> js_runner = boost::make_shared<js::runner_t>();
        int thread = get_thread_id();

        TICKVAR(qt_G);

        query_language::runtime_environment_t runtime_environment(
            ctx->pool_group, ctx->ns_repo,
            ctx->cross_thread_namespace_watchables[thread]->get_watchable(),
            ctx->cross_thread_database_watchables[thread]->get_watchable(),
            ctx->semilattice_metadata,
            ctx->directory_read_manager,
            js_runner, interruptor, ctx->machine_id);

        TICKVAR(qt_H);

        //[execute_query] will set the status code unless it throws
        execute_query(q, &runtime_environment, &res, scopes_t(),
                      root_backtrace, stream_cache);

        TICKVAR(qt_I);

        // logRQM("QUERY_MEASURE: A %ld B %ld C %ld D %ld E %ld F %ld G %ld H %ld I",
        //        qt_B - qt_A, qt_C - qt_B, qt_D - qt_C, qt_E - qt_D, qt_F - qt_E, qt_G - qt_F, qt_H - qt_G, qt_I - qt_H);
    } catch (const query_language::broken_client_exc_t &e) {
        res.set_status_code(Response::BROKEN_CLIENT);
        res.set_error_message(e.message);
        return res;
    } catch (const query_language::bad_query_exc_t &e) {
        res.set_status_code(Response::BAD_QUERY);
        res.set_error_message(e.message);
        put_backtrace(e.backtrace, &res);
        return res;
    } catch (const query_language::runtime_exc_t &e) {
        res.set_status_code(Response::RUNTIME_ERROR);
        res.set_error_message(e.message);
        put_backtrace(e.backtrace, &res);
    } catch (const interrupted_exc_t &e) {
        res.set_status_code(Response::RUNTIME_ERROR);
        res.set_error_message("Query interrupted.  Did you shut down the server?");
    }

    return res;
}
