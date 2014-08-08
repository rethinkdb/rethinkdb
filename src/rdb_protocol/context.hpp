// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_CONTEXT_HPP_
#define RDB_PROTOCOL_CONTEXT_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "concurrency/promise.hpp"
#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/geo/lat_lon_types.hpp"
#include "perfmon/perfmon.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/changes.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/shards.hpp"
#include "rdb_protocol/wire_func.hpp"

class auth_semilattice_metadata_t;
class datum_range_t;
class ellipsoid_spec_t;
class extproc_pool_t;
class name_string_t;
class namespace_interface_t;
template <class> class semilattice_readwrite_view_t;
enum class sindex_rename_result_t;

enum class sindex_multi_bool_t;
enum class sindex_geo_bool_t;

namespace ql {
class configured_limits_t;

class db_t : public single_threaded_countable_t<db_t> {
public:
    db_t(uuid_u _id, const std::string &_name) : id(_id), name(_name) { }
    const uuid_u id;
    const std::string name;
};

class env_t;

}   // namespace ql

class base_table_t {
public:
    virtual const std::string &get_pkey() = 0;

    virtual counted_t<const ql::datum_t> read_row(ql::env_t *env,
        counted_t<const ql::datum_t> pval, bool use_outdated) = 0;
    virtual counted_t<ql::datum_stream_t> read_all(
        ql::env_t *env,
        const std::string &sindex,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,   /* the table's own name, for display purposes */
        const datum_range_t &range,
        sorting_t sorting,
        bool use_outdated) = 0;
    virtual counted_t<ql::datum_stream_t> read_row_changes(
        ql::env_t *env,
        counted_t<const ql::datum_t> pval,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name) = 0;
    virtual counted_t<ql::datum_stream_t> read_all_changes(
        ql::env_t *env,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name) = 0;
    virtual counted_t<ql::datum_stream_t> read_intersecting(
        ql::env_t *env,
        const std::string &sindex,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,
        bool use_outdated,
        const counted_t<const ql::datum_t> &query_geometry) = 0;
    virtual counted_t<ql::datum_stream_t> read_nearest(
        ql::env_t *env,
        const std::string &sindex,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,
        bool use_outdated,
        lat_lon_point_t center,
        double max_dist,
        uint64_t max_results,
        const ellipsoid_spec_t &geo_system,
        dist_unit_t dist_unit,
        const ql::configured_limits_t &limits) = 0;

    virtual counted_t<const ql::datum_t> write_batched_replace(ql::env_t *env,
        const std::vector<counted_t<const ql::datum_t> > &keys,
        const counted_t<ql::func_t> &func,
        return_changes_t _return_changes, durability_requirement_t durability) = 0;
    virtual counted_t<const ql::datum_t> write_batched_insert(ql::env_t *env,
        std::vector<counted_t<const ql::datum_t> > &&inserts,
        conflict_behavior_t conflict_behavior, return_changes_t return_changes,
        durability_requirement_t durability) = 0;
    virtual bool write_sync_depending_on_durability(ql::env_t *env,
        durability_requirement_t durability) = 0;

    virtual bool sindex_create(ql::env_t *env, const std::string &id,
        counted_t<ql::func_t> index_func, sindex_multi_bool_t multi,
        sindex_geo_bool_t geo) = 0;
    virtual bool sindex_drop(ql::env_t *env, const std::string &id) = 0;
    virtual sindex_rename_result_t sindex_rename(ql::env_t *env,
        const std::string &old_name, const std::string &new_name, bool overwrite) = 0;
    virtual std::vector<std::string> sindex_list(ql::env_t *env) = 0;
    virtual std::map<std::string, counted_t<const ql::datum_t> > sindex_status(
        ql::env_t *env, const std::set<std::string> &sindexes) = 0;

    /* This must be public */
    virtual ~base_table_t() { }
};

class reql_cluster_interface_t {
public:
    /* All of these methods return `true` on success and `false` on failure; if they
    fail, they will set `*error_out` to a description of the problem. They can all throw
    `interrupted_exc_t`.

    These methods are safe to call from any thread, and the calls can overlap
    concurrently in arbitrary ways. By the time a method returns, any changes it makes
    must be visible on every thread. */

    virtual bool db_create(const name_string_t &name,
            signal_t *interruptor, std::string *error_out) = 0;
    virtual bool db_drop(const name_string_t &name,
            signal_t *interruptor, std::string *error_out) = 0;
    virtual bool db_list(
            signal_t *interruptor,
            std::set<name_string_t> *names_out, std::string *error_out) = 0;
    virtual bool db_find(const name_string_t &name,
            signal_t *interruptor,
            counted_t<const ql::db_t> *db_out, std::string *error_out) = 0;

    /* `table_create()` won't return until the table is ready for reading */
    virtual bool table_create(const name_string_t &name, counted_t<const ql::db_t> db,
            const boost::optional<name_string_t> &primary_dc, bool hard_durability,
            const std::string &primary_key,
            signal_t *interruptor, std::string *error_out) = 0;
    virtual bool table_drop(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, std::string *error_out) = 0;
    virtual bool table_list(counted_t<const ql::db_t> db,
            signal_t *interruptor, std::set<name_string_t> *names_out,
            std::string *error_out) = 0;
    virtual bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, scoped_ptr_t<base_table_t> *table_out,
            std::string *error_out) = 0;

protected:
    virtual ~reql_cluster_interface_t() { }   // silence compiler warnings
};

class mailbox_manager_t;

class rdb_context_t {
public:
    // Used by unit tests.
    rdb_context_t();
    // Also used by unit tests.
    rdb_context_t(extproc_pool_t *_extproc_pool,
                  reql_cluster_interface_t *_cluster_interface);

    // The "real" constructor used outside of unit tests.
    rdb_context_t(extproc_pool_t *_extproc_pool,
                  mailbox_manager_t *_mailbox_manager,
                  reql_cluster_interface_t *_cluster_interface,
                  boost::shared_ptr<
                    semilattice_readwrite_view_t<
                        auth_semilattice_metadata_t> > _auth_metadata,
                  perfmon_collection_t *_global_stats,
                  const std::string &_reql_http_proxy);

    ~rdb_context_t();

    extproc_pool_t *extproc_pool;
    reql_cluster_interface_t *cluster_interface;

    boost::shared_ptr< semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
        auth_metadata;

    mailbox_manager_t *manager;

    perfmon_collection_t ql_stats_collection;
    perfmon_membership_t ql_stats_membership;
    perfmon_counter_t ql_ops_running;
    perfmon_membership_t ql_ops_running_membership;

    const std::string reql_http_proxy;

private:
    DISABLE_COPYING(rdb_context_t);
};

#endif /* RDB_PROTOCOL_CONTEXT_HPP_ */

