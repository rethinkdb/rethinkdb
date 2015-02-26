// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef BTREE_REQL_SPECIFIC_HPP_
#define BTREE_REQL_SPECIFIC_HPP_

#include "btree/operations.hpp"

class binary_blob_t;

/* real_superblock_t implements superblock_t in terms of an actual on-disk block
   structure. */
class real_superblock_t : public superblock_t {
public:
    explicit real_superblock_t(buf_lock_t &&sb_buf);

    void release();
    buf_lock_t *get() { return &sb_buf_; }

    block_id_t get_root_block_id();
    void set_root_block_id(block_id_t new_root_block);

    block_id_t get_stat_block_id();
    void set_stat_block_id(block_id_t new_stat_block);

    block_id_t get_sindex_block_id();
    void set_sindex_block_id(block_id_t new_block_id);

    buf_parent_t expose_buf() { return buf_parent_t(&sb_buf_); }

private:
    buf_lock_t sb_buf_;
};

enum class index_type_t {
    PRIMARY,
    SECONDARY
};

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t : public home_thread_mixin_debug_only_t {
public:
    // Initializes a superblock (presumably, a buf_lock_t constructed with
    // alt_create_t::create) for use with btrees, setting the initial value of the
    // metainfo (with a single key/value pair).
    static void init_superblock(buf_lock_t *superblock,
                                const std::vector<char> &metainfo_key,
                                const binary_blob_t &metainfo_value);

    btree_slice_t(cache_t *cache,
                  perfmon_collection_t *parent,
                  const std::string &identifier,
                  index_type_t index_type);

    ~btree_slice_t();

    cache_t *cache() { return cache_; }
    cache_account_t *get_backfill_account() { return &backfill_account_; }

    btree_stats_t stats;

private:
    cache_t *cache_;

    // Cache account to be used when backfilling.
    cache_account_t backfill_account_;

    DISABLE_COPYING(btree_slice_t);
};

/* This iterator encapsulates most of the metainfo data layout. Unfortunately,
 * functions set_superblock_metainfo and delete_superblock_metainfo also know a
 * lot about the data layout, so if it's changed, these functions must be
 * changed as well.
 *
 * Data layout is dead simple right now, it's an array of the following
 * (unaligned, unpadded) contents:
 *
 *   sz_t key_size;
 *   char key[key_size];
 *   sz_t value_size;
 *   char value[value_size];
 */
struct superblock_metainfo_iterator_t {
    typedef uint32_t sz_t;  // be careful: the values of this type get casted to int64_t in checks, so it must fit
    typedef std::pair<sz_t, char *> key_t;
    typedef std::pair<sz_t, char *> value_t;

    superblock_metainfo_iterator_t(char *metainfo, char *metainfo_end) : end(metainfo_end) { advance(metainfo); }

    bool is_end() { return pos == end; }

    void operator++();

    std::pair<key_t, value_t> operator*() {
        return std::make_pair(key(), value());
    }
    key_t key() { return std::make_pair(key_size, key_ptr); }
    value_t value() { return std::make_pair(value_size, value_ptr); }

    char *record_ptr() { return pos; }
    char *next_record_ptr() { return next_pos; }
    char *end_ptr() { return end; }
    sz_t *value_size_ptr() { return reinterpret_cast<sz_t*>(value_ptr) - 1; }
private:
    void advance(char *p);

    char *pos;
    char *next_pos;
    char *end;
    sz_t key_size;
    char *key_ptr;
    sz_t value_size;
    char *value_ptr;
};


// Metainfo functions
bool get_superblock_metainfo(buf_lock_t *superblock,
                             const std::vector<char> &key,
                             std::vector<char> *value_out);

void get_superblock_metainfo(
    buf_lock_t *superblock,
    std::vector< std::pair<std::vector<char>, std::vector<char> > > *kv_pairs_out);

void set_superblock_metainfo(buf_lock_t *superblock,
                             const std::vector<char> &key,
                             const binary_blob_t &value);

void set_superblock_metainfo(buf_lock_t *superblock,
                             const std::vector<std::vector<char> > &keys,
                             const std::vector<binary_blob_t> &values);

void delete_superblock_metainfo(buf_lock_t *superblock,
                                const std::vector<char> &key);
void clear_superblock_metainfo(buf_lock_t *superblock);

void get_btree_superblock(txn_t *txn, access_t access,
                          scoped_ptr_t<real_superblock_t> *got_superblock_out);

void get_btree_superblock_and_txn(cache_conn_t *cache_conn,
                                  write_access_t superblock_access,
                                  int expected_change_count,
                                  repli_timestamp_t tstamp,
                                  write_durability_t durability,
                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                  scoped_ptr_t<txn_t> *txn_out);

void get_btree_superblock_and_txn_for_backfilling(cache_conn_t *cache_conn,
                                                  cache_account_t *backfill_account,
                                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                                  scoped_ptr_t<txn_t> *txn_out);

void get_btree_superblock_and_txn_for_reading(cache_conn_t *cache_conn,
                                              cache_snapshotted_t snapshotted,
                                              scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                              scoped_ptr_t<txn_t> *txn_out);

#endif /* BTREE_REQL_SPECIFIC_HPP_ */

