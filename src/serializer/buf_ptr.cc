#include "serializer/buf_ptr.hpp"

#include "math.hpp"

buf_ptr buf_ptr::alloc(block_size_t size) {
    guarantee(size.ser_value() != 0);
    buf_ptr ret;
    ret.block_size_ = size;
    ret.ser_buffer_.init(malloc_aligned(compute_in_memory_size(size.ser_value()),
                                        DEVICE_BLOCK_SIZE));
    return ret;
}
