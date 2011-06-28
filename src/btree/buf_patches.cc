#include "btree/buf_patches.hpp"

#include "btree/leaf_node.hpp"

leaf_shift_pairs_patch_t::leaf_shift_pairs_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t offset, const uint16_t shift) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_SHIFT_PAIRS),
            offset(offset),
            shift(shift) {
}

leaf_shift_pairs_patch_t::leaf_shift_pairs_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_SHIFT_PAIRS) {
    guarantee_patch_format(data_length == get_data_size());
    offset = *((uint16_t*)(data));
    data += sizeof(offset);
    shift = *((uint16_t*)(data));
    data += sizeof(shift);
}

void leaf_shift_pairs_patch_t::apply_to_buf(char* buf_data) {
    leaf::impl::shift_pairs(ptr_cast<leaf_node_t>(buf_data), offset, shift);
}

void leaf_shift_pairs_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &offset, sizeof(offset));
    destination += sizeof(offset);
    memcpy(destination, &shift, sizeof(shift));
    destination += sizeof(shift);
}

uint16_t leaf_shift_pairs_patch_t::get_data_size() const {
    return sizeof(offset) + sizeof(shift);
}


leaf_insert_pair_patch_t::leaf_insert_pair_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, block_size_t bs, const btree_value_t *value, const uint8_t key_size, const char *key_contents) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT_PAIR) {
    metadata_flags_t metadata_flags;
    metadata_flags.flags = value_metadata_flags;
    value_buf = new char[value->inline_size(bs)];
    valuecpy(bs, value_buf, value);

    key_buf = new char[sizeof(btree_key_t) + key_size];
    btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_insert_pair_patch_t::leaf_insert_pair_patch_t(block_size_t bs, const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT_PAIR) {
    guarantee_patch_format(data_length >= sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t));
    guarantee_patch_format(btree_value_fits(data_length, data));
    int inline_size = reinterpret_cast<const btree_value_t *>(data)->inline_size(bs);
    value_buf = new char[inline_size];
    valuecpy(bs, value_buf, data);
    data += inline_size;
    uint8_t key_size = *((uint8_t*)(data));
    data += sizeof(key_size);
    key_buf = new char[sizeof(btree_key_t) + key_size];
    try {
        btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
        key->size = key_size;
        guarantee_patch_format(data_length == sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + value->size + metadata_size(value->metadata_flags) + key->size);
        memcpy(key->contents, data, key->size);
        data += key->size;
    } catch (patch_deserialization_error_t &e) {
        delete[] key_buf;
        delete[] value_buf;
        throw e;
    }
}

void leaf_insert_pair_patch_t::serialize_data(char* destination) const {
    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);

    memcpy(destination, &value->size, sizeof(value->size));
    destination += sizeof(value->size);
    memcpy(destination, &value->metadata_flags.flags, sizeof(value->metadata_flags.flags));
    destination += sizeof(value->metadata_flags.flags);
    memcpy(destination, value->contents, value->size + metadata_size(value->metadata_flags));
    destination += value->size + metadata_size(value->metadata_flags);

    memcpy(destination, &key->size, sizeof(key->size));
    destination += sizeof(key->size);
    memcpy(destination, key->contents, key->size);
    destination += key->size;
}

uint16_t leaf_insert_pair_patch_t::get_data_size() const {
    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);

    return sizeof(uint8_t) + sizeof(uint8_t) + value->size + metadata_size(value->metadata_flags) + sizeof(uint8_t) + key->size;
}

leaf_insert_pair_patch_t::~leaf_insert_pair_patch_t() {
    delete[] value_buf;
    delete[] key_buf;
}

size_t leaf_insert_pair_patch_t::get_affected_data_size() const {
    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
    return key->size + value->size + metadata_size(value->metadata_flags) + sizeof(value->metadata_flags.flags) + sizeof(value->size) + sizeof(key->size);
}

void leaf_insert_pair_patch_t::apply_to_buf(char* buf_data) {
    leaf::impl::insert_pair(ptr_cast<leaf_node_t>(buf_data), ptr_cast<btree_value>(value_buf), ptr_cast<btree_key_t>(key_buf));
}

