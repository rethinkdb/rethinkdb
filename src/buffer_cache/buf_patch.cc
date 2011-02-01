#include "buffer_cache/buf_patch.hpp"

#include <string.h>
#include "errors.hpp"

buf_patch_t* buf_patch_t::load_patch(char* source) {
    size_t remaining_length = *((size_t*)(source));
    source += sizeof(remaining_length);
    if (remaining_length == 0)
        return NULL;
    guarantee(remaining_length >= sizeof(block_id) + sizeof(patch_counter) + sizeof(operation_code));
    block_id_t block_id = *((block_id_t*)(source));
    source += sizeof(block_id);
    patch_counter_t patch_counter = *((patch_counter_t*)(source));
    source += sizeof(patch_counter);
    ser_transaction_id_t applies_to_transaction_id = *((ser_transaction_id_t*)(source));
    source += sizeof(applies_to_transaction_id);
    patch_operation_code_t operation_code = *((patch_operation_code_t*)(source));
    source += sizeof(operation_code);

    switch (operation_code) {
        case (OPER_MEMCPY):
            return new memcpy_patch_t(block_id, patch_counter, applies_to_transaction_id, source, remaining_length);
        case (OPER_MEMMOVE):
            return new memcpy_patch_t(block_id, patch_counter, applies_to_transaction_id, source, remaining_length);
        default:
            guarantee(false, "Unsupported patch operation code");
            return NULL;
    }
}

void buf_patch_t::serialize(char* destination) const {
    size_t length = get_serialized_size();
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

size_t buf_patch_t::get_serialized_size() const {
    return sizeof(size_t) + sizeof(block_id) + sizeof(patch_counter) + sizeof(applies_to_transaction_id) + sizeof(operation_code) + get_data_size();
}

patch_counter_t buf_patch_t::get_patch_counter() const {
    return patch_counter;
}

ser_transaction_id_t buf_patch_t::get_transaction_id() const {
    return applies_to_transaction_id;
}

block_id_t buf_patch_t::get_block_id() const {
    return block_id;
}

buf_patch_t::buf_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const patch_operation_code_t operation_code) :
            block_id(block_id),
            patch_counter(patch_counter),
            applies_to_transaction_id(applies_to_transaction_id),
            operation_code(operation_code) {
}

bool buf_patch_t::operator<(const buf_patch_t& p) const {
    rassert(block_id == p.block_id, "Tried to compare incomparable patches");
    return applies_to_transaction_id < p.applies_to_transaction_id || patch_counter < p.patch_counter;
}

memcpy_patch_t::memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const size_t dest_offset, const char *src, const size_t n) :
            buf_patch_t(block_id, patch_counter, applies_to_transaction_id, buf_patch_t::OPER_MEMCPY),
            dest_offset(dest_offset),
            n(n) {
    src_buf = new char[n];
    memcpy(src_buf, src, n);
}
memcpy_patch_t::memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const char* data, const size_t data_length)  :
            buf_patch_t(block_id, patch_counter, applies_to_transaction_id, buf_patch_t::OPER_MEMCPY) {
    guarantee(data_length >= sizeof(dest_offset) + sizeof(n));
    dest_offset = *((size_t*)(data));
    data += sizeof(dest_offset);
    n = *((size_t*)(data));
    data += sizeof(n);
    fprintf(stderr, "remaining data_length is %d, n is %d\n", (int)(data_length - sizeof(dest_offset) - sizeof(n)), (int)n); // TODO!
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
size_t memcpy_patch_t::get_data_size() const {
    return sizeof(dest_offset) + sizeof(n) + n;
}

memcpy_patch_t::~memcpy_patch_t() {
    delete[] src_buf;
}

void memcpy_patch_t::apply_to_buf(char* buf_data) {
    memcpy(buf_data + dest_offset, src_buf, n);
}

memmove_patch_t::memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const size_t dest_offset, const size_t src_offset, const size_t n) :
            buf_patch_t(block_id, patch_counter, applies_to_transaction_id, buf_patch_t::OPER_MEMMOVE),
            src_offset(src_offset),
            dest_offset(dest_offset),
            n(n) {
}
memmove_patch_t::memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const char* data, const size_t data_length)  :
            buf_patch_t(block_id, patch_counter, applies_to_transaction_id, buf_patch_t::OPER_MEMMOVE) {
    guarantee(data_length == sizeof(dest_offset) + sizeof(src_offset) + sizeof(n));
    dest_offset = *((size_t*)(data));
    data += sizeof(dest_offset);
    src_offset = *((size_t*)(data));
    data += sizeof(src_offset);
    n = *((size_t*)(data));
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
size_t memmove_patch_t::get_data_size() const {
    return sizeof(dest_offset) + sizeof(src_offset) + sizeof(n);
}

void memmove_patch_t::apply_to_buf(char* buf_data) {
    memmove(buf_data + dest_offset, buf_data + src_offset, n);
}

