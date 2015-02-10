// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/context.hpp"

#include "rdb_protocol/query_cache.hpp"
#include "rdb_protocol/datum.hpp"
#include "time.hpp"

const char *rql_perfmon_name = "query_engine";

rdb_context_t::stats_t::stats_t(perfmon_collection_t *global_stats)
    : qe_stats_membership(global_stats, &qe_stats_collection, rql_perfmon_name),
      client_connections_membership(&qe_stats_collection,
                                    &client_connections, "client_connections"),
      clients_active_membership(&qe_stats_collection,
                                &clients_active, "clients_active"),
      queries_per_sec(secs_to_ticks(1)),
      queries_per_sec_membership(&qe_stats_collection,
                                 &queries_per_sec, "queries_per_sec"),
      queries_total_membership(&qe_stats_collection,
                               &queries_total, "queries_total") { }

rdb_context_t::rdb_context_t()
    : extproc_pool(nullptr),
      cluster_interface(nullptr),
      manager(nullptr),
      reql_http_proxy(),
      stats(&get_global_perfmon_collection()) { }

rdb_context_t::rdb_context_t(
        extproc_pool_t *_extproc_pool,
        reql_cluster_interface_t *_cluster_interface)
    : extproc_pool(_extproc_pool),
      cluster_interface(_cluster_interface),
      manager(nullptr),
      reql_http_proxy(),
      stats(&get_global_perfmon_collection()) { }

rdb_context_t::rdb_context_t(
        extproc_pool_t *_extproc_pool,
        mailbox_manager_t *_mailbox_manager,
        reql_cluster_interface_t *_cluster_interface,
        boost::shared_ptr< semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
            _auth_metadata,
        perfmon_collection_t *global_stats,
        const std::string &_reql_http_proxy)
    : extproc_pool(_extproc_pool),
      cluster_interface(_cluster_interface),
      auth_metadata(_auth_metadata),
      manager(_mailbox_manager),
      reql_http_proxy(_reql_http_proxy),
      stats(global_stats)
{ }

rdb_context_t::~rdb_context_t() { }

std::set<ql::query_cache_t *> *rdb_context_t::get_query_caches_for_this_thread() {
    return query_caches.get();
}
