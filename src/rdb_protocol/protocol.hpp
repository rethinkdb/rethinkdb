// Copyright 2010-2014 RethinkDB, all rights reserved.
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
#include "rdb_protocol/shards.hpp"
#include "region/region.hpp"
#include "repli_timestamp.hpp"
#include "rpc/mailbox/typed.hpp"

class store_t;
class buf_lock_t;
template <class> class clone_ptr_t;
template <class> class cow_ptr_t;
template <class> class cross_thread_watchable_variable_t;
class cross_thread_signal_t;
struct secondary_index_t;
class traversal_progress_combiner_t;
template <class> class watchable_t;
class Term;
class Datum;
class Backtrace;


namespace unittest { struct make_sindex_read_t; }

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

enum class sindex_rename_result_t {
    OLD_NAME_DOESNT_EXIST,
    NEW_NAME_EXISTS,
    SUCCESS
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        sindex_rename_result_t, int8_t,
        sindex_rename_result_t::OLD_NAME_DOESNT_EXIST, sindex_rename_result_t::SUCCESS);

#define RDB_DECLARE_PROTOB_SERIALIZABLE(pb_t) \
    void serialize_protobuf(write_message_t *wm, const pb_t &p); \
    MUST_USE archive_result_t deserialize_protobuf(read_stream_t *s, pb_t *p)

RDB_DECLARE_PROTOB_SERIALIZABLE(Term);
RDB_DECLARE_PROTOB_SERIALIZABLE(Datum);
RDB_DECLARE_PROTOB_SERIALIZABLE(Backtrace);

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
class env_t;
class primary_readgen_t;
class readgen_t;
class sindex_readgen_t;
class intersecting_readgen_t;
} // namespace ql

struct backfill_atom_t {
    store_key_t key;
    ql::datum_t value;
    repli_timestamp_t recency;

    backfill_atom_t() { }
    backfill_atom_t(const store_key_t &_key,
                    const ql::datum_t &_value,
                    const repli_timestamp_t &_recency) :
        key(_key),
        value(_value),
        recency(_recency)
    { }
};
RDB_DECLARE_SERIALIZABLE(backfill_atom_t);

enum class sindex_multi_bool_t { SINGLE = 0, MULTI = 1};
enum class sindex_geo_bool_t { REGULAR = 0, GEO = 1};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(sindex_multi_bool_t, int8_t,
        sindex_multi_bool_t::SINGLE, sindex_multi_bool_t::MULTI);
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(sindex_geo_bool_t, int8_t,
        sindex_geo_bool_t::REGULAR, sindex_geo_bool_t::GEO);

namespace rdb_protocol {

void bring_sindexes_up_to_date(
        const std::set<sindex_name_t> &sindexes_to_bring_up_to_date,
        store_t *store,
        buf_lock_t *sindex_block)
    THROWS_NOTHING;

struct single_sindex_status_t {
    single_sindex_status_t()
        : blocks_processed(0),
          blocks_total(0), ready(true), outdated(false),
          geo(sindex_geo_bool_t::REGULAR), multi(sindex_multi_bool_t::SINGLE)
    { }
    single_sindex_status_t(size_t _blocks_processed, size_t _blocks_total, bool _ready)
        : blocks_processed(_blocks_processed),
          blocks_total(_blocks_total), ready(_ready) { }
    size_t blocks_processed, blocks_total;
    bool ready;
    bool outdated;
    sindex_geo_bool_t geo;
    sindex_multi_bool_t multi;
    std::string func;
};

} // namespace rdb_protocol

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(rdb_protocol::single_sindex_status_t);

struct point_read_response_t {
    ql::datum_t data;
    point_read_response_t() { }
    explicit point_read_response_t(ql::datum_t _data)
        : data(_data) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(point_read_response_t);

struct rget_read_response_t {
    ql::result_t result;
    ql::skey_version_t skey_version;
    bool truncated;
    store_key_t last_key;

    rget_read_response_t()
        : skey_version(ql::skey_version_t::pre_1_16), truncated(false) { }
    explicit rget_read_response_t(const ql::exc_t &ex)
        : result(ex), skey_version(ql::skey_version_t::pre_1_16), truncated(false) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(rget_read_response_t);

struct intersecting_geo_read_response_t {
    boost::variant<ql::datum_t, ql::exc_t> results_or_error;

