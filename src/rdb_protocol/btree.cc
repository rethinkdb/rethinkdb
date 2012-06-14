#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "containers/archive/vector_stream.hpp"
#include "rdb_protocol/btree.hpp"

bool btree_value_fits(block_size_t bs, int data_length, const rdb_value_t *value) {
    return blob::ref_fits(bs, data_length, value->value_ref(), blob::btree_maxreflen);
}

point_read_response_t db_get(const store_key_t &store_key, btree_slice_t *slice, transaction_t *txn, superblock_t *superblock) {
    keyvalue_location_t<rdb_value_t> kv_location;
    find_keyvalue_location_for_read(txn, superblock, store_key.btree_key(), &kv_location, slice->root_eviction_priority, &slice->stats);

    if (!kv_location.value) {
        return point_read_response_t();
    }

    const rdb_value_t *value = kv_location.value.get();
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
