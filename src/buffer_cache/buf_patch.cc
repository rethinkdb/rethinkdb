#include "buffer_cache/buf_patch.hpp"

/* For now, we have to include all buf_patches files here to allow deserialization */
#include "btree/buf_patches.hpp"

#include <string.h>
#include "errors.hpp"
#include "logger.hpp"
#include "buf_patch.hpp"

buf_patch_t* buf_patch_t::load_patch(const char* source) {
    try {
        uint16_t remaining_length = *reinterpret_cast<const uint16_t *>(source);
        source += sizeof(remaining_length);
        if (remaining_length == 0) {
            return NULL;
        }
        remaining_length -= sizeof(remaining_length);
        guarantee_patch_format(remaining_length >= sizeof(block_id_t) + sizeof(patch_counter_t) + sizeof(patch_operation_code_t));
        block_id_t block_id = *reinterpret_cast<const block_id_t *>(source);
        source += sizeof(block_id_t);
        remaining_length -= sizeof(block_id_t);
        patch_counter_t patch_counter = *reinterpret_cast<const patch_counter_t *>(source);
        source += sizeof(patch_counter_t);
        remaining_length -= sizeof(block_id_t);
        ser_transaction_id_t applies_to_transaction_id = *reinterpret_cast<const ser_transaction_id_t *>(source);
        source += sizeof(ser_transaction_id_t);
        remaining_length -= sizeof(ser_transaction_id_t);
        patch_operation_code_t operation_code = *reinterpret_cast<const patch_operation_code_t *>(source);
        source += sizeof(patch_operation_code_t);
        remaining_length -= sizeof(patch_operation_code_t);

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
                guarantee_patch_format(false, "Unsupported patch operation code");
                return NULL;
        }
        result->set_transaction_id(applies_to_transaction_id);
        return result;
    } catch (patch_deserialization_error_t &e) {
        logERR("%s\n", e.c_str());
        throw e;
    }
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




memcpy_patch_t::memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t dest_offset, const char* src, const uint16_t n) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMCPY),
            dest_offset(dest_offset),
            n(n) {
    src_buf = new char[n];
    memcpy(src_buf, src, n);
}
memcpy_patch_t::memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMCPY) {
    guarantee_patch_format(data_length >= sizeof(dest_offset) + sizeof(n));
    dest_offset = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(dest_offset);
    n = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(n);
    guarantee_patch_format(data_length == sizeof(dest_offset) + sizeof(n) + n);
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
    guarantee_patch_format(data_length == get_data_size());
    dest_offset = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(dest_offset);
    src_offset = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(src_offset);
    n = *reinterpret_cast<const uint16_t *>(data);
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

