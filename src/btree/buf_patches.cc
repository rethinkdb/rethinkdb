#include "btree/buf_patches.hpp"

#include "btree/leaf_node.hpp"
#include "memcached/memcached_btree/node.hpp"
#include "memcached/memcached_btree/value.hpp"
#include "btree/detemplatizer.hpp"

leaf_insert_patch_t::leaf_insert_patch_t(block_id_t block_id, repli_timestamp_t block_timestamp, patch_counter_t patch_counter, uint16_t _value_size, const void *value, const btree_key_t *_key, repli_timestamp_t _insertion_time)
    : buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
      value_size(_value_size), key(_key),
      insertion_time(std::max(block_timestamp, _insertion_time)) {

    {
        scoped_malloc_t<char> tmp(value_size);
        value_buf.swap(tmp);
        memcpy(value_buf.get(), value, value_size);
    }
}

leaf_insert_patch_t::leaf_insert_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char* data, uint16_t data_length)
    : buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
      value_size(0) {

    guarantee_patch_format(data_length >= sizeof(value_size) + sizeof(insertion_time));
    value_size = *(reinterpret_cast<const uint16_t *>(data));
    data += sizeof(value_size);
    insertion_time = *(reinterpret_cast<const repli_timestamp_t *>(data));
    data += sizeof(insertion_time);

    guarantee_patch_format(sizeof(value_size) + sizeof(insertion_time) + value_size + 1 <= data_length);

    {
        scoped_malloc_t<char> tmp(data, data + value_size);
        value_buf.swap(tmp);
        data += value_size;
    }

    key.set_size(*(reinterpret_cast<const uint8_t *>(data)));
    data += sizeof(uint8_t);
    guarantee_patch_format(data_length == sizeof(value_size) + sizeof(insertion_time) + value_size + sizeof(uint8_t) + key.size());
    memcpy(key.contents(), data, key.size());

    // Uncomment this if you have more to read.
    // data += key->size;
}

void leaf_insert_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &value_size, sizeof(value_size));
    destination += sizeof(value_size);
    memcpy(destination, &insertion_time, sizeof(insertion_time));
    destination += sizeof(insertion_time);

    const void *value = reinterpret_cast<const void *>(value_buf.get());
    const btree_key_t *k = key.btree_key();

    memcpy(destination, value, value_size);
    destination += value_size;

    memcpy(destination, &k->size, sizeof(k->size));
    destination += sizeof(k->size);
    memcpy(destination, k->contents, k->size);

    // Uncomment this if you have more data to write.
    // destination += key->size;
}

uint16_t leaf_insert_patch_t::get_data_size() const {
    return sizeof(value_size) + sizeof(insertion_time) + value_size + sizeof(uint8_t) + key.size();
}

void leaf_insert_patch_t::apply_to_buf(char *buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::insert, leaf_node, bs, leaf_node, key.btree_key(), value_buf.get(), insertion_time, key_modification_proof_t::real_proof());
}


leaf_remove_patch_t::leaf_remove_patch_t(block_id_t block_id, repli_timestamp_t block_timestamp, patch_counter_t patch_counter, repli_timestamp_t tstamp, const btree_key_t *_key) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            timestamp(std::max(block_timestamp, tstamp)), key(_key) { }

leaf_remove_patch_t::leaf_remove_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char* data, uint16_t data_length) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            timestamp(repli_timestamp_t::invalid) {
    guarantee_patch_format(data_length >= sizeof(repli_timestamp_t) + sizeof(uint8_t));

    timestamp = *reinterpret_cast<const repli_timestamp_t *>(data);
    data += sizeof(timestamp);

    key.set_size(*(reinterpret_cast<const uint8_t *>(data)));
    data += sizeof(uint8_t);
    guarantee_patch_format(data_length >= sizeof(repli_timestamp_t) + sizeof(uint8_t) + key.size());
    memcpy(key.contents(), data, key.size());

    // Uncomment this if you have more to read.
    // data += key->size;
}

void leaf_remove_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &timestamp, sizeof(timestamp));
    destination += sizeof(timestamp);

    const btree_key_t *k = key.btree_key();

    memcpy(destination, &k->size, sizeof(k->size));
    destination += sizeof(k->size);
    memcpy(destination, k->contents, k->size);

    // Uncomment this if you have more to write.
    // destination += key->size;
}

uint16_t leaf_remove_patch_t::get_data_size() const {
    return sizeof(timestamp) + sizeof(uint8_t) + key.size();
}

void leaf_remove_patch_t::apply_to_buf(char* buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::remove, leaf_node, bs, reinterpret_cast<leaf_node_t *>(buf_data), key.btree_key(), timestamp, key_modification_proof_t::real_proof());
}



leaf_erase_presence_patch_t::leaf_erase_presence_patch_t(block_id_t block_id, patch_counter_t patch_counter, const btree_key_t *_key)
    : buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_ERASE_PRESENCE), key(_key) { }

leaf_erase_presence_patch_t::leaf_erase_presence_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char *data, uint16_t data_length)
    : buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_ERASE_PRESENCE) {
    const btree_key_t *data_as_key = reinterpret_cast<const btree_key_t *>(data);
    guarantee_patch_format(data_as_key->fits(data_length));
    key.assign(data_as_key);
}

void leaf_erase_presence_patch_t::apply_to_buf(char *buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::erase_presence, leaf_node, bs, leaf_node, key.btree_key(), key_modification_proof_t::real_proof());
}

void leaf_erase_presence_patch_t::serialize_data(char *destination) const {
    keycpy(reinterpret_cast<btree_key_t *>(destination), key.btree_key());
}

uint16_t leaf_erase_presence_patch_t::get_data_size() const {
    return key.size() + 1;
}

