// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_BTREE_HPP_
#define RDB_PROTOCOL_BTREE_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "backfill_progress.hpp"
#include "btree/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/store.hpp"

class btree_slice_t;
class deletion_context_t;
class key_tester_t;
class parallel_traversal_progress_t;
template <class> class promise_t;
struct rdb_value_t;
class refcount_superblock_t;
struct sindex_disk_info_t;

class parallel_traversal_progress_t;

bool btree_value_fits(max_block_size_t bs, int data_length, const rdb_value_t *value);

class rdb_value_sizer_t : public value_sizer_t {
public:
    explicit rdb_value_sizer_t(max_block_size_t bs);

    static const rdb_value_t *as_rdb(const void *p);

    int size(const void *value) const;

    bool fits(const void *value, int length_available) const;

    int max_possible_size() const;

    static block_magic_t leaf_magic();

    block_magic_t btree_leaf_magic() const;

    max_block_size_t block_size() const;

private:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    max_block_size_t block_size_;

    DISABLE_COPYING(rdb_value_sizer_t);
};

struct rdb_modification_info_t;
struct rdb_modification_report_t;
class rdb_modification_report_cb_t;

void rdb_get(
    const store_key_t &key,
    btree_slice_t *slice,
    superblock_t *superblock,
    point_read_response_t *response,
    profile::trace_t *trace);

struct btree_info_t {
    btree_info_t(btree_slice_t *_slice,
                 repli_timestamp_t _timestamp,
                 const datum_string_t &_primary_key)
        : slice(_slice), timestamp(_timestamp),
          primary_key(_primary_key) {
        guarantee(slice != NULL);
    }
    btree_slice_t *const slice;
    const repli_timestamp_t timestamp;
    const datum_string_t primary_key;
};

struct btree_loc_info_t {
    btree_loc_info_t(const btree_info_t *_btree,
                     real_superblock_t *_superblock,
                     const store_key_t *_key)
        : btree(_btree), superblock(_superblock), key(_key) {
        guarantee(btree != NULL);
        guarantee(superblock != NULL);
        guarantee(key != NULL);
    }
    const btree_info_t *const btree;
    real_superblock_t *const superblock;
    const store_key_t *const key;
};

struct btree_batched_replacer_t {
    virtual ~btree_batched_replacer_t() { }
    virtual ql::datum_t replace(
        const ql::datum_t &d, size_t index) const = 0;
    virtual return_changes_t should_return_changes() const = 0;
};
struct btree_point_replacer_t {
    virtual ~btree_point_replacer_t() { }
    virtual ql::datum_t replace(
        const ql::datum_t &d) const = 0;
    virtual return_changes_t should_return_changes() const = 0;
};

batched_replace_response_t rdb_batched_replace(
    const btree_info_t &info,
    scoped_ptr_t<real_superblock_t> *superblock,
    const std::vector<store_key_t> &keys,
    const btree_batched_replacer_t *replacer,
    rdb_modification_report_cb_t *sindex_cb,
    ql::configured_limits_t limits,
    profile::sampler_t *sampler,
    profile::trace_t *trace);

void rdb_set(const store_key_t &key, ql::datum_t data,
             bool overwrite,
             btree_slice_t *slice, repli_timestamp_t timestamp,
             superblock_t *superblock,
             const deletion_context_t *deletion_context,
             point_write_response_t *response,
             rdb_modification_info_t *mod_info,
             profile::trace_t *trace,
             promise_t<superblock_t *> *pass_back_superblock = NULL);

