#include "buffer_cache/buf_patch.hpp"

/* For now, we have to include all buf_patches files here to allow deserialization */
#include "btree/buf_patches.hpp"

#include <string.h>
#include "utils.hpp"
#include "logger.hpp"

std::string patch_deserialization_message(const char *file, int line, const char *msg) {
    return strprintf("Patch deserialization error%s%s (in %s:%d)",
                     msg[0] ? ": " : "", msg, file, line);
}

patch_deserialization_error_t::patch_deserialization_error_t(const std::string &message) : message_(message) { }

buf_patch_t *buf_patch_t::load_patch(const char *source) {
    try {
        uint16_t remaining_length = *reinterpret_cast<const uint16_t *>(source);
        source += sizeof(remaining_length);
        if (remaining_length == 0) {
            return NULL;
        }
        remaining_length -= sizeof(remaining_length);
        guarantee_patch_format(remaining_length >= sizeof(block_id_t) + sizeof(patch_counter_t) + sizeof(patch_operation_code_t));

        // TODO: Put these fields in a POD struct and do a single memcpy.
        block_id_t block_id = *reinterpret_cast<const block_id_t *>(source);
        source += sizeof(block_id_t);
        remaining_length -= sizeof(block_id_t);
        patch_counter_t patch_counter = *reinterpret_cast<const patch_counter_t *>(source);
        source += sizeof(patch_counter_t);
        remaining_length -= sizeof(block_id_t);
        block_sequence_id_t applies_to_block_sequence_id = *reinterpret_cast<const block_sequence_id_t *>(source);
        source += sizeof(block_sequence_id_t);
        remaining_length -= sizeof(block_sequence_id_t);
        patch_operation_code_t operation_code = *reinterpret_cast<const patch_operation_code_t *>(source);
        source += sizeof(patch_operation_code_t);
        remaining_length -= sizeof(patch_operation_code_t);

        buf_patch_t* result = NULL;
        switch (operation_code) {
        case OPER_MEMCPY:
            result = new memcpy_patch_t(block_id, patch_counter, source, remaining_length);
            break;
        case OPER_MEMMOVE:
            result = new memmove_patch_t(block_id, patch_counter, source, remaining_length);
            break;
        case OPER_LEAF_INSERT:
            result = new leaf_insert_patch_t(block_id, patch_counter, source, remaining_length);
            break;
        case OPER_LEAF_REMOVE:
            result = new leaf_remove_patch_t(block_id, patch_counter, source, remaining_length);
            break;
        case OPER_LEAF_ERASE_PRESENCE:
            result = new leaf_erase_presence_patch_t(block_id, patch_counter, source, remaining_length);
            break;
        default:
            throw patch_deserialization_error_t("Unsupported patch operation code");
        }
        result->set_block_sequence_id(applies_to_block_sequence_id);
        return result;
    } catch (const patch_deserialization_error_t &e) {
        logERR("%s", e.c_str());
        throw e;
    }
}

void buf_patch_t::serialize(char* destination) const {
    uint16_t length = get_serialized_size();

    // TODO: Put these fields in a POD struct and do a single memcpy.
    memcpy(destination, &length, sizeof(length));
    destination += sizeof(length);
    memcpy(destination, &block_id, sizeof(block_id));
    destination += sizeof(block_id);
    memcpy(destination, &patch_counter, sizeof(patch_counter));
    destination += sizeof(patch_counter);
    memcpy(destination, &applies_to_block_sequence_id, sizeof(applies_to_block_sequence_id));
    destination += sizeof(applies_to_block_sequence_id);
    memcpy(destination, &operation_code, sizeof(operation_code));
    destination += sizeof(operation_code);
    serialize_data(destination);
}

buf_patch_t::buf_patch_t(const block_id_t _block_id, const patch_counter_t _patch_counter, const patch_operation_code_t _operation_code) :
            block_id(_block_id),
            patch_counter(_patch_counter),
            applies_to_block_sequence_id(NULL_BLOCK_SEQUENCE_ID),
            operation_code(_operation_code) {
}

bool buf_patch_t::applies_before(const buf_patch_t *p) const {
    rassert(block_id == p->block_id, "Tried to compare incomparable patches");
    return applies_to_block_sequence_id < p->applies_to_block_sequence_id
        || (applies_to_block_sequence_id == p->applies_to_block_sequence_id && patch_counter < p->patch_counter);
}




memcpy_patch_t::memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t _dest_offset, const char* src, const uint16_t n) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMCPY),
            dest_offset(_dest_offset) {
    src_buf.init(n);
    memcpy(src_buf.data(), src, n);
}
memcpy_patch_t::memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char *data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMCPY) {
    uint16_t n;
    guarantee_patch_format(data_length >= sizeof(dest_offset) + sizeof(n));
    dest_offset = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(dest_offset);
    n = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(n);
    guarantee_patch_format(data_length == sizeof(dest_offset) + sizeof(n) + n);
    src_buf.init(n);
    memcpy(src_buf.data(), data, n);

    // Uncomment if you have more to read.
    // data += n;
}

void memcpy_patch_t::serialize_data(char *destination) const {
    memcpy(destination, &dest_offset, sizeof(dest_offset));
    destination += sizeof(dest_offset);
    uint64_t n = src_buf.size();
    memcpy(destination, &n, sizeof(n));
    destination += sizeof(n);
    memcpy(destination, src_buf.data(), n);

    // Uncomment if you have more to write.
    // destination += n;
}
uint16_t memcpy_patch_t::get_data_size() const {
    return sizeof(dest_offset) + sizeof(uint16_t) + src_buf.size();
}

memcpy_patch_t::~memcpy_patch_t() { }

void memcpy_patch_t::apply_to_buf(char* buf_data, UNUSED block_size_t bs) {
    memcpy(buf_data + dest_offset, src_buf.data(), src_buf.size());
}

memmove_patch_t::memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t _dest_offset, const uint16_t _src_offset, const uint16_t _n) :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMMOVE),
            dest_offset(_dest_offset),
            src_offset(_src_offset),
            n(_n) { }

memmove_patch_t::memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length)  :
            buf_patch_t(block_id, patch_counter, buf_patch_t::OPER_MEMMOVE) {
    guarantee_patch_format(data_length == get_data_size());
    dest_offset = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(dest_offset);
    src_offset = *reinterpret_cast<const uint16_t *>(data);
    data += sizeof(src_offset);
    n = *reinterpret_cast<const uint16_t *>(data);

    // Uncomment if you have more to read.
    // data += sizeof(n);
}

void memmove_patch_t::serialize_data(char* destination) const {
    memcpy(destination, &dest_offset, sizeof(dest_offset));
    destination += sizeof(dest_offset);
    memcpy(destination, &src_offset, sizeof(src_offset));
    destination += sizeof(src_offset);
    memcpy(destination, &n, sizeof(n));

    // Uncomment if you have more to write.
    // destination += sizeof(n);
}
uint16_t memmove_patch_t::get_data_size() const {
    return sizeof(dest_offset) + sizeof(src_offset) + sizeof(n);
}

void memmove_patch_t::apply_to_buf(char* buf_data, UNUSED block_size_t bs) {
    memmove(buf_data + dest_offset, buf_data + src_offset, n);
}

