// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QUERY_LANGUAGE_HPP_
#define RDB_PROTOCOL_QUERY_LANGUAGE_HPP_

#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "utils.hpp"
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "http/json.hpp"
#include "rdb_protocol/bt.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/scope.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/environment.hpp"

void wait_for_rdb_table_readiness(namespace_repo_t<rdb_protocol_t> *ns_repo, namespace_id_t namespace_id, signal_t *interruptor, boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata) THROWS_ONLY(interrupted_exc_t);

#endif /* RDB_PROTOCOL_QUERY_LANGUAGE_HPP_ */
