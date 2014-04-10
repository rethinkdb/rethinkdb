#include "serializer/buf_ptr.hpp"

#include "math.hpp"

buf_ptr buf_ptr::alloc(block_size_t size) {
    guarantee(size.ser_value() != 0);
    const uint32_t real_size = ceil_aligned(size.ser_value(), DEVICE_BLOCK_SIZE);
    buf_ptr ret;
    ret.block_size_ = block_size_t::unsafe_make(real_size);
    ret.ser_buffer_.init(malloc_aligned(real_size, DEVICE_BLOCK_SIZE));
    return ret;
}
