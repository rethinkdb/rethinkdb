// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/context.hpp"

#include "rdb_protocol/datum.hpp"

rdb_context_t::rdb_context_t()
    : extproc_pool(NULL),
      cluster_interface(NULL),
      manager(NULL),
      ql_stats_membership(
          &get_global_perfmon_collection(), &ql_stats_collection, "query_language"),
      ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running"),
      reql_http_proxy()
{ }

rdb_context_t::rdb_context_t(
        extproc_pool_t *_extproc_pool,
        reql_cluster_interface_t *_cluster_interface)
    : extproc_pool(_extproc_pool),
      cluster_interface(_cluster_interface),
      manager(NULL),
      ql_stats_membership(
          &get_global_perfmon_collection(), &ql_stats_collection, "query_language"),
      ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running"),
      reql_http_proxy()
{ }

rdb_context_t::rdb_context_t(
        extproc_pool_t *_extproc_pool,
        mailbox_manager_t *_mailbox_manager,
        reql_cluster_interface_t *_cluster_interface,
        boost::shared_ptr< semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
            _auth_metadata,
        perfmon_collection_t *_global_stats,
        const std::string &_reql_http_proxy)
    : extproc_pool(_extproc_pool),
      cluster_interface(_cluster_interface),
      auth_metadata(_auth_metadata),
      manager(_mailbox_manager),
      ql_stats_membership(_global_stats, &ql_stats_collection, "query_language"),
      ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running"),
      reql_http_proxy(_reql_http_proxy)
{ }

rdb_context_t::~rdb_context_t() { }

