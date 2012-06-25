#include "rdb_protocol/pb_server.hpp"

query_server_t::query_server_t(int port, const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata, namespace_repo_t<rdb_protocol_t> * _ns_repo) 
    : server(port, boost::bind(&query_server_t::handle, this, _1), INLINE),
      semilattice_metadata(_semilattice_metadata), ns_repo(_ns_repo)
{ }

Response query_server_t::handle(const Query &q) {
    Response res;
    if (!is_well_defined(q)) {
        res.set_status_code(-1);
        res.set_token(0);
        res.add_response("Ill defined message");
        return res;
    }

    query_language::variable_type_scope_t scope;

    if (!(get_type(q, &scope) == query_language::QUERY())) {
        res.set_status_code(-2);
        res.set_token(0);
        res.add_response("Message failed to type check");
        return res;
    }

    res.set_status_code(0);
    res.set_token(0);
    res.add_response("Foo");
    return res;
}
