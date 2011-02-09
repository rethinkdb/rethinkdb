#include "buffer_cache/buf_patch.hpp"

#include <string.h>
#include "errors.hpp"

buf_patch_t* buf_patch_t::load_patch(char* source) {
    uint16_t remaining_length = *((uint16_t*)(source));
    source += sizeof(remaining_length);
    if (remaining_length == 0)
        return NULL;
    remaining_length -= sizeof(remaining_length);
    guarantee(remaining_length >= sizeof(block_id) + sizeof(patch_counter) + sizeof(operation_code));
    block_id_t block_id = *((block_id_t*)(source));
    source += sizeof(block_id);
    remaining_length -= sizeof(block_id);
    patch_counter_t patch_counter = *((patch_counter_t*)(source));
    source += sizeof(patch_counter);
    remaining_length -= sizeof(block_id);
    ser_transaction_id_t applies_to_transaction_id = *((ser_transaction_id_t*)(source));
    source += sizeof(applies_to_transaction_id);
    remaining_length -= sizeof(applies_to_transaction_id);
    patch_operation_code_t operation_code = *((patch_operation_code_t*)(source));
    source += sizeof(operation_code);
    remaining_length -= sizeof(operation_code);

    buf_patch_t* result = NULL;
    switch (operation_code) {
        case (OPER_MEMCPY):
            result = new memcpy_patch_t(block_id, patch_counter, source, remaining_length); break;
        case (OPER_MEMMOVE):
            result = new memmove_patch_t(block_id, patch_counter, source, remaining_length); break;
        case (OPER_LEAF_SHIFT_PAIRS):
            result = new leaf_shift_pairs_patch_t(block_id, patch_counter, source, remaining_length); break;
        case (OPER_LEAF_INSERT_PAIR):
            result = new leaf_insert_pair_patch_t(block_id, patch_counter, source, remaining_length); break;
        case (OPER_LEAF_INSERT):
            result = new leaf_insert_patch_t(block_id, patch_counter, source, remaining_length); break;
        case (OPER_LEAF_REMOVE):
            result = new leaf_remove_patch_t(block_id, patch_counter, source, remaining_length); break;
        default:
            guarantee(false, "Unsupported patch operation code");
            return NULL;
    }
    result->set_transaction_id(applies_to_transaction_id);
    return result;
}

void buf_patch_t::serialize(char* destination) const {
    uint16_t length = get_serialized_size();
    memcpy(destination, &length, sizeof(length));
    destination += sizeof(length);
    memcpy(destination, &block_id, sizeof(block_id));
    destination += sizeof(block_id);
    memcpy(destination, &patch_counter, sizeof(patch_counter));
    destination += sizeof(patch_counter);
    memcpy(destination, &applies_to_transaction_id, sizeof(applies_to_transaction_id));
    destination += sizeof(applies_to_transaction_id);
    memcpy(destination, &operation_code, sizeof(operation_code));
    destination += sizeof(operation_code);
    serialize_data(destination);
}

buf_patch_t::buf_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const patch_operation_code_t operation_code) :
            block_id(block_id),
            patch_counter(patch_counter),
            applies_to_transaction_id(NULL_SER_TRANSACTION_ID),
            operation_code(operation_code) {
}

bool buf_patch_t::operator<(const buf_patch_t& p) const {
    rassert(block_id == p.block_id, "Tried to compare incomparable patches");
    return applies_to_transaction_id < p.applies_to_transaction_id || (applies_to_transaction_id == p.applies_to_transaction_id && patch_counter < p.patch_counter);
}





memcpy_patch_t::memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t dest_offset, const char *src, const uint16_t n) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMCPY),
            dest_offset(dest_offset),
            n(n) {
    src_buf = new char[n];
    memcpy(src_buf, src, n);
}
memcpy_patch_t::memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMCPY) {
    guarantee(data_length >= sizeof(dest_offset) + sizeof(n));
    dest_offset = *((uint16_t*)(data));
    data += sizeof(dest_offset);
    n = *((uint16_t*)(data));
    data += sizeof(n);
    guarantee(data_length == sizeof(dest_offset) + sizeof(n) + n);
    src_buf = new char[n];
    memcpy(src_buf, data, n);
    data += n;
}

void memcpy_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &dest_offset, sizeof(dest_offset));
    destination += sizeof(dest_offset);
    memcpy(destination, &n, sizeof(n));
    destination += sizeof(n);
    memcpy(destination, src_buf, n);
    destination += n;
}
uint16_t memcpy_patch_t::get_data_size() const {
    return sizeof(dest_offset) + sizeof(n) + n;
}

memcpy_patch_t::~memcpy_patch_t() {
    delete[] src_buf;
}

size_t memcpy_patch_t::get_affected_data_size() const {
    return n;
}

void memcpy_patch_t::apply_to_buf(char* buf_data) {
    memcpy(buf_data + dest_offset, src_buf, n);
}