leaf_insert_patch_t::leaf_insert_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const block_size_t block_size, const btree_value_t *value, const uint8_t key_size, const char *key_contents, const repli_timestamp insertion_time) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
            block_size(block_size),
            insertion_time(insertion_time) {
    metadata_flags_t metadata_flags;
    metadata_flags.flags = value_metadata_flags;
    value_buf = new char[value->inline_size(block_size)];
    valuecpy(block_size, value_buf, value);

    key_buf = new char[sizeof(btree_key_t) + key_size];
    btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_insert_patch_t::leaf_insert_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
            block_size(block_size_t::unsafe_make(0)) {
    guarantee_patch_format(data_length >= sizeof(block_size) + sizeof(insertion_time) + sizeof(uint8_t) + sizeof(uint8_t));
    block_size = *((block_size_t*)(data));
    data += sizeof(block_size);
    insertion_time = *((repli_timestamp*)(data));
    data += sizeof(insertion_time);

    guarantee_patch_format(btree_value_fits(data_length - sizeof(block_size) - sizeof(insertion_time), data));
    int inline_size = reinterpret_cast<const btree_value_t *>(data)->inline_size(block_size);
    value_buf = new char[inline_size];
    valuecpy(bs, value_buf, data);
    data += inline_size;

    uint8_t key_size = *((uint8_t*)(data));
    data += sizeof(key_size);
    key_buf = new char[sizeof(btree_key_t) + key_size];
    try {
        btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
        key->size = key_size;
        guarantee_patch_format(data_length == sizeof(block_size) + sizeof(insertion_time) + inline_size + sizeof(uint8_t) + key->size);
        memcpy(key->contents, data, key->size);
        data += key->size;
    } catch (patch_deserialization_error_t& e) {
        delete[] key_buf;
        delete[] value_buf;
        throw e;
    }
}

void leaf_insert_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &block_size, sizeof(block_size));
    destination += sizeof(block_size);
    memcpy(destination, &insertion_time, sizeof(insertion_time));
    destination += sizeof(insertion_time);

    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);

    memcpy(destination, &value->size, sizeof(value->size));
    destination += sizeof(value->size);
    memcpy(destination, &value->metadata_flags.flags, sizeof(value->metadata_flags.flags));
    destination += sizeof(value->metadata_flags.flags);
    memcpy(destination, value->contents, value->size + metadata_size(value->metadata_flags));
    destination += value->size + metadata_size(value->metadata_flags);

    memcpy(destination, &key->size, sizeof(key->size));
    destination += sizeof(key->size);
    memcpy(destination, key->contents, key->size);
    destination += key->size;
}

uint16_t leaf_insert_patch_t::get_data_size() const {
    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);

    return sizeof(block_size) + sizeof(insertion_time) + sizeof(uint8_t) + sizeof(uint8_t) +  value->size + metadata_size(value->metadata_flags) + sizeof(uint8_t) + key->size;
}

leaf_insert_patch_t::~leaf_insert_patch_t() {
    delete[] value_buf;
    delete[] key_buf;
}

size_t leaf_insert_patch_t::get_affected_data_size() const {
    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
    return key->size + value->size + metadata_size(value->metadata_flags) + sizeof(value->metadata_flags.flags) + sizeof(value->size) + sizeof(key->size) + sizeof(insertion_time);
}

void leaf_insert_patch_t::apply_to_buf(char* buf_data) {
    leaf::insert(ptr_cast<leaf_node_t>(buf_data), ptr_cast<btree_key_t>(key_buf), ptr_cast<btree_value>(value_buf), insertion_time);
}


leaf_remove_patch_t::leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const block_size_t block_size, const uint8_t key_size, const char *key_contents) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            block_size(block_size) {
    key_buf = new char[sizeof(btree_key_t) + key_size];
    btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_remove_patch_t::leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            block_size(block_size_t::unsafe_make(0)) {
    guarantee_patch_format(data_length >= sizeof(block_size) + sizeof(uint8_t));
    block_size = *((block_size_t*)(data));
    data += sizeof(block_size);

    uint8_t key_size = *((uint8_t*)(data));
    data += sizeof(key_size);
    key_buf = new char[sizeof(btree_key_t) + key_size];
    try {
        btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
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

    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);

    memcpy(destination, &key->size, sizeof(key->size));
    destination += sizeof(key->size);
    memcpy(destination, key->contents, key->size);
    destination += key->size;
}

uint16_t leaf_remove_patch_t::get_data_size() const {
    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);

    return sizeof(block_size) + sizeof(uint8_t) + key->size;
}

leaf_remove_patch_t::~leaf_remove_patch_t() {
    delete[] key_buf;
}

size_t leaf_remove_patch_t::get_affected_data_size() const {
    const btree_key_t *key = ptr_cast<btree_key_t>(key_buf);
    return key->size + sizeof(key->size);
}

void leaf_remove_patch_t::apply_to_buf(char* buf_data) {
    leaf::remove(ptr_cast<leaf_node_t>(buf_data), ptr_cast<btree_key_t>(key_buf));
}

