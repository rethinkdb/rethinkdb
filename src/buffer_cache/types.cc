#include "buffer_cache/types.hpp"
#include "buffer_cache/large_buf.hpp"

int large_buf_ref::refsize(block_size_t block_size, int64_t size, int64_t offset) {
    // We haven't implemented inlining yet.  // TODO probably this comment is out of date.
    return sizeof(large_buf_ref) + sizeof(block_id_t) * large_buf_t::compute_large_buf_ref_size(block_size, size + offset);
}
int large_buf_ref::refsize(block_size_t block_size) const {
    return large_buf_ref::refsize(block_size, this->size, this->offset);
}
