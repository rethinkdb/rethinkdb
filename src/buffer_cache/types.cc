#include "buffer_cache/types.hpp"
#include "buffer_cache/large_buf.hpp"

int large_buf_ref::refsize(block_size_t block_size, int64_t size, int64_t offset, lbref_limit_t ref_limit) {
    // We haven't implemented inlining yet.  // TODO probably this comment is out of date.
    return sizeof(large_buf_ref) + sizeof(block_id_t) * large_buf_t::compute_large_buf_ref_num_inlined(block_size, size + offset, ref_limit);
}
int large_buf_ref::refsize(block_size_t block_size, lbref_limit_t ref_limit) const {
    return large_buf_ref::refsize(block_size, this->size, this->offset, ref_limit);
}
