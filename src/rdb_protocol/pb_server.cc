#include "rdb_protocol/pb_server.hpp"

query_server_t::query_server_t(int port, const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata, namespace_repo_t<rdb_protocol_t> * _ns_repo)
    : server(port, boost::bind(&query_server_t::handle, this, _1), INLINE),
      semilattice_metadata(_semilattice_metadata), ns_repo(_ns_repo)
{ }

Response query_server_t::handle(const Query &q) {
    Response res;
    res.set_token(q.token());

    query_language::type_checking_environment_t type_environment;

    query_language::backtrace_t root_backtrace;

    try {
        query_language::check_query_type(q, &type_environment, root_backtrace);
    } catch (query_language::bad_protobuf_exc_t &e) {
        res.set_status_code(-3);
        res.add_response("bad protocol buffer; client is buggy.");
        return res;
    } catch (query_language::bad_query_exc_t &e) {
        res.set_status_code(-2);
        res.add_response("bad query: " + e.message);
        res.add_response(e.backtrace.as_string());
        return res;
    }

    query_language::runtime_environment_t runtime_environment(ns_repo, semilattice_metadata);
    try {
        res.set_status_code(0);
        execute(q, &runtime_environment, &res, root_backtrace);
    } catch (query_language::runtime_exc_t &e) {
        res.set_status_code(-4);
        res.add_response("runtime exception: " + e.message);
        res.add_response(e.backtrace.as_string());
    }

    return res;
}
