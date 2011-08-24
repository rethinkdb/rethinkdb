#include "btree/buf_patches.hpp"

#include "btree/leaf_node.hpp"
#include "riak/riak_value.hpp"
#include "btree/detemplatizer.hpp"




leaf_insert_patch_t::leaf_insert_patch_t(block_id_t block_id, patch_counter_t patch_counter, uint16_t _value_size, const void *value, uint8_t key_size, const char *key_contents, repli_timestamp_t _insertion_time) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
            value_size(_value_size),
            insertion_time(_insertion_time) {
    value_buf = new char[value_size];
    memcpy(value_buf, value, value_size);

    key_buf = new char[sizeof(btree_key_t) + key_size];
    btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_insert_patch_t::leaf_insert_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char* data, uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
            value_size(0) {
    guarantee_patch_format(data_length >= sizeof(value_size) + sizeof(insertion_time));
    value_size = *(reinterpret_cast<const uint16_t *>(data));
    data += sizeof(value_size);
    insertion_time = *(reinterpret_cast<const repli_timestamp_t *>(data));
    data += sizeof(insertion_time);

    guarantee_patch_format(sizeof(value_size) + sizeof(insertion_time) + value_size + 1 <= data_length);
    value_buf = new char[value_size];
    memcpy(value_buf, data, value_size);
    data += value_size;

    uint8_t key_size = *(reinterpret_cast<const uint8_t *>(data));
    data += sizeof(key_size);
    key_buf = new char[sizeof(btree_key_t) + key_size];
    try {
        btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
        key->size = key_size;
        guarantee_patch_format(data_length == sizeof(value_size) + sizeof(insertion_time) + value_size + sizeof(uint8_t) + key->size);
        memcpy(key->contents, data, key->size);
        data += key->size;
    } catch (patch_deserialization_error_t& e) {
        delete[] key_buf;
        delete[] value_buf;
        throw e;
    }
}

void leaf_insert_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &value_size, sizeof(value_size));
    destination += sizeof(value_size);
    memcpy(destination, &insertion_time, sizeof(insertion_time));
    destination += sizeof(insertion_time);

    const void *value = reinterpret_cast<void *>(value_buf);
    const btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);

    memcpy(destination, value, value_size);
    destination += value_size;

    memcpy(destination, &key->size, sizeof(key->size));
    destination += sizeof(key->size);
    memcpy(destination, key->contents, key->size);
    destination += key->size;
}

uint16_t leaf_insert_patch_t::get_data_size() const {
    const btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);

    return sizeof(value_size) + sizeof(insertion_time) + value_size + sizeof(uint8_t) + key->size;
}

leaf_insert_patch_t::~leaf_insert_patch_t() {
    delete[] value_buf;
    delete[] key_buf;
}

size_t leaf_insert_patch_t::get_affected_data_size() const {
    const btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
    return value_size + sizeof(uint8_t) + key->size + sizeof(insertion_time);
}

void leaf_insert_patch_t::apply_to_buf(char *buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::insert, leaf_node, bs, leaf_node, reinterpret_cast<btree_key_t *>(key_buf), value_buf, insertion_time);
}


leaf_remove_patch_t::leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, repli_timestamp_t tstamp, const uint8_t key_size, const char *key_contents) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            timestamp(tstamp) {
    key_buf = new char[sizeof(btree_key_t) + key_size];
    btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_remove_patch_t::leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            timestamp(repli_timestamp_t::invalid) {
    guarantee_patch_format(data_length >= sizeof(repli_timestamp_t) + sizeof(uint8_t));

    timestamp = *reinterpret_cast<const repli_timestamp_t *>(data);
    data += sizeof(timestamp);

    uint8_t key_size = *(reinterpret_cast<const uint8_t *>(data));
    data += sizeof(key_size);
    key_buf = new char[sizeof(btree_key_t) + key_size];
    try {
        btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
        key->size = key_size;
        guarantee_patch_format(data_length >= sizeof(repli_timestamp_t) + sizeof(uint8_t) + key->size);
        memcpy(key->contents, data, key->size);
        data += key->size;
    } catch (patch_deserialization_error_t &e) {
        delete[] key_buf;
        throw e;
    }
}

void leaf_remove_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &timestamp, sizeof(timestamp));
    destination += sizeof(timestamp);

    const btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);

    memcpy(destination, &key->size, sizeof(key->size));
    destination += sizeof(key->size);
    memcpy(destination, key->contents, key->size);
    destination += key->size;
}

uint16_t leaf_remove_patch_t::get_data_size() const {
    const btree_key_t *key = reinterpret_cast<const btree_key_t *>(key_buf);

    return sizeof(timestamp) + sizeof(uint8_t) + key->size;
}

leaf_remove_patch_t::~leaf_remove_patch_t() {
    delete[] key_buf;
}

size_t leaf_remove_patch_t::get_affected_data_size() const {
    const btree_key_t *key = reinterpret_cast<const btree_key_t *>(key_buf);
    return key->size + sizeof(key->size);
}

void leaf_remove_patch_t::apply_to_buf(char* buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::remove, leaf_node, bs, reinterpret_cast<leaf_node_t *>(buf_data), reinterpret_cast<btree_key_t *>(key_buf), timestamp);
}



leaf_erase_presence_patch_t::leaf_erase_presence_patch_t(block_id_t block_id, patch_counter_t patch_counter, uint8_t key_size, const char *key_contents)
    : buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_ERASE_PRESENCE),
      key_buf(NULL) {
    rassert(key_size <= 250);
    key_buf = new char[1 + key_size];
    *reinterpret_cast<uint8_t *>(key_buf) = key_size;
    memcpy(key_buf + 1, key_contents, key_size);
}

leaf_erase_presence_patch_t::leaf_erase_presence_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char *data, uint16_t data_length)
    : buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_ERASE_PRESENCE),
      key_buf(NULL) {
    const btree_key_t *data_as_key = reinterpret_cast<const btree_key_t *>(data);
    guarantee_patch_format(data_as_key->fits(data_length));
    key_buf = new char[1 + data_as_key->size];
    keycpy(reinterpret_cast<btree_key_t *>(key_buf), data_as_key);
}

leaf_erase_presence_patch_t::~leaf_erase_presence_patch_t() {
    delete[] key_buf;
}

void leaf_erase_presence_patch_t::apply_to_buf(char *buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::erase_presence, leaf_node, bs, leaf_node, reinterpret_cast<btree_key_t *>(key_buf));
}

// TODO (sam): We have get_data_size and get_affected_data_size, idk
// what get_affected_data_size really means.
size_t leaf_erase_presence_patch_t::get_affected_data_size() const {
    return reinterpret_cast<const btree_key_t *>(key_buf)->size + 1;
}

void leaf_erase_presence_patch_t::serialize_data(char *destination) const {
    keycpy(reinterpret_cast<btree_key_t *>(destination), reinterpret_cast<const btree_key_t *>(key_buf));
}

uint16_t leaf_erase_presence_patch_t::get_data_size() const {
    return reinterpret_cast<const btree_key_t *>(key_buf)->size + 1;
}