    intersecting_geo_read_response_t() { }
    intersecting_geo_read_response_t(
            const ql::datum_t &_results)
        : results_or_error(_results) { }
    intersecting_geo_read_response_t(
            const ql::exc_t &_error)
        : results_or_error(_error) { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(intersecting_geo_read_response_t);

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

struct sindex_list_response_t {
    sindex_list_response_t() { }
    std::vector<std::string> sindexes;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_list_response_t);

struct sindex_status_response_t {
    sindex_status_response_t()
    { }
    std::map<std::string, rdb_protocol::single_sindex_status_t> statuses;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_status_response_t);

struct changefeed_subscribe_response_t {
    changefeed_subscribe_response_t() { }
    std::set<uuid_u> server_uuids;
    std::set<ql::changefeed::server_t::addr_t> addrs;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_subscribe_response_t);

struct changefeed_stamp_response_t {
    changefeed_stamp_response_t() { }
    // The `uuid_u` below is the uuid of the changefeed `server_t`.  (We have
    // different timestamps for each `server_t` because they're on different
    // servers and don't synchronize with each other.)
    std::map<uuid_u, uint64_t> stamps;
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_stamp_response_t);

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
    std::pair<uuid_u, uint64_t> stamp;
    ql::datum_t initial_val;
};

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
                           sindex_list_response_t,
                           sindex_status_response_t,
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
                       boost::optional<region_t> &&_region,
                       const ql::datum_range_t _original_range)
        : id(_id), region(std::move(_region)), original_range(_original_range) { }
    std::string id; // What sindex we're using.
    // What keyspace we're currently operating on.  If empty, assume the
    // original range and create the readgen on the shards.
    boost::optional<region_t> region;
    ql::datum_range_t original_range; // For dealing with truncation.
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_rangespec_t);

class rget_read_t {
public:
    rget_read_t() : batchspec(ql::batchspec_t::empty()) { }

    rget_read_t(const region_t &_region,
                const std::map<std::string, ql::wire_func_t> &_optargs,
                const std::string &_table_name,
                const ql::batchspec_t &_batchspec,
                const std::vector<ql::transform_variant_t> &_transforms,
                boost::optional<ql::terminal_variant_t> &&_terminal,
                boost::optional<sindex_rangespec_t> &&_sindex,
                sorting_t _sorting)
        : region(_region),
          optargs(_optargs),
          table_name(_table_name),
          batchspec(_batchspec),
          transforms(_transforms),
          terminal(std::move(_terminal)),
          sindex(std::move(_sindex)),
          sorting(_sorting) { }

    region_t region; // We need this even for sindex reads due to sharding.
    std::map<std::string, ql::wire_func_t> optargs;
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
            const region_t &_region,
            const std::map<std::string, ql::wire_func_t> &_optargs,
            const std::string &_table_name,
            const ql::batchspec_t &_batchspec,
            const std::vector<ql::transform_variant_t> &_transforms,
            boost::optional<ql::terminal_variant_t> &&_terminal,
            sindex_rangespec_t &&_sindex,
            const ql::datum_t &_query_geometry)
        : region(_region),
          optargs(_optargs),
          table_name(_table_name),
          batchspec(_batchspec),
          transforms(_transforms),
          terminal(std::move(_terminal)),
          sindex(std::move(_sindex)),
          query_geometry(_query_geometry) { }

    region_t region; // Primary key range. We need this because of sharding.
    std::map<std::string, ql::wire_func_t> optargs;
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
            lon_lat_point_t _center, double _max_dist, uint64_t _max_results,
            const ellipsoid_spec_t &_geo_system, const std::string &_table_name,
            const std::string &_sindex_id,
            const std::map<std::string, ql::wire_func_t> &_optargs)
        : optargs(_optargs), center(_center), max_dist(_max_dist),
          max_results(_max_results), geo_system(_geo_system),
          region(_region), table_name(_table_name),
          sindex_id(_sindex_id) { }

    std::map<std::string, ql::wire_func_t> optargs;

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

struct sindex_list_t {
    sindex_list_t() { }
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_list_t);

struct sindex_status_t {
    sindex_status_t() { }
    explicit sindex_status_t(const std::set<std::string> &_sindexes)
        : sindexes(_sindexes), region(region_t::universe())
    { }
    std::set<std::string> sindexes;
    region_t region;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_status_t);

struct changefeed_subscribe_t {
    changefeed_subscribe_t() { }
    explicit changefeed_subscribe_t(ql::changefeed::client_t::addr_t _addr)
        : addr(_addr), region(region_t::universe()) { }
    ql::changefeed::client_t::addr_t addr;
    region_t region;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_subscribe_t);

