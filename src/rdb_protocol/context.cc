// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/context.hpp"

#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "rdb_protocol/query_cache.hpp"
#include "rdb_protocol/datum.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "time.hpp"

bool sindex_config_t::operator==(const sindex_config_t &o) const {
    if (func_version != o.func_version || multi != o.multi || geo != o.geo) {
        return false;
    }
    /* This is kind of a hack--we compare the functions by serializing them and comparing
    the serialized values. */
    write_message_t wm1, wm2;
    serialize<cluster_version_t::CLUSTER>(&wm1, func);
    serialize<cluster_version_t::CLUSTER>(&wm2, o.func);
    vector_stream_t stream1, stream2;
    int res = send_write_message(&stream1, &wm1);
    guarantee(res == 0);
    res = send_write_message(&stream2, &wm2);
    guarantee(res == 0);
    return stream1.vector() == stream2.vector();
}

RDB_IMPL_SERIALIZABLE_4_SINCE_v2_1(sindex_config_t,
    func, func_version, multi, geo);

void sindex_status_t::accum(const sindex_status_t &other) {
    progress_numerator += other.progress_numerator;
    progress_denominator += other.progress_denominator;
    ready &= other.ready;
    start_time = std::min(start_time, other.start_time);
    rassert(outdated == other.outdated);
}

RDB_IMPL_SERIALIZABLE_5_FOR_CLUSTER(sindex_status_t,
    progress_numerator, progress_denominator, ready, outdated, start_time);

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
        boost::shared_ptr<semilattice_read_view_t<auth_semilattice_metadata_t>>
            auth_semilattice_view,
        perfmon_collection_t *global_stats,
        const std::string &_reql_http_proxy)
    : extproc_pool(_extproc_pool),
      cluster_interface(_cluster_interface),
      manager(_mailbox_manager),
      reql_http_proxy(_reql_http_proxy),
      stats(global_stats) {
    for (int thread = 0; thread < get_num_threads(); ++thread) {
        m_cross_thread_auth_watchables.emplace_back(
            new cross_thread_watchable_variable_t<auth_semilattice_metadata_t>(
                clone_ptr_t<semilattice_watchable_t<auth_semilattice_metadata_t>>(
                    new semilattice_watchable_t<auth_semilattice_metadata_t>(
                        auth_semilattice_view)),
                threadnum_t(thread)));
    }
}

rdb_context_t::~rdb_context_t() { }

std::set<ql::query_cache_t *> *rdb_context_t::get_query_caches_for_this_thread() {
    return query_caches.get();
}

clone_ptr_t<watchable_t<auth_semilattice_metadata_t>>
        rdb_context_t::get_auth_watchable() const{
    return m_cross_thread_auth_watchables[get_thread_id().threadnum]->get_watchable();
}
