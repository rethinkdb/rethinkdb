// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/blob_wrapper.hpp"

#include "buffer_cache/alt.hpp"

rdb_blob_wrapper_t::rdb_blob_wrapper_t(max_block_size_t block_size, char *ref,
                                       int maxreflen)
    : internal(block_size, ref, maxreflen) { }

rdb_blob_wrapper_t::rdb_blob_wrapper_t(
        max_block_size_t block_size, char *ref, int maxreflen,
        buf_parent_t parent, const std::string &data)
    : internal(block_size, ref, maxreflen)
{
#ifndef NDEBUG
    /* This is to check that this is actually a new blob that no one else could
     * have a reference to. */
    for (auto it = ref; it < ref + maxreflen; ++it) {
        rassert(*it == 0);
    }
#endif
    internal.append_region(parent, data.size());
    internal.write_from_string(data, parent, 0);
}

int rdb_blob_wrapper_t::refsize(max_block_size_t block_size) const {
    return internal.refsize(block_size);
}

int64_t rdb_blob_wrapper_t::valuesize() const {
    return internal.valuesize();
}

void rdb_blob_wrapper_t::expose_all(
        buf_parent_t parent, access_t mode,
        buffer_group_t *buffer_group_out,
        blob_acq_t *acq_group_out) {
    guarantee(mode == access_t::read,
        "Other blocks might be referencing this blob, it's invalid to modify it in place.");
    internal.expose_all(parent, mode, buffer_group_out, acq_group_out);
}
