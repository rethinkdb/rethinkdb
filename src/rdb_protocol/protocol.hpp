// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PROTOCOL_HPP_
#define RDB_PROTOCOL_PROTOCOL_HPP_

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "btree/secondary_operations.hpp"
#include "clustering/administration/auth/user_context.hpp"
#include "concurrency/cond_var.hpp"
#include "perfmon/perfmon.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/context.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/erase_range.hpp"
#include "rdb_protocol/geo/ellipsoid.hpp"
#include "rdb_protocol/geo/lon_lat_types.hpp"
#include "rdb_protocol/optargs.hpp"
#include "rdb_protocol/shards.hpp"
#include "region/region.hpp"
#include "repli_timestamp.hpp"
#include "rpc/mailbox/typed.hpp"

class store_t;
class buf_lock_t;
template <class> class clone_ptr_t;
template <class> class cross_thread_watchable_variable_t;
class cross_thread_signal_t;
struct secondary_index_t;
class traversal_progress_combiner_t;
template <class> class watchable_t;

enum class profile_bool_t {
    PROFILE,
    DONT_PROFILE
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        profile_bool_t, int8_t,
        profile_bool_t::PROFILE, profile_bool_t::DONT_PROFILE);

enum class point_write_result_t {
    STORED,
    DUPLICATE
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        point_write_result_t, int8_t,
        point_write_result_t::STORED, point_write_result_t::DUPLICATE);

enum class point_delete_result_t {
    DELETED,
    MISSING
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        point_delete_result_t, int8_t,
        point_delete_result_t::DELETED, point_delete_result_t::MISSING);

class key_le_t {
public:
    explicit key_le_t(sorting_t _sorting) : sorting(_sorting) { }
    bool is_le(const store_key_t &key1, const store_key_t &key2) const {
        return (!reversed(sorting) && key1 <= key2)
            || (reversed(sorting) && key2 <= key1);
    }
private:
    sorting_t sorting;
};

namespace ql {
class datum_t;
class primary_readgen_t;
class readgen_t;
class sindex_readgen_t;
class intersecting_readgen_t;
} // namespace ql

namespace rdb_protocol {

void resume_construct_sindex(
        const uuid_u &sindex_to_construct,
        const key_range_t &construct_range,
        store_t *store,
        auto_drainer_t::lock_t store_keepalive)
    THROWS_NOTHING;

} // namespace rdb_protocol

struct point_read_response_t {
    ql::datum_t data;
    point_read_response_t() { }
    explicit point_read_response_t(ql::datum_t _data)
        : data(_data) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(point_read_response_t);

struct shard_stamp_info_t {
    uint64_t stamp;
    region_t shard_region;
    // The starting points of the reads (assuming left to right traversal)
    store_key_t last_read_start;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(shard_stamp_info_t);

struct changefeed_stamp_response_t {
    changefeed_stamp_response_t() { }
    // The `uuid_u` below is the uuid of the changefeed `server_t`.  (We have
    // different timestamps for each `server_t` because they're on different
    // servers and don't synchronize with each other.)  If this is empty it
    // means the feed was aborted.
    boost::optional<std::map<uuid_u, shard_stamp_info_t> > stamp_infos;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_stamp_response_t);

struct rget_read_response_t {
    boost::optional<changefeed_stamp_response_t> stamp_response;
    ql::result_t result;
    reql_version_t reql_version;

    rget_read_response_t()
        : reql_version(reql_version_t::EARLIEST) { }
    explicit rget_read_response_t(const ql::exc_t &ex)
        : result(ex), reql_version(reql_version_t::EARLIEST) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(rget_read_response_t);

struct nearest_geo_read_response_t {
    typedef std::pair<double, ql::datum_t> dist_pair_t;
    typedef std::vector<dist_pair_t> result_t;
    boost::variant<result_t, ql::exc_t> results_or_error;

