#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "btree/backfill.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rdb_protocol/btree.hpp"

boost::shared_ptr<scoped_cJSON_t> get_data(const rdb_value_t *value, transaction_t *txn) {
    blob_t blob(const_cast<rdb_value_t *>(value)->value_ref(), blob::btree_maxreflen);

    boost::shared_ptr<scoped_cJSON_t> data;

    /* Grab the data from the blob. */
    //TODO unnecessary copies, I hate them
    std::string serialized_data;
    blob.read_to_string(serialized_data, txn, 0, blob.valuesize());

    /* Deserialize the value and return it. */
    std::vector<char> data_vec(serialized_data.begin(), serialized_data.end());

    vector_read_stream_t read_stream(&data_vec);

    int res = deserialize(&read_stream, &data);
    guarantee_err(res == 0, "corruption detected... this should probably be an exception\n");

    return data;
}

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value) {
    return blob::ref_fits(bs, data_length, value->value_ref(), blob::btree_maxreflen);
}

point_read_response_t db_get(const store_key_t &store_key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_read(txn, superblock, store_key.btree_key(), &kv_location, slice->root_eviction_priority, &slice->stats);

    if (!kv_location.value) {
        return point_read_response_t();
    }

    boost::shared_ptr<scoped_cJSON_t> data = get_data(kv_location.value.get(), txn);

    return point_read_response_t(data);
}

point_write_response_t rdb_set(const store_key_t &key, boost::shared_ptr<scoped_cJSON_t> data, 
                       btree_slice_t *slice, repli_timestamp_t timestamp,
                       transaction_t *txn, superblock_t *superblock) {
    block_size_t block_size = slice->cache()->get_block_size();

    keyvalue_location_t<rdb_value_t> kv_location;

    bool already_existed = bool(kv_location.value);

    find_keyvalue_location_for_write(txn, superblock, key.btree_key(), &kv_location, &slice->root_eviction_priority, &slice->stats);
    scoped_malloc<rdb_value_t> new_value(MAX_RDB_VALUE_SIZE);
    bzero(new_value.get(), MAX_RDB_VALUE_SIZE);

    //TODO unnecessary copies they must go away.
    write_message_t wm;
    wm << data;
    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee_err(res == 0, "Serialization for json data failed... this shouldn't happen.\n");

    blob_t blob(new_value->value_ref(), blob::btree_maxreflen);

    //TODO more copies, good lord
    blob.append_region(txn, stream.vector().size());
    std::string sered_data(stream.vector().begin(), stream.vector().end());
    blob.write_from_string(sered_data, txn, 0);

    // Actually update the leaf, if needed.
    kv_location.value.reinterpret_swap(new_value);
    null_key_modification_callback_t<rdb_value_t> null_cb;
    apply_keyvalue_change(txn, &kv_location, key.btree_key(), timestamp, false, &null_cb, &slice->root_eviction_priority);
    //                                                                     ^-- That means the key isn't expired.

    return point_write_response_t(already_existed ? DUPLICATE : STORED);
}

class agnostic_rdb_backfill_callback_t : public agnostic_backfill_callback_t {
public:
    agnostic_rdb_backfill_callback_t(backfill_callback_t *cb, const key_range_t &kr) : cb_(cb), kr_(kr) { }

    void on_delete_range(const key_range_t &range) {
        rassert(kr_.is_superset(range));
        cb_->on_delete_range(range);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency) {
        rassert(kr_.contains_key(key->contents, key->size));
        cb_->on_deletion(key, recency);
    }

    void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *val) {
        rassert(kr_.contains_key(key->contents, key->size));
        const rdb_value_t *value = static_cast<const rdb_value_t *>(val);

        backfill_atom_t atom;
        atom.key.assign(key->size, key->contents);
        atom.value = get_data(value, txn);
        atom.recency = recency;
        cb_->on_keyvalue(atom);
    }

    backfill_callback_t *cb_;
    key_range_t kr_;
};

void rdb_backfill(btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when, backfill_callback_t *callback,
                    transaction_t *txn, superblock_t *superblock, traversal_progress_t *p) {
    agnostic_rdb_backfill_callback_t agnostic_cb(callback, key_range);
    value_sizer_t<rdb_value_t> sizer(slice->cache()->get_block_size());
    do_agnostic_btree_backfill(&sizer, slice, key_range, since_when, &agnostic_cb, txn, superblock, p);
}
