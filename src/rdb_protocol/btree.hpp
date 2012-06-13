#ifndef RDB_PROTOCOL_BTREE_HPP__
#define RDB_PROTOCOL_BTREE_HPP__

#include "rdb_protocol/protocol.hpp"

rdb_protocol_t::point_read_response_t rdb_get(const store_key_t &key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock);

rdb_protocol_t::point_write_response_t rdb_set(const store_key_t &key, boost::shared_ptr<scoped_cJSON_t> data, 
                       btree_slice_t *slice, repli_timestamp_t timestamp,
                       transaction_t *txn, superblock_t *superblock);


class backfill_callback_t {
public:
    virtual void on_delete_range(const key_range_t &range) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency) = 0;
    virtual void on_keyvalue(const backfill_atom_t& atom) = 0;
protected:
    virtual ~backfill_callback_t() { }
};


void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when, backfill_callback_t *callback,
                    transaction_t *txn, superblock_t *superblock, traversal_progress_t *p);


void rdb_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, repli_timestamp_t timestamp, transaction_t *txn, superblock_t *superblock);

void rdb_erase_range(btree_slice_t *slice, key_tester_t *tester,
                                 const key_range_t &keys,
                                 transaction_t *txn, superblock_t *superblock);

#endif