    nearest_geo_read_response_t() { }
    explicit nearest_geo_read_response_t(result_t &&_results) {
        // Implement "move" on _results through std::vector<...>::swap to avoid
        // problems with boost::variant not supporting move assignment.
        results_or_error = result_t();
        boost::get<result_t>(&results_or_error)->swap(_results);
    }
    explicit nearest_geo_read_response_t(const ql::exc_t &_error)
        : results_or_error(_error) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(nearest_geo_read_response_t);

void scale_down_distribution(size_t result_limit, std::map<store_key_t, int64_t> *key_counts);

struct distribution_read_response_t {
    // Supposing the map has keys:
    // k1, k2 ... kn
    // with k1 < k2 < .. < kn
    // Then k1 == left_key
    // and key_counts[ki] = the number of keys in [ki, ki+1) if i < n
    // key_counts[kn] = the number of keys in [kn, right_key)
    region_t region;
    std::map<store_key_t, int64_t> key_counts;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(distribution_read_response_t);

struct changefeed_subscribe_response_t {
    changefeed_subscribe_response_t() { }
    std::set<uuid_u> server_uuids;
    std::set<ql::changefeed::server_t::addr_t> addrs;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_subscribe_response_t);

struct changefeed_limit_subscribe_response_t {
    int64_t shards;
    std::vector<ql::changefeed::server_t::limit_addr_t> limit_addrs;

    changefeed_limit_subscribe_response_t() { }
    changefeed_limit_subscribe_response_t(
        int64_t _shards, decltype(limit_addrs) _limit_addrs)
        : shards(_shards), limit_addrs(std::move(_limit_addrs)) { }
};
RDB_DECLARE_SERIALIZABLE(changefeed_limit_subscribe_response_t);

struct changefeed_point_stamp_response_t {
    changefeed_point_stamp_response_t() { }
    // The `uuid_u` below is the uuid of the changefeed `server_t`.  (We have
    // different timestamps for each `server_t` because they're on different
    // servers and don't synchronize with each other.)
    struct valid_response_t {
        std::pair<uuid_u, uint64_t> stamp;
        ql::datum_t initial_val;
    };
    // If this is empty it means the feed was aborted.
    boost::optional<valid_response_t> resp;
};
RDB_DECLARE_SERIALIZABLE(changefeed_point_stamp_response_t::valid_response_t);
RDB_DECLARE_SERIALIZABLE(changefeed_point_stamp_response_t);

struct dummy_read_response_t {
    // dummy read always succeeds
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(dummy_read_response_t);

struct read_response_t {
    typedef boost::variant<point_read_response_t,
                           rget_read_response_t,
                           nearest_geo_read_response_t,
                           changefeed_subscribe_response_t,
                           changefeed_limit_subscribe_response_t,
                           changefeed_stamp_response_t,
                           changefeed_point_stamp_response_t,
                           distribution_read_response_t,
                           dummy_read_response_t> variant_t;
    variant_t response;
    profile::event_log_t event_log;
    size_t n_shards;

    read_response_t() { }
    explicit read_response_t(const variant_t &r)
        : response(r) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(read_response_t);

class point_read_t {
public:
    point_read_t() { }
    explicit point_read_t(const store_key_t& _key) : key(_key) { }

    store_key_t key;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(point_read_t);

// `dummy_read_t` can be used to poll for table readiness - it will go through all
// the clustering layers, but is a no-op in the protocol layer.
class dummy_read_t {
public:
    dummy_read_t() : region(region_t::universe()) { }
    region_t region;
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(dummy_read_t);

struct sindex_rangespec_t {
    sindex_rangespec_t() { }
    sindex_rangespec_t(const std::string &_id,
                       // This is the region in the sindex keyspace.  It's
                       // sometimes smaller than the datum range below when
                       // dealing with truncated keys.
                       boost::optional<region_t> _region,
                       ql::datumspec_t _datumspec,
                       require_sindexes_t _require_sindex_val = require_sindexes_t::NO)
        : id(_id),
          region(std::move(_region)),
          datumspec(std::move(_datumspec)),
          require_sindex_val(_require_sindex_val){ }
    std::string id; // What sindex we're using.
    // What keyspace we're currently operating on.  If empty, assume the
    // original range and create the readgen on the shards.
    boost::optional<region_t> region;
    // For dealing with truncation and `get_all`.
    ql::datumspec_t datumspec;
    // For forcing sindex values to be returned with sorting::UNORDERED, used in eq_join.
    require_sindexes_t require_sindex_val;
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_rangespec_t);

struct changefeed_stamp_t {
    changefeed_stamp_t() : region(region_t::universe()) { }
    explicit changefeed_stamp_t(ql::changefeed::client_t::addr_t _addr)
        : addr(std::move(_addr)), region(region_t::universe()) { }
    ql::changefeed::client_t::addr_t addr;
    region_t region;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_stamp_t);

class rget_read_t {
public:
    rget_read_t() : batchspec(ql::batchspec_t::empty()) { }

