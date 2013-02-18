// Copyright 2010-2012 RethinkDB, all rights reserved.
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

void buf_patch_t::serialize(char* destination) const {
    uint16_t length = get_serialized_size();

    // TODO: Put these fields in a POD struct and do a single memcpy.
    memcpy(destination, &length, sizeof(length));
    destination += sizeof(length);
    memcpy(destination, &block_id, sizeof(block_id));
    destination += sizeof(block_id);
    memcpy(destination, &operation_code, sizeof(operation_code));
    destination += sizeof(operation_code);
    serialize_data(destination);
}

buf_patch_t::buf_patch_t(const block_id_t _block_id, const patch_operation_code_t _operation_code) :
    block_id(_block_id),
    operation_code(_operation_code) { }
