#include "rdb_protocol/pb_server.hpp"

query_server_t::query_server_t(int port, const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata, namespace_repo_t<rdb_protocol_t> * _ns_repo)
    : server(port, boost::bind(&query_server_t::handle, this, _1), INLINE),
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
    bool is_deterministic;

    try {
        query_language::check_query_type(q, &type_environment, &is_deterministic, root_backtrace);
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
    query_language::runtime_environment_t runtime_environment(ns_repo, semilattice_metadata, &interruptor);
    try {
        execute(q, &runtime_environment, &res, root_backtrace);
        res.set_status_code(Response::SUCCESS);
    } catch (query_language::runtime_exc_t &e) {
        res.set_status_code(Response::RUNTIME_ERROR);
        res.set_error_message(e.message);
        put_backtrace(e.backtrace, &res);
    }

    return res;
}