    rget_read_t(boost::optional<changefeed_stamp_t> &&_stamp,
                region_t _region,
                boost::optional<std::map<region_t, store_key_t> > _hints,
                boost::optional<std::map<store_key_t, uint64_t> > _primary_keys,
                ql::global_optargs_t _optargs,
                auth::user_context_t user_context,
                std::string _table_name,
                ql::batchspec_t _batchspec,
                std::vector<ql::transform_variant_t> _transforms,
                boost::optional<ql::terminal_variant_t> &&_terminal,
                boost::optional<sindex_rangespec_t> &&_sindex,
                sorting_t _sorting)
    : stamp(std::move(_stamp)),
      region(std::move(_region)),
      hints(std::move(_hints)),
      primary_keys(std::move(_primary_keys)),
      optargs(std::move(_optargs)),
      m_user_context(std::move(user_context)),
      table_name(std::move(_table_name)),
      batchspec(std::move(_batchspec)),
      transforms(std::move(_transforms)),
      terminal(std::move(_terminal)),
      sindex(std::move(_sindex)),
      sorting(std::move(_sorting)) { }

    boost::optional<changefeed_stamp_t> stamp;

    region_t region; // We need this even for sindex reads due to sharding.
    boost::optional<region_t> current_shard;
    boost::optional<std::map<region_t, store_key_t> > hints;

    // The `uint64_t`s here are counts.  This map is used to make `get_all` more
    // efficient, and it's legal to pass duplicate keys to `get_all`.
    boost::optional<std::map<store_key_t, uint64_t> > primary_keys;

    ql::global_optargs_t optargs;
    auth::user_context_t m_user_context;
    std::string table_name;
    ql::batchspec_t batchspec; // used to size batches

    // We use these two for lazy maps, reductions, etc.
    std::vector<ql::transform_variant_t> transforms;
    boost::optional<ql::terminal_variant_t> terminal;

    // This is non-empty if we're doing an sindex read.
    // TODO: `read_t` should maybe be multiple types.  Determining the type
    // of read by branching on whether an optional is full sucks.
    boost::optional<sindex_rangespec_t> sindex;

    sorting_t sorting; // Optional sorting info (UNORDERED means no sorting).
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(rget_read_t);

class intersecting_geo_read_t {
public:
    intersecting_geo_read_t() : batchspec(ql::batchspec_t::empty()) { }

    intersecting_geo_read_t(
        boost::optional<changefeed_stamp_t> &&_stamp,
        region_t _region,
        ql::global_optargs_t _optargs,
        auth::user_context_t user_context,
        std::string _table_name,
        ql::batchspec_t _batchspec,
        std::vector<ql::transform_variant_t> _transforms,
        boost::optional<ql::terminal_variant_t> &&_terminal,
        sindex_rangespec_t &&_sindex,
        ql::datum_t _query_geometry)
        : stamp(std::move(_stamp)),
          region(std::move(_region)),
          optargs(std::move(_optargs)),
          m_user_context(std::move(user_context)),
          table_name(std::move(_table_name)),
          batchspec(std::move(_batchspec)),
          transforms(std::move(_transforms)),
          terminal(std::move(_terminal)),
          sindex(std::move(_sindex)),
          query_geometry(std::move(_query_geometry)) { }