class rdb_backfill_callback_t {
public:
    virtual void on_delete_range(
        const key_range_t &range,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_deletion(
        const btree_key_t *key,
        repli_timestamp_t recency,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_keyvalues(
        std::vector<backfill_atom_t> &&atoms,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_sindexes(
        const std::map<std::string, secondary_index_t> &sindexes,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
protected:
    virtual ~rdb_backfill_callback_t() { }
};


void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range,
                  repli_timestamp_t since_when, rdb_backfill_callback_t *callback,
                  refcount_superblock_t *superblock,
                  buf_lock_t *sindex_block,
                  parallel_traversal_progress_t *p, signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);


void rdb_delete(const store_key_t &key, btree_slice_t *slice, repli_timestamp_t
                timestamp, real_superblock_t *superblock,
                const deletion_context_t *deletion_context,
                point_delete_response_t *response,
                rdb_modification_info_t *mod_info,
                profile::trace_t *trace);

void rdb_rget_slice(
    btree_slice_t *slice,
    const key_range_t &range,
    superblock_t *superblock,
    ql::env_t *ql_env,
    const ql::batchspec_t &batchspec,
    const std::vector<ql::transform_variant_t> &transforms,
    const boost::optional<ql::terminal_variant_t> &terminal,
    sorting_t sorting,
    rget_read_response_t *response,
    release_superblock_t release_superblock);

void rdb_rget_secondary_slice(
    btree_slice_t *slice,
    const ql::datum_range_t &datum_range,
    const region_t &sindex_region,
    sindex_superblock_t *superblock,
    ql::env_t *ql_env,
    const ql::batchspec_t &batchspec,
    const std::vector<ql::transform_variant_t> &transforms,
    const boost::optional<ql::terminal_variant_t> &terminal,
    const key_range_t &pk_range,
    sorting_t sorting,
    const sindex_disk_info_t &sindex_info,
    rget_read_response_t *response,
    release_superblock_t release_superblock);

void rdb_get_intersecting_slice(
    btree_slice_t *slice,
    const ql::datum_t &query_geometry,
    const region_t &sindex_region,
    sindex_superblock_t *superblock,
    ql::env_t *ql_env,
    const ql::batchspec_t &batchspec,
    const std::vector<ql::transform_variant_t> &transforms,
    const boost::optional<ql::terminal_variant_t> &terminal,
    const key_range_t &pk_range,
    const sindex_disk_info_t &sindex_info,
    rget_read_response_t *response);

void rdb_get_nearest_slice(
    btree_slice_t *slice,
    const lon_lat_point_t &center,
    double max_dist,
    uint64_t max_results,
    const ellipsoid_spec_t &geo_system,
    sindex_superblock_t *superblock,
    ql::env_t *ql_env,
    const key_range_t &pk_range,
    const sindex_disk_info_t &sindex_info,
    nearest_geo_read_response_t *response);

void rdb_distribution_get(int max_depth,
                          const store_key_t &left_key,
                          real_superblock_t *superblock,
                          distribution_read_response_t *response);

/* Secondary Indexes */

struct rdb_modification_info_t {
    typedef std::pair<ql::datum_t, std::vector<char> > data_pair_t;
    data_pair_t deleted;
    data_pair_t added;
};

RDB_DECLARE_SERIALIZABLE(rdb_modification_info_t);

struct rdb_modification_report_t {
    rdb_modification_report_t() { }
    explicit rdb_modification_report_t(const store_key_t &_primary_key)
        : primary_key(_primary_key) { }

    store_key_t primary_key;
    rdb_modification_info_t info;
};

RDB_DECLARE_SERIALIZABLE(rdb_modification_report_t);

// The query evaluation reql version information that we store with each secondary
// index function.
struct sindex_reql_version_info_t {
    // Generally speaking, original_reql_version <= latest_compatible_reql_version <=
    // latest_checked_reql_version.  When a new sindex is created, the values are the
    // same.  When a new version of RethinkDB gets run, latest_checked_reql_version
    // will get updated, and latest_compatible_reql_version will get updated if the
    // sindex function is compatible with a later version than the original value of
    // `latest_checked_reql_version`.

    // The original ReQL version of the sindex function.  The value here never
    // changes.  This might become useful for tracking down some bugs or fixing them
    // in-place, or performing a desperate reverse migration.
    reql_version_t original_reql_version;

    // This is the latest version for which evaluation of the sindex function remains
    // compatible.
    reql_version_t latest_compatible_reql_version;

    // If this is less than the current server version, we'll re-check
    // opaque_definition for compatibility and update this value and
    // `latest_compatible_reql_version` accordingly.
    reql_version_t latest_checked_reql_version;

    // To be used for new secondary indexes.
    static sindex_reql_version_info_t LATEST() {
        sindex_reql_version_info_t ret = { reql_version_t::LATEST,
                                           reql_version_t::LATEST,
                                           reql_version_t::LATEST };
        return ret;
    }
};

struct sindex_disk_info_t {
    sindex_disk_info_t() { }
    sindex_disk_info_t(const ql::map_wire_func_t &_mapping,
                       const sindex_reql_version_info_t &_mapping_version_info,
                       sindex_multi_bool_t _multi,
                       sindex_geo_bool_t _geo) :
        mapping(_mapping), mapping_version_info(_mapping_version_info),
        multi(_multi), geo(_geo) { }
    ql::map_wire_func_t mapping;
    sindex_reql_version_info_t mapping_version_info;
    sindex_multi_bool_t multi;
    sindex_geo_bool_t geo;
};

void serialize_sindex_info(write_message_t *wm,
                           const sindex_disk_info_t &info);
// Note that this will throw an exception if there's an error rather than just
// crashing.
void deserialize_sindex_info(const std::vector<char> &data,
                             sindex_disk_info_t *info_out)
    THROWS_ONLY(archive_exc_t);

/* An rdb_modification_cb_t is passed to BTree operations and allows them to
 * modify the secondary while they perform an operation. */
class superblock_queue_t;
class rdb_modification_report_cb_t {
public:
    rdb_modification_report_cb_t(
            store_t *store,
            buf_lock_t *sindex_block,
            auto_drainer_t::lock_t lock);
    ~rdb_modification_report_cb_t();

    scoped_ptr_t<new_mutex_in_line_t> get_in_line_for_sindex();
    rwlock_in_line_t get_in_line_for_stamp();

    void on_mod_report(const rdb_modification_report_t &mod_report,
                       bool update_pkey_cfeeds,
                       new_mutex_in_line_t *sindex_spot,
                       rwlock_in_line_t *stamp_spot);
    bool has_pkey_cfeeds();
    void finish(btree_slice_t *btree, real_superblock_t *superblock);

private:
    void on_mod_report_sub(
        const rdb_modification_report_t &mod_report,
        new_mutex_in_line_t *spot,
        cond_t *keys_available_cond,
        cond_t *done_cond,
        std::map<std::string,
                 std::vector<std::pair<ql::datum_t, boost::optional<uint64_t> > > >
            *old_keys_out,
        std::map<std::string,
                 std::vector<std::pair<ql::datum_t, boost::optional<uint64_t> > > >
            *new_keys_out);

    /* Fields initialized by the constructor. */
    auto_drainer_t::lock_t lock_;
    store_t *store_;
    buf_lock_t *sindex_block_;

    /* Fields initialized by calls to on_mod_report */
    store_t::sindex_access_vector_t sindexes_;
};

void rdb_update_sindexes(
    store_t *store,
    const store_t::sindex_access_vector_t &sindexes,
    const rdb_modification_report_t *modification,
    txn_t *txn,
    const deletion_context_t *deletion_context,
    cond_t *keys_available_cond,
    std::map<std::string,
             std::vector<std::pair<ql::datum_t, boost::optional<uint64_t> > > >
        *old_keys_out,
    std::map<std::string,
             std::vector<std::pair<ql::datum_t, boost::optional<uint64_t> > > >
        *new_keys_out);

void post_construct_secondary_indexes(
        store_t *store,
        const std::set<uuid_u> &sindexes_to_post_construct,
        signal_t *interruptor,
        parallel_traversal_progress_t *progress_tracker)
    THROWS_ONLY(interrupted_exc_t);

/* This deleter actually deletes the value and all associated blocks. */
class rdb_value_deleter_t : public value_deleter_t {
public:
    void delete_value(buf_parent_t parent, const void *_value) const;
};

/* A deleter that doesn't actually delete the values. Needed for secondary
 * indexes which only have references. */
class rdb_value_detacher_t : public value_deleter_t {
public:
    void delete_value(buf_parent_t parent, const void *value) const;
};

/* Used for operations on the live storage.
 * Each value is first detached in all trees, and then actually deleted through
 * the post_deleter. */
class rdb_live_deletion_context_t : public deletion_context_t {
public:
    rdb_live_deletion_context_t() { }
    const value_deleter_t *balancing_detacher() const { return &detacher; }
    const value_deleter_t *in_tree_deleter() const { return &detacher; }
    const value_deleter_t *post_deleter() const { return &deleter; }
private:
    rdb_value_detacher_t detacher;
    rdb_value_deleter_t deleter;
};

/* A deleter that does absolutely nothing. */
class noop_value_deleter_t : public value_deleter_t {
public:
    noop_value_deleter_t() { }
    void delete_value(buf_parent_t, const void *) const;
};

/* Used for operations on secondary indexes that aren't yet post-constructed.
 * Since we don't have any guarantees that referenced blob blocks still exist
 * during that stage, we use noop deleters for everything. */
class rdb_noop_deletion_context_t : public deletion_context_t {
public:
    rdb_noop_deletion_context_t() { }
    const value_deleter_t *balancing_detacher() const { return &no_deleter; }
    const value_deleter_t *in_tree_deleter() const { return &no_deleter; }
    const value_deleter_t *post_deleter() const { return &no_deleter; }
private:
    noop_value_deleter_t no_deleter;
};
typedef rdb_noop_deletion_context_t rdb_post_construction_deletion_context_t;


#endif /* RDB_PROTOCOL_BTREE_HPP_ */
