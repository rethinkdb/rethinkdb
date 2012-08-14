#ifndef RDB_PROTOCOL_PB_SERVER_HPP_
#define RDB_PROTOCOL_PB_SERVER_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "extproc/pool.hpp"
#include "protob/protob.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.hpp"

class query_server_t {
public:
    query_server_t(int port, extproc::pool_group_t *_pool_group, const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata, namespace_repo_t<rdb_protocol_t> * _ns_repo);

private:
    Response handle(Query *q, stream_cache_t *stream_cache);

    extproc::pool_group_t *pool_group;
    protob_server_t<Query, Response> server;
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
    namespace_repo_t<rdb_protocol_t> *ns_repo;
};

#endif /* RDB_PROTOCOL_PB_SERVER_HPP_ */