    boost::optional<changefeed_stamp_t> stamp;

    region_t region; // Primary key range. We need this because of sharding.
    ql::global_optargs_t optargs;
    auth::user_context_t m_user_context;
    std::string table_name;
    ql::batchspec_t batchspec; // used to size batches

    // We use these two for lazy maps, reductions, etc.
    std::vector<ql::transform_variant_t> transforms;
    boost::optional<ql::terminal_variant_t> terminal;

    sindex_rangespec_t sindex;

    ql::datum_t query_geometry; // Tested for intersection
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(intersecting_geo_read_t);

class nearest_geo_read_t {
public:
    nearest_geo_read_t() { }

    nearest_geo_read_t(
            const region_t &_region,
            lon_lat_point_t _center,
            double _max_dist,
            uint64_t _max_results,
            const ellipsoid_spec_t &_geo_system,
            const std::string &_table_name,
            const std::string &_sindex_id,
            ql::global_optargs_t _optargs,
            auth::user_context_t user_context)
        : optargs(std::move(_optargs)),
          m_user_context(std::move(user_context)),
          center(_center),
          max_dist(_max_dist),
          max_results(_max_results),
          geo_system(_geo_system),
          region(_region),
          table_name(_table_name),
          sindex_id(_sindex_id) { }

    ql::global_optargs_t optargs;
    auth::user_context_t m_user_context;

    lon_lat_point_t center;
    double max_dist;
    uint64_t max_results;
    ellipsoid_spec_t geo_system;

    region_t region; // We need this even for sindex reads due to sharding.
    std::string table_name;

    std::string sindex_id;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(nearest_geo_read_t);

class distribution_read_t {
public:
    distribution_read_t()
        : max_depth(0), result_limit(0), region(region_t::universe())
    { }
    distribution_read_t(int _max_depth, size_t _result_limit)
        : max_depth(_max_depth), result_limit(_result_limit),
          region(region_t::universe())
    { }

    int max_depth;
    size_t result_limit;
    region_t region;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(distribution_read_t);

struct changefeed_subscribe_t {
    changefeed_subscribe_t() { }
    explicit changefeed_subscribe_t(ql::changefeed::client_t::addr_t _addr)
        : addr(_addr), shard_region(region_t::universe()) { }
    ql::changefeed::client_t::addr_t addr;
    region_t shard_region;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_subscribe_t);

struct changefeed_limit_subscribe_t {
    changefeed_limit_subscribe_t() { }
    explicit changefeed_limit_subscribe_t(
        ql::changefeed::client_t::addr_t _addr,
        uuid_u _uuid,
        ql::changefeed::keyspec_t::limit_t _spec,
        std::string _table,
        ql::global_optargs_t _optargs,
        auth::user_context_t user_context,
        region_t pkey_region)
        : addr(std::move(_addr)),
          uuid(std::move(_uuid)),
          spec(std::move(_spec)),
          table(std::move(_table)),
          optargs(std::move(_optargs)),
          m_user_context(std::move(user_context)),
          region(std::move(pkey_region)) { }
    ql::changefeed::client_t::addr_t addr;
    uuid_u uuid;
    ql::changefeed::keyspec_t::limit_t spec;
    std::string table;
    ql::global_optargs_t optargs;
    auth::user_context_t m_user_context;
    region_t region;
    boost::optional<region_t> current_shard;
};
RDB_DECLARE_SERIALIZABLE(changefeed_limit_subscribe_t);

// This is a separate class because it needs to shard and unshard differently.
struct changefeed_point_stamp_t {
    ql::changefeed::client_t::addr_t addr;
    store_key_t key;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_point_stamp_t);

struct read_t {
    typedef boost::variant<point_read_t,
                           rget_read_t,
                           intersecting_geo_read_t,
                           nearest_geo_read_t,
                           changefeed_subscribe_t,
                           changefeed_stamp_t,
                           changefeed_limit_subscribe_t,
                           changefeed_point_stamp_t,
                           distribution_read_t,
                           dummy_read_t> variant_t;

    variant_t read;
    profile_bool_t profile;
    read_mode_t read_mode;

