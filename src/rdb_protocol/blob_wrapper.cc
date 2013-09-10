#include <snappy.h>

#include "compression/blob_adapter.hpp"
#include "rdb_protocol/blob_wrapper.hpp"

rdb_blob_wrapper_t::rdb_blob_wrapper_t(block_size_t blk_size, char *ref, int maxreflen) 
    : internal(blk_size, ref, maxreflen) { }

rdb_blob_wrapper_t::rdb_blob_wrapper_t(
        block_size_t blk_size, char *ref, int maxreflen,
        transaction_t *txn, const std::string &data) 
    : internal(blk_size, ref, maxreflen)
{
#ifndef NDEBUG
    /* This is to check that this is actually a new blob that no one else could
     * have a reference to. */
    for (auto it = ref; it < ref + maxreflen; ++it) {
        rassert(*it == 0);
    }
#endif
    snappy::ByteArraySource src(data.data(), data.size());
    blob_sink_t sink(&internal, txn);

    snappy::Compress(&src, &sink);
}

int rdb_blob_wrapper_t::refsize(block_size_t block_size) const {
    return internal.refsize(block_size);
}

int64_t rdb_blob_wrapper_t::valuesize() const {
    return internal.valuesize();
}

void rdb_blob_wrapper_t::expose_all(
        transaction_t *txn, access_t mode, 
        buffer_group_t *buffer_group_out, 
        blob_acq_t *acq_group_out) {
    guarantee(mode == rwi_read,
        "Other blocks might be referencing this blob, it's invalid to modify it in place.");
    internal.expose_all(txn, mode, buffer_group_out, acq_group_out);
}

void rdb_blob_wrapper_t::clear(transaction_t *txn) {
    internal.clear(txn);
}
