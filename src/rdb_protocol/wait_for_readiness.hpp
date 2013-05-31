// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_
#define RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "utils.hpp"

class cluster_semilattice_metadata_t;
template <class protocol_t> class base_namespace_repo_t;
class signal_t;
template <class metadata_t> class semilattice_readwrite_view_t;
struct rdb_protocol_t;
class uuid_u;

void wait_for_rdb_table_readiness(base_namespace_repo_t<rdb_protocol_t> *ns_repo, uuid_u namespace_id, signal_t *interruptor, boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata) THROWS_ONLY(interrupted_exc_t);

#endif /* RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_ */
