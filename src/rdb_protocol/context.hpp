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

#include "concurrency/one_per_thread.hpp"
#include "concurrency/promise.hpp"
#include "containers/counted.hpp"
#include "containers/name_string.hpp"
#include "containers/scoped.hpp"
#include "containers/uuid.hpp"
#include "perfmon/perfmon.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/geo/lon_lat_types.hpp"
#include "rdb_protocol/shards.hpp"
#include "rdb_protocol/wire_func.hpp"

namespace auth {
    class user_context_t;
    class permissions_t;
}  // namespace auth

struct admin_err_t;

enum class return_changes_t {
    NO = 0,
    YES = 1,
    ALWAYS = 2
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        return_changes_t, int8_t,
        return_changes_t::NO, return_changes_t::ALWAYS);

class auth_semilattice_metadata_t;
class ellipsoid_spec_t;
class extproc_pool_t;
class name_string_t;
class namespace_interface_t;
template <class> class cross_thread_watchable_variable_t;
template <class> class semilattice_read_view_t;

enum class sindex_multi_bool_t { SINGLE = 0, MULTI = 1};
enum class sindex_geo_bool_t { REGULAR = 0, GEO = 1};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(sindex_multi_bool_t, int8_t,
        sindex_multi_bool_t::SINGLE, sindex_multi_bool_t::MULTI);
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(sindex_geo_bool_t, int8_t,
        sindex_geo_bool_t::REGULAR, sindex_geo_bool_t::GEO);

class sindex_config_t {
public:
    sindex_config_t() { }
    sindex_config_t(const ql::map_wire_func_t &_func, reql_version_t _func_version,
            sindex_multi_bool_t _multi, sindex_geo_bool_t _geo) :
        func(_func), func_version(_func_version), multi(_multi), geo(_geo) { }

    bool operator==(const sindex_config_t &o) const;
    bool operator!=(const sindex_config_t &o) const {
        return !(*this == o);
    }

    ql::map_wire_func_t func;
    reql_version_t func_version;
    sindex_multi_bool_t multi;
    sindex_geo_bool_t geo;
};
RDB_DECLARE_SERIALIZABLE(sindex_config_t);

class sindex_status_t {
public:
    sindex_status_t() :
        progress_numerator(0),
        progress_denominator(0),
        ready(true),
        outdated(false),
        start_time(-1) { }
    void accum(const sindex_status_t &other);
    double progress_numerator;
    double progress_denominator;
    bool ready;
    bool outdated;
    /* Note that `start_time` is only valid when `ready` is false, and while we
    serialize it it's relative to the local clock. If this becomes a problem in the
    future you can apply the same solution as in
        `void serialize(write_message_t *wm, const batchspec_t &batchspec)`,
    but that's relatively expensive. */
    microtime_t start_time;
};
RDB_DECLARE_SERIALIZABLE(sindex_status_t);

namespace ql {
class configured_limits_t;
class env_t;
class query_cache_t;
class db_t : public single_threaded_countable_t<db_t> {
public:
    db_t(uuid_u _id, const name_string_t &_name) : id(_id), name(_name) { }
    const uuid_u id;
    const name_string_t name;
};
} // namespace ql

class table_generate_config_params_t {
public:
    static table_generate_config_params_t make_default() {
        table_generate_config_params_t p;
        p.num_shards = 1;
        p.primary_replica_tag = name_string_t::guarantee_valid("default");
        p.num_replicas[p.primary_replica_tag] = 1;
        return p;
    }
    size_t num_shards;
    std::map<name_string_t, size_t> num_replicas;
    std::set<name_string_t> nonvoting_replica_tags;
    name_string_t primary_replica_tag;
};

enum class admin_identifier_format_t {
    /* Some parts of the code rely on the fact that `admin_identifier_format_t` can be
    mapped to `{0, 1}` using `static_cast`. */
    name = 0,
    uuid = 1
};

