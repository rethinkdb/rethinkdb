// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_BTREE_HPP_
#define RDB_PROTOCOL_BTREE_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "backfill_progress.hpp"
#include "btree/btree_store.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/protocol.hpp"

class key_tester_t;
class parallel_traversal_progress_t;
template <class> class promise_t;
struct rdb_value_t;

typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;

typedef rdb_protocol_t::rget_read_t rget_read_t;
typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

typedef rdb_protocol_t::distribution_read_t distribution_read_t;
typedef rdb_protocol_t::distribution_read_response_t distribution_read_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::batched_replace_t batched_replace_t;
typedef rdb_protocol_t::batched_insert_t batched_insert_t;
typedef rdb_protocol_t::batched_replace_response_t batched_replace_response_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

class parallel_traversal_progress_t;

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value);

template <>
class value_sizer_t<rdb_value_t> : public value_sizer_t<void> {
public:
    explicit value_sizer_t<rdb_value_t>(block_size_t bs);

    static const rdb_value_t *as_rdb(const void *p);

    int size(const void *value) const;

    bool fits(const void *value, int length_available) const;

    int max_possible_size() const;

    static block_magic_t leaf_magic();

    block_magic_t btree_leaf_magic() const;

    block_size_t block_size() const;

private:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;

    DISABLE_COPYING(value_sizer_t<rdb_value_t>);
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

enum return_vals_t {
    NO_RETURN_VALS = 0,
    RETURN_VALS = 1
};

struct btree_info_t {
    btree_info_t(btree_slice_t *_slice,
                 repli_timestamp_t _timestamp,
                 const std::string *_primary_key)
        : slice(_slice), timestamp(_timestamp),
          primary_key(_primary_key) {
        guarantee(slice != NULL);
        guarantee(primary_key != NULL);
    }
    btree_slice_t *const slice;
    const repli_timestamp_t timestamp;
    const std::string *primary_key;
};

struct btree_loc_info_t {
    btree_loc_info_t(const btree_info_t *_btree,
                     superblock_t *_superblock,
                     const store_key_t *_key)
        : btree(_btree), superblock(_superblock), key(_key) {
        guarantee(btree != NULL);
        guarantee(superblock != NULL);
        guarantee(key != NULL);
    }
    const btree_info_t *const btree;
    superblock_t *const superblock;
    const store_key_t *const key;
};

struct btree_batched_replacer_t {
    virtual ~btree_batched_replacer_t() { }
    virtual counted_t<const ql::datum_t> replace(
        const counted_t<const ql::datum_t> &d, size_t index) const = 0;
    virtual bool should_return_vals() const = 0;
};
struct btree_point_replacer_t {
    virtual ~btree_point_replacer_t() { }
    virtual counted_t<const ql::datum_t> replace(
        const counted_t<const ql::datum_t> &d) const = 0;
    virtual bool should_return_vals() const = 0;
};

batched_replace_response_t rdb_batched_replace(
    const btree_info_t &info,
    scoped_ptr_t<superblock_t> *superblock,
    const std::vector<store_key_t> &keys,
    const btree_batched_replacer_t *replacer,
    rdb_modification_report_cb_t *sindex_cb,
    profile::trace_t *trace);

void rdb_set(const store_key_t &key, counted_t<const ql::datum_t> data,
             bool overwrite,
             btree_slice_t *slice, repli_timestamp_t timestamp,
             superblock_t *superblock,
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
        std::vector<rdb_protocol_details::backfill_atom_t> &&atoms,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_sindexes(
        const std::map<std::string, secondary_index_t> &sindexes,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
protected:
    virtual ~rdb_backfill_callback_t() { }
};


void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range,
                  repli_timestamp_t since_when, rdb_backfill_callback_t *callback,
                  superblock_t *superblock,
                  buf_lock_t *sindex_block,
                  parallel_traversal_progress_t *p, signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);


void rdb_delete(const store_key_t &key, btree_slice_t *slice, repli_timestamp_t
                timestamp, superblock_t *superblock,
                point_delete_response_t *response,
                rdb_modification_info_t *mod_info,
                profile::trace_t *trace);

/* A deleter that doesn't actually delete the values. Needed for secondary
 * indexes which only have references. */
class rdb_value_detacher_t : public value_deleter_t {
    void delete_value(buf_parent_t parent, void *value);
};

/* `rdb_erase_major_range` has a complexity of O(n) where n is the size of the
 * btree, if secondary indexes are present. Be careful when to use it. */
void rdb_erase_major_range(key_tester_t *tester,
                           const key_range_t &keys,
                           buf_lock_t *sindex_block,
                           superblock_t *superblock,
                           btree_store_t<rdb_protocol_t> *store,
                           signal_t *interruptor);

/* `rdb_erase_small_range` has a complexity of O(log n * m) where n is the size of
 * the btree, and m is the number of documents actually being deleted.
 * It also requires O(m) memory. 
 * In contrast to `rdb_erase_major_range()`, it doesn't update secondary indexes
 * itself, but returns a number of modification reports that should be applied
 * to secondary indexes separately. Furthermore, it detaches blobs rather than
 * deleting them. */
void rdb_erase_small_range(key_tester_t *tester,
                           const key_range_t &keys,
                           superblock_t *superblock,
                           signal_t *interruptor,
                           std::vector<rdb_modification_report_t> *mod_reports_out);

/* RGETS */
size_t estimate_rget_response_size(const counted_t<const ql::datum_t> &datum);

void rdb_rget_slice(
    btree_slice_t *slice,
    const key_range_t &range,
    superblock_t *superblock,
    ql::env_t *ql_env,
    const ql::batchspec_t &batchspec,
    const std::vector<rdb_protocol_details::transform_variant_t> &transforms,
    const boost::optional<rdb_protocol_details::terminal_variant_t> &terminal,
    sorting_t sorting,
    rget_read_response_t *response);

void rdb_rget_secondary_slice(
    btree_slice_t *slice,
    const datum_range_t &datum_range,
    const rdb_protocol_t::region_t &sindex_region,
    superblock_t *superblock,
    ql::env_t *ql_env,
    const ql::batchspec_t &batchspec,
    const std::vector<rdb_protocol_details::transform_variant_t> &transforms,
    const boost::optional<rdb_protocol_details::terminal_variant_t> &terminal,
    const key_range_t &pk_range,
    sorting_t sorting,
    const ql::map_wire_func_t &sindex_func,
    sindex_multi_bool_t sindex_multi,
    rget_read_response_t *response);

void rdb_distribution_get(int max_depth,
                          const store_key_t &left_key,
                          superblock_t *superblock,
                          distribution_read_response_t *response);

/* Secondary Indexes */

struct rdb_modification_info_t {
    typedef std::pair<counted_t<const ql::datum_t>,
                      std::vector<char> > data_pair_t;
    data_pair_t deleted;
    data_pair_t added;

