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
    compress_to_blob(&internal, txn, data);
}

int rdb_blob_wrapper_t::refsize(block_size_t block_size) const {
    return internal.refsize(block_size);
}

int64_t rdb_blob_wrapper_t::valuesize() const {
    return internal.valuesize();
}

std::vector<char> *rdb_blob_wrapper_t::get_data(transaction_t *txn) {
    decompress_from_blob(&internal, txn, &decompressed_data);
    return &decompressed_data;
}

void rdb_blob_wrapper_t::clear(transaction_t *txn) {
    internal.clear(txn);
}