namespace ql {
    class reader_t;
}

class base_table_t : public slow_atomic_countable_t<base_table_t> {
public:
    virtual namespace_id_t get_id() const = 0;
    virtual const std::string &get_pkey() const = 0;

    virtual scoped_ptr_t<ql::reader_t> read_all_with_sindexes(
        ql::env_t *,
        const std::string &,
        ql::backtrace_id_t,
        const std::string &,
        const ql::datumspec_t &,
        sorting_t,
        read_mode_t) {
        r_sanity_fail();
    }

    virtual ql::datum_t read_row(ql::env_t *env,
        ql::datum_t pval, read_mode_t read_mode) = 0;
    virtual counted_t<ql::datum_stream_t> read_all(
        ql::env_t *env,
        const std::string &sindex,
        ql::backtrace_id_t bt,
        const std::string &table_name,   /* the table's own name, for display purposes */
        const ql::datumspec_t &datumspec,
        sorting_t sorting,
        read_mode_t read_mode) = 0;
    virtual counted_t<ql::datum_stream_t> read_changes(
        ql::env_t *env,
        const ql::changefeed::streamspec_t &ss,
        ql::backtrace_id_t bt) = 0;
    virtual counted_t<ql::datum_stream_t> read_intersecting(
        ql::env_t *env,
        const std::string &sindex,
        ql::backtrace_id_t bt,
        const std::string &table_name,
        read_mode_t read_mode,
        const ql::datum_t &query_geometry) = 0;
    virtual ql::datum_t read_nearest(
        ql::env_t *env,
        const std::string &sindex,
        const std::string &table_name,
        read_mode_t read_mode,
        lon_lat_point_t center,
        double max_dist,
        uint64_t max_results,
        const ellipsoid_spec_t &geo_system,
        dist_unit_t dist_unit,
        const ql::configured_limits_t &limits) = 0;

    virtual ql::datum_t write_batched_replace(
        ql::env_t *env,
        const std::vector<ql::datum_t> &keys,
        const counted_t<const ql::func_t> &func,
        return_changes_t _return_changes, durability_requirement_t durability) = 0;
    virtual ql::datum_t write_batched_insert(
        ql::env_t *env,
        std::vector<ql::datum_t> &&inserts,
        std::vector<bool> &&pkey_was_autogenerated,
        conflict_behavior_t conflict_behavior,
        boost::optional<counted_t<const ql::func_t> > conflict_func,
        return_changes_t return_changes,
        durability_requirement_t durability) = 0;
    virtual bool write_sync_depending_on_durability(
        ql::env_t *env,
        durability_requirement_t durability) = 0;

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

    /* From the user's point of view, many of these are methods on the table object. The
    reason they're internally defined on `reql_cluster_interface_t` rather than
    `base_table_t` is because their implementations fits better with the implementations
    of the other methods of `reql_cluster_interface_t` than `base_table_t`. */

