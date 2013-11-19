// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/blob_wrapper.hpp"

#if SLICE_ALT
using namespace alt;  // RSI
#endif

rdb_blob_wrapper_t::rdb_blob_wrapper_t(block_size_t block_size, char *ref,
                                       int maxreflen)
    : internal(block_size, ref, maxreflen) { }

#if SLICE_ALT
rdb_blob_wrapper_t::rdb_blob_wrapper_t(
        block_size_t block_size, char *ref, int maxreflen,
        alt_buf_parent_t parent, const std::string &data)
    : internal(block_size, ref, maxreflen)
#else
rdb_blob_wrapper_t::rdb_blob_wrapper_t(
        block_size_t block_size, char *ref, int maxreflen,
        transaction_t *txn, const std::string &data)
    : internal(block_size, ref, maxreflen)
#endif
{
#ifndef NDEBUG
    /* This is to check that this is actually a new blob that no one else could
     * have a reference to. */
    for (auto it = ref; it < ref + maxreflen; ++it) {
        rassert(*it == 0);
    }
#endif
#if SLICE_ALT
    internal.append_region(parent, data.size());
    internal.write_from_string(data, parent, 0);
#else
    internal.append_region(txn, data.size());
    internal.write_from_string(data, txn, 0);
#endif
}

int rdb_blob_wrapper_t::refsize(block_size_t block_size) const {
    return internal.refsize(block_size);
}

int64_t rdb_blob_wrapper_t::valuesize() const {
    return internal.valuesize();
}

#if SLICE_ALT
void rdb_blob_wrapper_t::expose_all(
        alt_buf_parent_t parent, alt_access_t mode,
        buffer_group_t *buffer_group_out,
        alt::blob_acq_t *acq_group_out) {
#else
void rdb_blob_wrapper_t::expose_all(
        transaction_t *txn, access_t mode,
        buffer_group_t *buffer_group_out,
        blob_acq_t *acq_group_out) {
#endif
#if SLICE_ALT
    guarantee(mode == alt_access_t::read,
        "Other blocks might be referencing this blob, it's invalid to modify it in place.");
#else
    guarantee(mode == rwi_read,
        "Other blocks might be referencing this blob, it's invalid to modify it in place.");
#endif
#if SLICE_ALT
    internal.expose_all(parent, mode, buffer_group_out, acq_group_out);
#else
    internal.expose_all(txn, mode, buffer_group_out, acq_group_out);
#endif
}

#if SLICE_ALT
void rdb_blob_wrapper_t::clear(alt_buf_parent_t parent) {
    internal.clear(parent);
}
#else
void rdb_blob_wrapper_t::clear(transaction_t *txn) {
    internal.clear(txn);
}
#endif
