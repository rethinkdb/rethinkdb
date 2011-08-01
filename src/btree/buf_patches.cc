#include "btree/buf_patches.hpp"

#include "btree/leaf_node.hpp"
#include "riak/riak_value.hpp"
#include "btree/detemplatizer.hpp"




leaf_shift_pairs_patch_t::leaf_shift_pairs_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t offset, const uint16_t shift) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_SHIFT_PAIRS),
            offset(offset),
            shift(shift) {
}

leaf_shift_pairs_patch_t::leaf_shift_pairs_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_SHIFT_PAIRS) {
    guarantee_patch_format(data_length == get_data_size());
    offset = *(reinterpret_cast<const uint16_t *>(data));
    data += sizeof(offset);
    shift = *(reinterpret_cast<const uint16_t *>(data));
    data += sizeof(shift);
}

void leaf_shift_pairs_patch_t::apply_to_buf(char *buf_data, UNUSED block_size_t bs) {
    leaf::impl::shift_pairs(reinterpret_cast<leaf_node_t *>(buf_data), offset, shift);
}

void leaf_shift_pairs_patch_t::serialize_data(char *destination) const {
    memcpy(destination, &offset, sizeof(offset));
    destination += sizeof(offset);
    memcpy(destination, &shift, sizeof(shift));
    destination += sizeof(shift);
}

uint16_t leaf_shift_pairs_patch_t::get_data_size() const {
    return sizeof(offset) + sizeof(shift);
}


leaf_insert_pair_patch_t::leaf_insert_pair_patch_t(block_id_t block_id, patch_counter_t patch_counter, uint16_t value_size, const opaque_value_t *value, uint8_t key_size, const char *key_contents)
    : buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT_PAIR), value_size(value_size) {
    value_buf = new char[value_size];
    memcpy(value_buf, value, value_size);

    key_buf = new char[sizeof(btree_key_t) + key_size];
    btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_insert_pair_patch_t::leaf_insert_pair_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char* data, uint16_t data_length)
    : buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT_PAIR), value_size(0) {
    guarantee_patch_format(data_length >= sizeof(value_size));
    value_size = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(value_size);

    guarantee_patch_format(data_length >= sizeof(value_size) + value_size + 1);
    value_buf = new char[value_size];
    memcpy(value_buf, data, value_size);
    data += value_size;

    uint8_t key_size = *(reinterpret_cast<const uint8_t *>(data));
    data += sizeof(key_size);

    try {
        guarantee_patch_format(data_length == sizeof(value_size) + value_size + 1 + key_size);

        key_buf = new char[sizeof(btree_key_t) + key_size];
        try {
            btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
            key->size = key_size;
            memcpy(key->contents, data, key->size);
            data += key->size;
        } catch (patch_deserialization_error_t &e) {
            delete[] key_buf;
            throw e;
        }
    } catch (patch_deserialization_error_t &e) {
        delete[] value_buf;
        throw e;
    }
}

void leaf_insert_pair_patch_t::serialize_data(char* destination) const {
    const opaque_value_t *value = reinterpret_cast<const opaque_value_t *>(value_buf);
    const btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);

    memcpy(destination, &value_size, sizeof(value_size));
    destination += sizeof(value_size);

    memcpy(destination, value, value_size);
    destination += value_size;

    memcpy(destination, &key->size, sizeof(key->size));
    destination += sizeof(key->size);
    memcpy(destination, key->contents, key->size);
    destination += key->size;
}

uint16_t leaf_insert_pair_patch_t::get_data_size() const {
    const btree_key_t *key = reinterpret_cast<const btree_key_t *>(key_buf);

    return sizeof(value_size) + value_size + sizeof(uint8_t) + key->size;
}

leaf_insert_pair_patch_t::~leaf_insert_pair_patch_t() {
    delete[] value_buf;
    delete[] key_buf;
}

size_t leaf_insert_pair_patch_t::get_affected_data_size() const {
    const btree_key_t *key = reinterpret_cast<const btree_key_t *>(key_buf);
    return value_size + sizeof(uint8_t) + key->size;
}

void leaf_insert_pair_patch_t::apply_to_buf(char* buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::impl::insert_pair, leaf_node, bs, reinterpret_cast<leaf_node_t *>(buf_data), value_buf, reinterpret_cast<btree_key_t *>(key_buf));
}

leaf_insert_patch_t::leaf_insert_patch_t(block_id_t block_id, patch_counter_t patch_counter, uint16_t value_size, const opaque_value_t *value, uint8_t key_size, const char *key_contents, repli_timestamp_t insertion_time) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
            value_size(value_size),
            insertion_time(insertion_time) {
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

    const opaque_value_t *value = reinterpret_cast<opaque_value_t *>(value_buf);
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

void leaf_insert_patch_t::apply_to_buf(char* buf_data, block_size_t bs) {
    leaf_node_t *leaf_node = reinterpret_cast<leaf_node_t *>(buf_data);
    DETEMPLATIZE_LEAF_NODE_OP(leaf::insert, leaf_node, bs, reinterpret_cast<leaf_node_t *>(buf_data), reinterpret_cast<btree_key_t *>(key_buf), value_buf, insertion_time);
}


leaf_remove_patch_t::leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const block_size_t block_size, const uint8_t key_size, const char *key_contents) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            block_size(block_size) {
    key_buf = new char[sizeof(btree_key_t) + key_size];
    btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_remove_patch_t::leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            block_size(block_size_t::unsafe_make(0)) {
    guarantee_patch_format(data_length >= sizeof(block_size) + sizeof(uint8_t));
    block_size = *(reinterpret_cast<const block_size_t *>(data));
    data += sizeof(block_size);

    uint8_t key_size = *(reinterpret_cast<const uint8_t *>(data));
    data += sizeof(key_size);
    key_buf = new char[sizeof(btree_key_t) + key_size];
    try {
        btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);
        key->size = key_size;
        guarantee_patch_format(data_length >= sizeof(block_size) + sizeof(uint8_t) + key->size);
        memcpy(key->contents, data, key->size);
        data += key->size;
    } catch (patch_deserialization_error_t &e) {
        delete[] key_buf;
        throw e;
    }
}

void leaf_remove_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &block_size, sizeof(block_size));
    destination += sizeof(block_size);

    const btree_key_t *key = reinterpret_cast<btree_key_t *>(key_buf);

    memcpy(destination, &key->size, sizeof(key->size));
    destination += sizeof(key->size);
    memcpy(destination, key->contents, key->size);
    destination += key->size;
}

uint16_t leaf_remove_patch_t::get_data_size() const {
    const btree_key_t *key = reinterpret_cast<const btree_key_t *>(key_buf);

    return sizeof(block_size) + sizeof(uint8_t) + key->size;
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
    DETEMPLATIZE_LEAF_NODE_OP(leaf::remove, leaf_node, bs, reinterpret_cast<leaf_node_t *>(buf_data), reinterpret_cast<btree_key_t *>(key_buf));
 }