    virtual bool db_create(
            auth::user_context_t const &user_context,
            const name_string_t &name,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool db_drop(
            auth::user_context_t const &user_context,
            const name_string_t &name,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool db_list(
            signal_t *interruptor,
            std::set<name_string_t> *names_out, admin_err_t *error_out) = 0;
    virtual bool db_find(const name_string_t &name,
            signal_t *interruptor,
            counted_t<const ql::db_t> *db_out, admin_err_t *error_out) = 0;
    virtual bool db_config(
            const counted_t<const ql::db_t> &db,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            admin_err_t *error_out) = 0;

    /* `table_create()` won't return until the table is ready for writing */
    virtual bool table_create(
            auth::user_context_t const &user_context,
            const name_string_t &name,
            counted_t<const ql::db_t> db,
            const table_generate_config_params_t &config_params,
            const std::string &primary_key,
            write_durability_t durability,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool table_drop(
            auth::user_context_t const &user_context,
            const name_string_t &name,
            counted_t<const ql::db_t> db,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool table_list(counted_t<const ql::db_t> db,
            signal_t *interruptor, std::set<name_string_t> *names_out,
            admin_err_t *error_out) = 0;
    virtual bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
            boost::optional<admin_identifier_format_t> identifier_format,
            signal_t *interruptor, counted_t<base_table_t> *table_out,
            admin_err_t *error_out) = 0;
    virtual bool table_estimate_doc_counts(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::env_t *env,
            std::vector<int64_t> *doc_counts_out,
            admin_err_t *error_out) = 0;
    virtual bool table_config(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            admin_err_t *error_out) = 0;
    virtual bool table_status(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            admin_err_t *error_out) = 0;

    virtual bool table_wait(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            table_readiness_t readiness,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool db_wait(
            counted_t<const ql::db_t> db,
            table_readiness_t readiness,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;

    virtual bool table_reconfigure(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool db_reconfigure(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;

    virtual bool table_emergency_repair(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            emergency_repair_mode_t,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;

    virtual bool table_rebalance(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool db_rebalance(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;

    virtual bool grant_global(
            auth::user_context_t const &user_context,
            auth::username_t username,
            ql::datum_t permissions,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool grant_database(
            auth::user_context_t const &user_context,
            database_id_t const &database_id,
            auth::username_t username,
            ql::datum_t permissions,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;
    virtual bool grant_table(
            auth::user_context_t const &user_context,
            database_id_t const &database_id,
            namespace_id_t const &table_id,
            auth::username_t username,
            ql::datum_t permissions,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out) = 0;

    virtual bool sindex_create(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            const sindex_config_t &config,
            signal_t *interruptor,
            admin_err_t *error_out) = 0;
    virtual bool sindex_drop(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            signal_t *interruptor,
            admin_err_t *error_out) = 0;
    virtual bool sindex_rename(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            const std::string &new_name,
            bool overwrite,
            signal_t *interruptor,
            admin_err_t *error_out) = 0;
    virtual bool sindex_list(
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            signal_t *interruptor,
            admin_err_t *error_out,
            std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
                *configs_and_statuses_out) = 0;

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
    rdb_context_t(
        extproc_pool_t *_extproc_pool,
        mailbox_manager_t *_mailbox_manager,
        reql_cluster_interface_t *_cluster_interface,
        boost::shared_ptr<semilattice_read_view_t<auth_semilattice_metadata_t>>
            auth_semilattice_view,
        perfmon_collection_t *global_stats,
        const std::string &_reql_http_proxy);

    ~rdb_context_t();

    extproc_pool_t *extproc_pool;
    reql_cluster_interface_t *cluster_interface;

    mailbox_manager_t *manager;

    const std::string reql_http_proxy;

    class stats_t {
    public:
        explicit stats_t(perfmon_collection_t *global_stats);

        perfmon_collection_t qe_stats_collection;
        perfmon_membership_t qe_stats_membership;
        perfmon_counter_t client_connections;
        perfmon_membership_t client_connections_membership;
        perfmon_counter_t clients_active;
        perfmon_membership_t clients_active_membership;
        perfmon_rate_monitor_t queries_per_sec;
        perfmon_membership_t queries_per_sec_membership;
        perfmon_counter_t queries_total;
        perfmon_membership_t queries_total_membership;
    private:
        DISABLE_COPYING(stats_t);
    } stats;

    std::set<ql::query_cache_t *> *get_query_caches_for_this_thread();

    clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> get_auth_watchable() const;

private:
    std::vector<std::unique_ptr<cross_thread_watchable_variable_t<
        auth_semilattice_metadata_t>>> m_cross_thread_auth_watchables;

    one_per_thread_t<std::set<ql::query_cache_t *> > query_caches;

    DISABLE_COPYING(rdb_context_t);
};

#endif /* RDB_PROTOCOL_CONTEXT_HPP_ */