struct changefeed_limit_subscribe_t {
    changefeed_limit_subscribe_t() { }
    explicit changefeed_limit_subscribe_t(
        ql::changefeed::client_t::addr_t _addr,
        uuid_u _uuid,
        ql::changefeed::keyspec_t::limit_t _spec,
        std::string _table,
        std::map<std::string, ql::wire_func_t> _optargs,
        region_t pkey_region)
        : addr(std::move(_addr)),
          uuid(std::move(_uuid)),
          spec(std::move(_spec)),
          table(std::move(_table)),
          optargs(std::move(_optargs)),
          region(std::move(pkey_region)) { }
    ql::changefeed::client_t::addr_t addr;
    uuid_u uuid;
    ql::changefeed::keyspec_t::limit_t spec;
    std::string table;
    std::map<std::string, ql::wire_func_t> optargs;
    region_t region;
};
RDB_DECLARE_SERIALIZABLE(changefeed_limit_subscribe_t);

struct changefeed_stamp_t {
    changefeed_stamp_t() : region(region_t::universe()) { }
    explicit changefeed_stamp_t(
        ql::changefeed::client_t::addr_t _addr)
        : addr(std::move(_addr)),
          region(region_t::universe()) { }
    ql::changefeed::client_t::addr_t addr;
    region_t region;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(changefeed_stamp_t);

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
                           sindex_list_t,
                           sindex_status_t,
                           dummy_read_t> variant_t;
    variant_t read;
    profile_bool_t profile;

    region_t get_region() const THROWS_NOTHING;
    // Returns true if the read has any operation for this region.  Returns
    // false if read_out has not been touched.
    bool shard(const region_t &region,
               read_t *read_out) const THROWS_NOTHING;

    void unshard(read_response_t *responses, size_t count,
                 read_response_t *response, rdb_context_t *ctx,
                 signal_t *interruptor) const
        THROWS_ONLY(interrupted_exc_t);

    read_t() { }
    template<class T>
    read_t(T &&_read, profile_bool_t _profile)
        : read(std::forward<T>(_read)), profile(_profile) { }

    // We use snapshotting for queries that acquire-and-hold large portions of the
    // table, so that they don't block writes.
    bool use_snapshot() const THROWS_NOTHING;

    // Returns true if this read should be sent to every replica.
    bool all_read() const THROWS_NOTHING { return boost::get<sindex_status_t>(&read); }
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

// TODO we're reusing the enums from row writes and reads to avoid name
// shadowing. Nothing really wrong with this but maybe they could have a
// more generic name.
struct sindex_create_response_t {
    bool success;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_create_response_t);

struct sindex_drop_response_t {
    bool success;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_drop_response_t);

struct sindex_rename_response_t {
    sindex_rename_result_t result;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_rename_response_t);

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
                   sindex_create_response_t,
                   sindex_drop_response_t,
                   sindex_rename_response_t,
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
            const std::map<std::string, ql::wire_func_t > &_optargs,
            return_changes_t _return_changes)
        : keys(std::move(_keys)), pkey(_pkey), f(func), optargs(_optargs),
          return_changes(_return_changes) {
        r_sanity_check(keys.size() != 0);
    }
    std::vector<store_key_t> keys;
    std::string pkey;
    ql::wire_func_t f;
    std::map<std::string, ql::wire_func_t > optargs;
    return_changes_t return_changes;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(batched_replace_t);

struct batched_insert_t {
    batched_insert_t() { }
    batched_insert_t(
            std::vector<ql::datum_t> &&_inserts,
            const std::string &_pkey, conflict_behavior_t _conflict_behavior,
            const ql::configured_limits_t &_limits,
            return_changes_t _return_changes)
        : inserts(std::move(_inserts)), pkey(_pkey),
          conflict_behavior(_conflict_behavior), limits(_limits),
          return_changes(_return_changes) {
        r_sanity_check(inserts.size() != 0);
#ifndef NDEBUG
        // These checks are done above us, but in debug mode we do them
        // again.  (They're slow.)  We do them above us because the code in
        // val.cc knows enough to report the write errors correctly while
        // still doing the other writes.
        for (auto it = inserts.begin(); it != inserts.end(); ++it) {
            ql::datum_t keyval =
                it->get_field(datum_string_t(pkey), ql::NOTHROW);
            r_sanity_check(keyval.has());
            try {
                keyval.print_primary(); // ERROR CHECKING
                continue;
            } catch (const ql::base_exc_t &e) {
            }
            r_sanity_check(false); // throws, so can't do this in exception handler
        }
#endif // NDEBUG
    }
    std::vector<ql::datum_t> inserts;
    std::string pkey;
    conflict_behavior_t conflict_behavior;
    ql::configured_limits_t limits;
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

class sindex_create_t {
public:
    sindex_create_t() { }
    sindex_create_t(const std::string &_id, const ql::map_wire_func_t &_mapping,
                    sindex_multi_bool_t _multi, sindex_geo_bool_t _geo)
        : id(_id), mapping(_mapping), region(region_t::universe()),
          multi(_multi), geo(_geo)
    { }