memmove_patch_t::memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t dest_offset, const uint16_t src_offset, const uint16_t n) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMMOVE),
            dest_offset(dest_offset),
            src_offset(src_offset),
            n(n) {
}
memmove_patch_t::memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMMOVE) {
    guarantee(data_length == get_data_size());
    dest_offset = *((uint16_t*)(data));
    data += sizeof(dest_offset);
    src_offset = *((uint16_t*)(data));
    data += sizeof(src_offset);
    n = *((uint16_t*)(data));
    data += sizeof(n);
}

void memmove_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &dest_offset, sizeof(dest_offset));
    destination += sizeof(dest_offset);
    memcpy(destination, &src_offset, sizeof(src_offset));
    destination += sizeof(src_offset);
    memcpy(destination, &n, sizeof(n));
    destination += sizeof(n);
}
uint16_t memmove_patch_t::get_data_size() const {
    return sizeof(dest_offset) + sizeof(src_offset) + sizeof(n);
}

size_t memmove_patch_t::get_affected_data_size() const {
    return n;
}

void memmove_patch_t::apply_to_buf(char* buf_data) {
    memmove(buf_data + dest_offset, buf_data + src_offset, n);
}





#include "btree/leaf_node.hpp"

leaf_shift_pairs_patch_t::leaf_shift_pairs_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t offset, const uint16_t shift) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_SHIFT_PAIRS),
            offset(offset),
            shift(shift) {
}

leaf_shift_pairs_patch_t::leaf_shift_pairs_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_SHIFT_PAIRS) {
    guarantee(data_length == get_data_size());
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


leaf_insert_pair_patch_t::leaf_insert_pair_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint8_t value_size, const uint8_t value_metadata_flags, const byte *value_contents, const uint8_t key_size, const char *key_contents) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT_PAIR) {
    value_buf = new byte[sizeof(btree_value) + value_size];
    btree_value *value = ptr_cast<btree_value>(value_buf);
    value->size = value_size;
    value->metadata_flags.flags = value_metadata_flags;
    memcpy(value->contents, value_contents, value_size + metadata_size(value->metadata_flags));

    key_buf = new byte[sizeof(btree_key) + key_size];
    btree_key *key = ptr_cast<btree_key>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_insert_pair_patch_t::leaf_insert_pair_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT_PAIR) {
    guarantee(data_length >= sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t));
    uint8_t value_size = *((uint8_t*)(data));
    data += sizeof(value_size);
    value_buf = new byte[sizeof(btree_value) + value_size];
    btree_value *value = ptr_cast<btree_value>(value_buf);
    value->size = value_size;
    value->metadata_flags.flags = *((uint8_t*)(data));
    data += sizeof(value->metadata_flags.flags);
    guarantee(data_length >= sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + value->size +  metadata_size(value->metadata_flags));
    memcpy(value->contents, data, value->size + metadata_size(value->metadata_flags));
    data += value->size + metadata_size(value->metadata_flags);

    uint8_t key_size = *((uint8_t*)(data));
    data += sizeof(key_size);
    key_buf = new byte[sizeof(btree_key) + key_size];
    btree_key *key = ptr_cast<btree_key>(key_buf);
    key->size = key_size;
    guarantee(data_length == sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + value->size + metadata_size(value->metadata_flags) + key->size);
    memcpy(key->contents, data, key->size);
    data += key->size;
}

void leaf_insert_pair_patch_t::serialize_data(char* destination) const {
    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key *key = ptr_cast<btree_key>(key_buf);

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
    const btree_key *key = ptr_cast<btree_key>(key_buf);

    return sizeof(uint8_t) + sizeof(uint8_t) + value->size + metadata_size(value->metadata_flags) + sizeof(uint8_t) + key->size;
}

leaf_insert_pair_patch_t::~leaf_insert_pair_patch_t() {
    delete[] value_buf;
    delete[] key_buf;
}

size_t leaf_insert_pair_patch_t::get_affected_data_size() const {
    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key *key = ptr_cast<btree_key>(key_buf);
    return key->size + value->size + metadata_size(value->metadata_flags) + sizeof(value->metadata_flags.flags) + sizeof(value->size) + sizeof(key->size);
}

void leaf_insert_pair_patch_t::apply_to_buf(char* buf_data) {
    leaf::impl::insert_pair(ptr_cast<leaf_node_t>(buf_data), ptr_cast<btree_value>(value_buf), ptr_cast<btree_key>(key_buf));
}

