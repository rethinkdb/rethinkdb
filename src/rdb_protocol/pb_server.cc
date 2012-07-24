#include "rdb_protocol/pb_server.hpp"

#include <boost/make_shared.hpp>

query_server_t::query_server_t(int port, extproc::pool_group_t *_pool_group, const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata, namespace_repo_t<rdb_protocol_t> * _ns_repo)
    : pool_group(_pool_group),
      server(port, boost::bind(&query_server_t::handle, this, _1), INLINE),
      semilattice_metadata(_semilattice_metadata), ns_repo(_ns_repo)
{ }

static void put_backtrace(const query_language::backtrace_t &bt, Response *res_out) {
    std::vector<std::string> frames = bt.get_frames();
    for (int i = 0; i < int(frames.size()); i++) {
        res_out->mutable_backtrace()->add_frame(frames[i]);
    }
}

Response query_server_t::handle(const Query &q) {
    Response res;
    res.set_token(q.token());

    query_language::type_checking_environment_t type_environment;

    query_language::backtrace_t root_backtrace;

    try {
        query_language::check_query_type(q, &type_environment, root_backtrace);
    } catch (query_language::bad_protobuf_exc_t &e) {
        res.set_status_code(Response::BAD_PROTOBUF);
        res.set_error_message("bad protocol buffer; client is buggy.");
        return res;
    } catch (query_language::bad_query_exc_t &e) {
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
            execute(q, &runtime_environment, &res, root_backtrace);
            res.set_status_code(Response::SUCCESS);
        } catch (query_language::runtime_exc_t &e) {
            res.set_status_code(Response::RUNTIME_ERROR);
            res.set_error_message(e.message);
            put_backtrace(e.backtrace, &res);
        }
    }
    // js_runner shouldn't survive the end of query execution (really we'd like
    // a scoped_ptr_t)
    guarantee(js_runner.unique());

    return res;
}
