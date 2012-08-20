#include "rdb_protocol/pb_server.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "rdb_protocol/stream_cache.hpp"

query_server_t::query_server_t(int port, int http_port, extproc::pool_group_t *_pool_group, const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata, namespace_repo_t<rdb_protocol_t> * _ns_repo)
    : pool_group(_pool_group),
      server(port, http_port, boost::bind(&query_server_t::handle, this, _1, _2), &on_unparsable_query, INLINE),
      semilattice_metadata(_semilattice_metadata), ns_repo(_ns_repo)
{ }

static void put_backtrace(const query_language::backtrace_t &bt, Response *res_out) {
    std::vector<std::string> frames = bt.get_frames();
    for (size_t i = 0; i < frames.size(); ++i) {
        res_out->mutable_backtrace()->add_frame(frames[i]);
    }
}

Response on_unparsable_query(Query *q) {
    Response res;
    if (q->has_token()) {
        res.set_token(q->token());
    } else {
        res.set_token(-1);
    }

    res.set_status_code(Response::BROKEN_CLIENT);
    res.set_error_message("bad protocol buffer (failed to deserialize); client is buggy");
    return res;
}

Response query_server_t::handle(Query *q, stream_cache_t *stream_cache) {
    Response res;
    res.set_token(q->token());

    query_language::type_checking_environment_t type_environment;

    query_language::backtrace_t root_backtrace;
    bool is_deterministic;

    try {
        query_language::check_query_type(*q, &type_environment, &is_deterministic, root_backtrace);
    } catch (const query_language::broken_client_exc_t &e) {
        res.set_status_code(Response::BROKEN_CLIENT);
        res.set_error_message(e.message);
        return res;
    } catch (const query_language::bad_query_exc_t &e) {
        res.set_status_code(Response::BAD_QUERY);
        res.set_error_message(e.message);
        put_backtrace(e.backtrace, &res);
        return res;
    }

    cond_t interruptor;
    boost::shared_ptr<js::runner_t> js_runner = boost::make_shared<js::runner_t>();
    {
        query_language::runtime_environment_t runtime_environment(
            pool_group, ns_repo, semilattice_metadata, js_runner, &interruptor);
        try {
            //[execute] will set the status code unless it throws
            execute(q, &runtime_environment, &res, root_backtrace, stream_cache);
        } catch (const query_language::runtime_exc_t &e) {
            res.set_status_code(Response::RUNTIME_ERROR);
            res.set_error_message(e.message);
            put_backtrace(e.backtrace, &res);
        } catch (const query_language::broken_client_exc_t &e) {
            res.set_status_code(Response::BROKEN_CLIENT);
            res.set_error_message(e.message);
            return res;
        }
    }

    return res;
}