    RDB_DECLARE_ME_SERIALIZABLE;
};

struct rdb_modification_report_t {
    rdb_modification_report_t() { }
    explicit rdb_modification_report_t(const store_key_t &_primary_key)
        : primary_key(_primary_key) { }

    store_key_t primary_key;
    rdb_modification_info_t info;

    RDB_DECLARE_ME_SERIALIZABLE;
};


struct rdb_erase_major_range_report_t {
    rdb_erase_major_range_report_t() { }
    explicit rdb_erase_major_range_report_t(const key_range_t &_range_to_erase)
        : range_to_erase(_range_to_erase) { }
    key_range_t range_to_erase;

    RDB_DECLARE_ME_SERIALIZABLE;
};

typedef boost::variant<rdb_modification_report_t,
                       rdb_erase_major_range_report_t>
        rdb_sindex_change_t;

/* An rdb_modification_cb_t is passed to BTree operations and allows them to
 * modify the secondary while they perform an operation. */
class rdb_modification_report_cb_t {
public:
    rdb_modification_report_cb_t(
            btree_store_t<rdb_protocol_t> *store,
            buf_lock_t *sindex_block,
            auto_drainer_t::lock_t lock);

    void on_mod_report(const rdb_modification_report_t &mod_report);

    ~rdb_modification_report_cb_t();

private:
    /* Fields initialized by the constructor. */
    auto_drainer_t::lock_t lock_;
    btree_store_t<rdb_protocol_t> *store_;
    buf_lock_t *sindex_block_;

    /* Fields initialized by calls to on_mod_report */
    btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindexes_;
};

void rdb_update_sindexes(
        const btree_store_t<rdb_protocol_t>::sindex_access_vector_t &sindexes,
        const rdb_modification_report_t *modification,
        txn_t *txn);


void rdb_erase_major_range_sindexes(
        const btree_store_t<rdb_protocol_t>::sindex_access_vector_t &sindexes,
        const rdb_erase_major_range_report_t *erase_range,
        signal_t *interruptor);

void post_construct_secondary_indexes(
        btree_store_t<rdb_protocol_t> *store,
        const std::set<uuid_u> &sindexes_to_post_construct,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

class rdb_value_deleter_t : public value_deleter_t {
    friend void rdb_update_sindexes(
        const btree_store_t<rdb_protocol_t>::sindex_access_vector_t &sindexes,
        const rdb_modification_report_t *modification, txn_t *txn);

    void delete_value(buf_parent_t parent, void *_value);
};


#endif /* RDB_PROTOCOL_BTREE_HPP_ */
