// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_BTREE_HPP_
#define RDB_PROTOCOL_BTREE_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "backfill_progress.hpp"
#include "btree/btree_store.hpp"
#include "rdb_protocol/protocol.hpp"

class key_tester_t;
class parallel_traversal_progress_t;
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

typedef rdb_protocol_t::point_replace_t point_replace_t;
typedef rdb_protocol_t::point_replace_response_t point_replace_response_t;

typedef rdb_protocol_t::batched_replaces_response_t batched_replaces_response_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

class parallel_traversal_progress_t;

static const size_t rget_max_chunk_size = MEGABYTE;

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value);

template <>
class value_sizer_t<rdb_value_t> : public value_sizer_t<void> {
public:
    explicit value_sizer_t<rdb_value_t>(block_size_t bs);

    static const rdb_value_t *as_rdb(const void *p);

    int size(const void *value) const;

    bool fits(const void *value, int length_available) const;

    bool deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const;
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

void rdb_get(const store_key_t &key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock, point_read_response_t *response);

// QL2 This implements UPDATE, REPLACE, and part of DELETE and INSERT (each is
// just a different function passed to this function).
void rdb_replace(btree_slice_t *slice,
                 repli_timestamp_t timestamp,
                 transaction_t *txn,
                 superblock_t *superblock,
                 const std::string &primary_key,
                 const store_key_t &key,
                 ql::map_wire_func_t *f,
                 ql::env_t *ql_env,
                 Datum *response_out,
                 rdb_modification_info_t *mod_info);

void rdb_batched_replace(const std::vector<std::pair<int64_t, point_replace_t> > &replaces, btree_slice_t *slice, repli_timestamp_t timestamp,
                         transaction_t *txn, scoped_ptr_t<superblock_t> *superblock, ql::env_t *ql_env,
                         batched_replaces_response_t *response_out,
                         rdb_modification_report_cb_t *sindex_cb);

void rdb_set(const store_key_t &key, boost::shared_ptr<scoped_cJSON_t> data, bool overwrite,
             btree_slice_t *slice, repli_timestamp_t timestamp,
             transaction_t *txn, superblock_t *superblock, point_write_response_t *response,
             rdb_modification_info_t *mod_info);

class rdb_backfill_callback_t {
public:
    virtual void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_keyvalue(const rdb_protocol_details::backfill_atom_t& atom, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_sindexes(const std::map<std::string, secondary_index_t> &sindexes, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
protected:
    virtual ~rdb_backfill_callback_t() { }
};


void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range,
        repli_timestamp_t since_when, rdb_backfill_callback_t *callback,
        transaction_t *txn, superblock_t *superblock,
        buf_lock_t *sindex_block,
        parallel_traversal_progress_t *p, signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);


void rdb_delete(const store_key_t &key, btree_slice_t *slice, repli_timestamp_t
        timestamp, transaction_t *txn, superblock_t *superblock,
        point_delete_response_t *response,
        rdb_modification_info_t *mod_info);

class rdb_value_deleter_t : public value_deleter_t {
    void delete_value(transaction_t *_txn, void *_value);
};

void rdb_erase_range(btree_slice_t *slice, key_tester_t *tester,
                     const key_range_t &keys,
                     transaction_t *txn, superblock_t *superblock,
                     btree_store_t<rdb_protocol_t> *store, 
                     write_token_pair_t *token_pair,
                     signal_t *interruptor);

/* RGETS */
size_t estimate_rget_response_size(const boost::shared_ptr<scoped_cJSON_t> &json);

struct rget_response_t {
    std::vector<std::pair<store_key_t, boost::shared_ptr<scoped_cJSON_t> > > pairs;
    bool truncated;
};

void rdb_rget_slice(btree_slice_t *slice, const key_range_t &range,
                    transaction_t *txn, superblock_t *superblock,
                    ql::env_t *ql_env,
                    const rdb_protocol_details::transform_t &transform,
                    const boost::optional<rdb_protocol_details::terminal_t> &terminal,
                    rget_read_response_t *response);

void rdb_rget_secondary_slice(btree_slice_t *slice, const key_range_t &range,
                    transaction_t *txn, superblock_t *superblock,
                    ql::env_t *ql_env,
                    const rdb_protocol_details::transform_t &transform,
                    const boost::optional<rdb_protocol_details::terminal_t> &terminal,
                    const key_range_t &pk_range,
                    rget_read_response_t *response);

void rdb_distribution_get(btree_slice_t *slice, int max_depth, const store_key_t &left_key,
                          transaction_t *txn, superblock_t *superblock, distribution_read_response_t *response);

/* Secondary Indexes */

struct rdb_modification_info_t {
    boost::shared_ptr<scoped_cJSON_t> deleted;
    boost::shared_ptr<scoped_cJSON_t> added;

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


struct rdb_erase_range_report_t {
    rdb_erase_range_report_t() { }
    explicit rdb_erase_range_report_t(const key_range_t &_range_to_erase)
        : range_to_erase(_range_to_erase) { }
    key_range_t range_to_erase;

    RDB_DECLARE_ME_SERIALIZABLE;
};

typedef boost::variant<rdb_modification_report_t, 
                       rdb_erase_range_report_t>
        rdb_sindex_change_t;

/* An rdb_modification_cb_t is passed to BTree operations and allows them to
 * modify the secondary while they perform an operation. */
class rdb_modification_report_cb_t {
public:
    rdb_modification_report_cb_t(
            btree_store_t<rdb_protocol_t> *store, write_token_pair_t *token_pair,
            transaction_t *txn, block_id_t sindex_block, auto_drainer_t::lock_t lock);
    void add_row(const store_key_t &primary_key, boost::shared_ptr<scoped_cJSON_t> added);
    void delete_row(const store_key_t &primary_key, boost::shared_ptr<scoped_cJSON_t> deleted);
    void replace_row(const store_key_t &primary_key,
            boost::shared_ptr<scoped_cJSON_t> added,
            boost::shared_ptr<scoped_cJSON_t> removed);

    void on_mod_report(const rdb_modification_report_t &mod_report);

    ~rdb_modification_report_cb_t();
private:

    /* Fields initialized by the constructor. */
    btree_store_t<rdb_protocol_t> *store_;
    write_token_pair_t *token_pair_;
    transaction_t *txn_;
    block_id_t sindex_block_id_;
    auto_drainer_t::lock_t lock_;

    /* Fields initialized by calls to on_mod_report */
    scoped_ptr_t<buf_lock_t> sindex_block_;
    btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindexes_;
};

void rdb_update_sindexes(
        const btree_store_t<rdb_protocol_t>::sindex_access_vector_t &sindexes,
        const rdb_modification_report_t *modification,
        transaction_t *txn);

void rdb_erase_range_sindexes(
        const btree_store_t<rdb_protocol_t>::sindex_access_vector_t &sindexes,
        const rdb_erase_range_report_t *erase_range,
        transaction_t *txn,
        signal_t *interruptor);

void post_construct_secondary_indexes(
        btree_store_t<rdb_protocol_t> *store,
        const std::set<uuid_u> &sindexes_to_post_construct,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

#endif /* RDB_PROTOCOL_BTREE_HPP_ */