leaf_insert_patch_t::leaf_insert_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const block_size_t block_size, const uint8_t value_size, const uint8_t value_metadata_flags, const byte *value_contents, const uint8_t key_size, const char *key_contents, const repli_timestamp insertion_time) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
            block_size(block_size),
            insertion_time(insertion_time) {
    metadata_flags_t metadata_flags;
    metadata_flags.flags = value_metadata_flags;
    value_buf = new byte[sizeof(btree_value) + value_size + metadata_size(metadata_flags)];
    btree_value *value = ptr_cast<btree_value>(value_buf);
    value->size = value_size;
    value->metadata_flags.flags = value_metadata_flags;
    memcpy(value->contents, value_contents, value_size + metadata_size(value->metadata_flags));

    key_buf = new byte[sizeof(btree_key) + key_size];
    btree_key *key = ptr_cast<btree_key>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_insert_patch_t::leaf_insert_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_INSERT),
            block_size(block_size_t::unsafe_make(0)) {
    guarantee(data_length >= sizeof(block_size) + sizeof(insertion_time) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t));
    block_size = *((block_size_t*)(data));
    data += sizeof(block_size);
    insertion_time = *((repli_timestamp*)(data));
    data += sizeof(insertion_time);

    uint8_t value_size = *((uint8_t*)(data));
    data += sizeof(value_size);
    value_buf = new byte[sizeof(btree_value) + value_size];
    btree_value *value = ptr_cast<btree_value>(value_buf);
    value->size = value_size;
    value->metadata_flags.flags = *((uint8_t*)(data));
    data += sizeof(value->metadata_flags.flags);
    guarantee(data_length >= sizeof(block_size) + sizeof(insertion_time) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + value->size + metadata_size(value->metadata_flags));
    memcpy(value->contents, data, value->size + metadata_size(value->metadata_flags));
    data += value->size + metadata_size(value->metadata_flags);

    uint8_t key_size = *((uint8_t*)(data));
    data += sizeof(key_size);
    key_buf = new byte[sizeof(btree_key) + key_size];
    btree_key *key = ptr_cast<btree_key>(key_buf);
    key->size = key_size;
    guarantee(data_length == sizeof(block_size) + sizeof(insertion_time) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + value->size + metadata_size(value->metadata_flags) + key->size);
    memcpy(key->contents, data, key->size);
    data += key->size;
}

void leaf_insert_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &block_size, sizeof(block_size));
    destination += sizeof(block_size);
    memcpy(destination, &insertion_time, sizeof(insertion_time));
    destination += sizeof(insertion_time);

    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key *key = ptr_cast<btree_key>(key_buf);

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
    const btree_key *key = ptr_cast<btree_key>(key_buf);

    return sizeof(block_size) + sizeof(insertion_time) + sizeof(uint8_t) + sizeof(uint8_t) +  value->size + metadata_size(value->metadata_flags) + sizeof(uint8_t) + key->size;
}

leaf_insert_patch_t::~leaf_insert_patch_t() {
    delete[] value_buf;
    delete[] key_buf;
}

size_t leaf_insert_patch_t::get_affected_data_size() const {
    const btree_value *value = ptr_cast<btree_value>(value_buf);
    const btree_key *key = ptr_cast<btree_key>(key_buf);
    return key->size + value->size + metadata_size(value->metadata_flags) + sizeof(value->metadata_flags.flags) + sizeof(value->size) + sizeof(key->size) + sizeof(insertion_time);
}

void leaf_insert_patch_t::apply_to_buf(char* buf_data) {
    leaf::insert(block_size, ptr_cast<leaf_node_t>(buf_data), ptr_cast<btree_key>(key_buf), ptr_cast<btree_value>(value_buf), insertion_time);
}


leaf_remove_patch_t::leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const block_size_t block_size, const uint8_t key_size, const char *key_contents) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            block_size(block_size) {
    key_buf = new byte[sizeof(btree_key) + key_size];
    btree_key *key = ptr_cast<btree_key>(key_buf);
    key->size = key_size;
    memcpy(key->contents, key_contents, key_size);
}

leaf_remove_patch_t::leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_LEAF_REMOVE),
            block_size(block_size_t::unsafe_make(0)) {
    guarantee(data_length >= sizeof(block_size) + sizeof(uint8_t));
    block_size = *((block_size_t*)(data));
    data += sizeof(block_size);

    uint8_t key_size = *((uint8_t*)(data));
    data += sizeof(key_size);
    key_buf = new byte[sizeof(btree_key) + key_size];
    btree_key *key = ptr_cast<btree_key>(key_buf);
    key->size = key_size;
    guarantee(data_length >= sizeof(block_size) + sizeof(uint8_t) + key->size);
    memcpy(key->contents, data, key->size);
    data += key->size;
}

void leaf_remove_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &block_size, sizeof(block_size));
    destination += sizeof(block_size);

    const btree_key *key = ptr_cast<btree_key>(key_buf);

    memcpy(destination, &key->size, sizeof(key->size));
    destination += sizeof(key->size);
    memcpy(destination, key->contents, key->size);
    destination += key->size;
}

uint16_t leaf_remove_patch_t::get_data_size() const {
    const btree_key *key = ptr_cast<btree_key>(key_buf);

    return sizeof(block_size) + sizeof(uint8_t) + key->size;
}

leaf_remove_patch_t::~leaf_remove_patch_t() {
    delete[] key_buf;
}

size_t leaf_remove_patch_t::get_affected_data_size() const {
    const btree_key *key = ptr_cast<btree_key>(key_buf);
    return key->size + sizeof(key->size);
}

void leaf_remove_patch_t::apply_to_buf(char* buf_data) {
    leaf::remove(block_size, ptr_cast<leaf_node_t>(buf_data), ptr_cast<btree_key>(key_buf));
}

