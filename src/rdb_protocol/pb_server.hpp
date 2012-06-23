#ifndef RDB_PROTOCOL_PB_SERVER_HPP__
#define RDB_PROTOCOL_PB_SERVER_HPP__

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "protob/protob.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.hpp"
#include "rpc/mailbox/mailbox.hpp"

class query_server_t {
public:
    query_server_t(int port, const boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata, namespace_repo_t<rdb_protocol_t> * _ns_repo);

private:
    Response handle(const Query &q);

    protob_server_t<Query, Response> server;
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_metadata; 
    namespace_repo_t<rdb_protocol_t> *ns_repo;
};

#endif
