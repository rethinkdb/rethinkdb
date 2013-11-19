// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PARSER_HPP_
#define RDB_PROTOCOL_PARSER_HPP_

#include "http/http.hpp"
#include "rdb_protocol/protocol.hpp"
#include "protocol_api.hpp"

template <class> class semilattice_read_view_t;

namespace rdb_protocol {

class query_http_app_t : public http_app_t {
public:
    query_http_app_t(const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata, namespace_repo_t<rdb_protocol_t> * _ns_repo);
    ~query_http_app_t();
    http_res_t handle(const http_req_t &);

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
    namespace_repo_t<rdb_protocol_t> *ns_repo;
    order_source_t order_source;
    cond_t on_destruct;
};

} // namespace rdb_protocol

#endif /* RDB_PROTOCOL_PARSER_HPP_ */