    std::string id;
    ql::map_wire_func_t mapping;
    region_t region;
    sindex_multi_bool_t multi;
    sindex_geo_bool_t geo;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_create_t);

class sindex_drop_t {
public:
    sindex_drop_t() { }
    explicit sindex_drop_t(const std::string &_id)
        : id(_id), region(region_t::universe())
    { }

    std::string id;
    region_t region;
};
RDB_DECLARE_SERIALIZABLE(sindex_drop_t);

class sindex_rename_t {
public:
    sindex_rename_t() { }
    sindex_rename_t(const std::string &_old_name,
                    const std::string &_new_name,
                    bool _overwrite) :
        old_name(_old_name),
        new_name(_new_name),
        overwrite(_overwrite),
        region(region_t::universe()) { }

    std::string old_name;
    std::string new_name;
    bool overwrite;
    region_t region;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(sindex_rename_t);

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
                           sindex_create_t,
                           sindex_drop_t,
                           sindex_rename_t,
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

    durability_requirement_t durability() const { return durability_requirement; }

    write_t() : durability_requirement(DURABILITY_REQUIREMENT_DEFAULT), limits() {}
    /*  Note that for durability != DURABILITY_REQUIREMENT_HARD, sync might
     *  not have the desired effect (of writing unsaved data to disk).
     *  However there are cases where we use sync internally (such as when
     *  splitting up batched replaces/inserts) and want it to only have an
     *  effect if DURABILITY_REQUIREMENT_DEFAULT resolves to hard
     *  durability. */
    template<class T>
    write_t(T &&t,
            durability_requirement_t durability,
            profile_bool_t _profile,
            const ql::configured_limits_t &_limits)
        : write(std::forward<T>(t)),
          durability_requirement(durability), profile(_profile),
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

struct backfill_chunk_t {
    struct delete_key_t {
        store_key_t key;
        repli_timestamp_t recency;

        delete_key_t() { }
        delete_key_t(const store_key_t& _key, const repli_timestamp_t& _recency) : key(_key), recency(_recency) { }
    };
    struct delete_range_t {
        region_t range;
        delete_range_t() { }
        explicit delete_range_t(const region_t& _range) : range(_range) { }
    };
    struct key_value_pairs_t {
        std::vector<backfill_atom_t> backfill_atoms;

        key_value_pairs_t() { }
        explicit key_value_pairs_t(std::vector<backfill_atom_t> &&_backfill_atoms)
            : backfill_atoms(std::move(_backfill_atoms)) { }
    };
    struct sindexes_t {
        std::map<std::string, secondary_index_t> sindexes;

        sindexes_t() { }
        explicit sindexes_t(const std::map<std::string, secondary_index_t> &_sindexes)
            : sindexes(_sindexes) { }
    };

    typedef boost::variant<delete_range_t, delete_key_t, key_value_pairs_t, sindexes_t> value_t;

    backfill_chunk_t() { }
    explicit backfill_chunk_t(const value_t &_val) : val(_val) { }
    value_t val;

    static backfill_chunk_t delete_range(const region_t& range) {
        return backfill_chunk_t(delete_range_t(range));
    }
    static backfill_chunk_t delete_key(const store_key_t& key, const repli_timestamp_t& recency) {
        return backfill_chunk_t(delete_key_t(key, recency));
    }
    static backfill_chunk_t set_keys(std::vector<backfill_atom_t> &&keys) {
        return backfill_chunk_t(key_value_pairs_t(std::move(keys)));
    }

    static backfill_chunk_t sindexes(const std::map<std::string, secondary_index_t> &sindexes) {
        return backfill_chunk_t(sindexes_t(sindexes));
    }
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_chunk_t::delete_key_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_chunk_t::delete_range_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_chunk_t::key_value_pairs_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_chunk_t::sindexes_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_chunk_t);


class store_t;

namespace rdb_protocol {
const size_t MAX_PRIMARY_KEY_SIZE = 128;

// Construct a region containing only the specified key
region_t monokey_region(const store_key_t &k);

// Constructs a region which will query an sindex for matches to a specific key
// TODO consider relocating this
key_range_t sindex_key_range(const store_key_t &start,
                             const store_key_t &end);

region_t cpu_sharding_subspace(int subregion_number, int num_cpu_shards);
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