    region_t get_region() const THROWS_NOTHING;
    // Returns true if the read has any operation for this region.  Returns
    // false if read_out has not been touched.
    bool shard(const region_t &region,
               read_t *read_out) const THROWS_NOTHING;

    void unshard(read_response_t *responses, size_t count,
                 read_response_t *response, rdb_context_t *ctx,
                 signal_t *interruptor) const
        THROWS_ONLY(interrupted_exc_t);

    read_t() : profile(profile_bool_t::DONT_PROFILE), read_mode(read_mode_t::SINGLE) { }
    template<class T>
    read_t(T &&_read, profile_bool_t _profile, read_mode_t _read_mode)
        : read(std::forward<T>(_read)), profile(_profile), read_mode(_read_mode) { }

    // We use snapshotting for queries that acquire-and-hold large portions of the
    // table, so that they don't block writes.
    bool use_snapshot() const THROWS_NOTHING;

    // At the moment changefeed reads must be routed to the primary replica.
    bool route_to_primary() const THROWS_NOTHING;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(read_t);

struct point_write_response_t {
    point_write_result_t result;

    point_write_response_t() { }
    explicit point_write_response_t(point_write_result_t _result)
        : result(_result)
    { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(point_write_response_t);

struct point_delete_response_t {
    point_delete_result_t result;
    point_delete_response_t() {}
    explicit point_delete_response_t(point_delete_result_t _result)
        : result(_result)
    { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(point_delete_response_t);

struct sync_response_t {
    // sync always succeeds
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sync_response_t);

struct dummy_write_response_t {
    // dummy write always succeeds
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(dummy_write_response_t);

typedef ql::datum_t batched_replace_response_t;

struct write_response_t {
    boost::variant<batched_replace_response_t,
                   // batched_replace_response_t is also for batched_insert
                   point_write_response_t,
                   point_delete_response_t,
                   sync_response_t,
                   dummy_write_response_t> response;

    profile::event_log_t event_log;
    size_t n_shards;

    write_response_t() { }
    template<class T>
    explicit write_response_t(const T &t) : response(t) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(write_response_t);

struct batched_replace_t {
    batched_replace_t() { }
    batched_replace_t(
            std::vector<store_key_t> &&_keys,
            const std::string &_pkey,
            const counted_t<const ql::func_t> &func,
            ql::global_optargs_t _optargs,
            auth::user_context_t user_context,
            return_changes_t _return_changes)
        : keys(std::move(_keys)),
          pkey(_pkey),
          f(func),
          optargs(std::move(_optargs)),
          m_user_context(std::move(user_context)),
          return_changes(_return_changes) {
        r_sanity_check(keys.size() != 0);
    }
    std::vector<store_key_t> keys;
    std::string pkey;
    ql::wire_func_t f;
    ql::global_optargs_t optargs;
    auth::user_context_t m_user_context;
    return_changes_t return_changes;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(batched_replace_t);

struct batched_insert_t {
    batched_insert_t() { }
    batched_insert_t(
        std::vector<ql::datum_t> &&_inserts,
        const std::string &_pkey,
        conflict_behavior_t _conflict_behavior,
        boost::optional<counted_t<const ql::func_t> > _conflict_func,
        const ql::configured_limits_t &_limits,
        auth::user_context_t user_context,
        return_changes_t _return_changes);

    std::vector<ql::datum_t> inserts;
    std::string pkey;
    conflict_behavior_t conflict_behavior;
    boost::optional<ql::wire_func_t> conflict_func;
    ql::configured_limits_t limits;
    auth::user_context_t m_user_context;
    return_changes_t return_changes;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(batched_insert_t);

class point_write_t {
public:
    point_write_t() { }
    point_write_t(const store_key_t& _key,
                  ql::datum_t _data,
                  bool _overwrite = true)
        : key(_key), data(_data), overwrite(_overwrite) { }

    store_key_t key;
    ql::datum_t data;
    bool overwrite;
};
RDB_DECLARE_SERIALIZABLE(point_write_t);

class point_delete_t {
public:
    point_delete_t() { }
    explicit point_delete_t(const store_key_t& _key)
        : key(_key) { }

    store_key_t key;
};
RDB_DECLARE_SERIALIZABLE(point_delete_t);

class sync_t {
public:
    sync_t()
        : region(region_t::universe())
    { }

    region_t region;
};
RDB_DECLARE_SERIALIZABLE(sync_t);

// `dummy_write_t` can be used to poll for table readiness - it will go through all
// the clustering layers, but is a no-op in the protocol layer.
class dummy_write_t {
public:
    dummy_write_t() : region(region_t::universe()) { }
    region_t region;
};
RDB_DECLARE_SERIALIZABLE(dummy_write_t);

struct write_t {
    typedef boost::variant<batched_replace_t,
                           batched_insert_t,
                           point_write_t,
                           point_delete_t,
                           sync_t,
                           dummy_write_t> variant_t;
    variant_t write;

    durability_requirement_t durability_requirement;
    profile_bool_t profile;
    ql::configured_limits_t limits;

    region_t get_region() const THROWS_NOTHING;
    // Returns true if the write had any side effects applicable to the
    // region, and a non-empty write was written to write_out.
    bool shard(const region_t &region,
               write_t *write_out) const THROWS_NOTHING;
    void unshard(write_response_t *responses, size_t count,
                 write_response_t *response, rdb_context_t *cache, signal_t *)
        const THROWS_NOTHING;

    // This is currently used to improve the cache's write transaction throttling.
    int expected_document_changes() const;

    durability_requirement_t durability() const { return durability_requirement; }

    /* The clustering layer calls this. */
    static write_t make_sync(const region_t &region, profile_bool_t profile) {
        sync_t sync;
        sync.region = region;
        return write_t(
            sync,
            DURABILITY_REQUIREMENT_HARD,
            profile,
            ql::configured_limits_t());
    }

    write_t() :
        durability_requirement(DURABILITY_REQUIREMENT_DEFAULT),
        profile(profile_bool_t::DONT_PROFILE),
        limits() {}
    /*  Note that for durability != DURABILITY_REQUIREMENT_HARD, sync might
     *  not have the desired effect (of writing unsaved data to disk).
     *  However there are cases where we use sync internally (such as when
     *  splitting up batched replaces/inserts) and want it to only have an
     *  effect if DURABILITY_REQUIREMENT_DEFAULT resolves to hard
     *  durability. */
    template<class T>
    write_t(T &&t,
            durability_requirement_t _durability,
            profile_bool_t _profile,
            const ql::configured_limits_t &_limits)
        : write(std::forward<T>(t)),
          durability_requirement(_durability), profile(_profile),
          limits(_limits) { }
    template<class T>
    write_t(T &&t, profile_bool_t _profile,
            const ql::configured_limits_t &_limits)
        : write(std::forward<T>(t)),
          durability_requirement(DURABILITY_REQUIREMENT_DEFAULT),
          profile(_profile),
          limits(_limits) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(write_t);

class store_t;

namespace rdb_protocol {
const size_t MAX_PRIMARY_KEY_SIZE = 128;

// Construct a region containing only the specified key
region_t monokey_region(const store_key_t &k);

// Constructs a region which will query an sindex for matches to a specific key
// TODO consider relocating this
key_range_t sindex_key_range(const store_key_t &start,
                             const store_key_t &end,
                             key_range_t::bound_t end_type);
}  // namespace rdb_protocol


namespace rdb_protocol {
/* TODO: This might be redundant. I thought that `key_tester_t` was only
originally necessary because in v1.1.x the hashing scheme might be different
between the source and destination servers. */
struct range_key_tester_t : public key_tester_t {
    explicit range_key_tester_t(const region_t *_delete_range) : delete_range(_delete_range) { }
    virtual ~range_key_tester_t() { }
    bool key_should_be_erased(const btree_key_t *key);

    const region_t *delete_range;
};
} // namespace rdb_protocol

#endif  // RDB_PROTOCOL_PROTOCOL_HPP_
