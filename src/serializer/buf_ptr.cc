#include "serializer/buf_ptr.hpp"

#include <algorithm>

#include "math.hpp"

buf_ptr_t buf_ptr_t::alloc_uninitialized(block_size_t size) {
    guarantee(size.ser_value() != 0);
    const size_t count = compute_aligned_block_size(size);
    buf_ptr_t ret;
    ret.block_size_ = size;
    ret.ser_buffer_ = scoped_device_block_aligned_ptr_t<ser_buffer_t>(count);
    return ret;
}

buf_ptr_t buf_ptr_t::alloc_zeroed(block_size_t size) {
    buf_ptr_t ret = alloc_uninitialized(size);
    char *buf = reinterpret_cast<char *>(ret.ser_buffer());
    memset(buf, 0, compute_aligned_block_size(size));
    return ret;
}

scoped_device_block_aligned_ptr_t<ser_buffer_t>
help_allocate_copy(const ser_buffer_t *copyee, size_t amount_to_copy,
                   size_t reserved_size) {
    rassert(amount_to_copy <= reserved_size);
    auto buf = scoped_device_block_aligned_ptr_t<ser_buffer_t>(reserved_size);
    memcpy(buf.get(), copyee, amount_to_copy);
    memset(reinterpret_cast<char *>(buf.get()) + amount_to_copy,
           0,
           reserved_size - amount_to_copy);
    return buf;
}

buf_ptr_t buf_ptr_t::alloc_copy(const buf_ptr_t &copyee) {
    guarantee(copyee.has());
    return buf_ptr_t(copyee.block_size(),
                   help_allocate_copy(copyee.ser_buffer_.get(),
                                      copyee.block_size().ser_value(),
                                      copyee.aligned_block_size()));
}

void buf_ptr_t::resize_fill_zero(block_size_t new_size) {
    guarantee(new_size.ser_value() != 0);
    guarantee(ser_buffer_.has());

    uint16_t old_reserved = compute_aligned_block_size(block_size_);
    uint16_t new_reserved = compute_aligned_block_size(new_size);

    if (old_reserved == new_reserved) {
        if (new_size.ser_value() < block_size_.ser_value()) {
            // Set the newly unused part of the block to zero.
            memset(reinterpret_cast<char *>(ser_buffer_.get()) + new_size.ser_value(),
                   0,
                   block_size_.ser_value() - new_size.ser_value());
        }
    } else {
        // We actually need to reallocate.
        scoped_device_block_aligned_ptr_t<ser_buffer_t> buf
            = help_allocate_copy(ser_buffer_.get(),
                                 std::min(block_size_.ser_value(),
                                          new_size.ser_value()),
                                 new_reserved);

        ser_buffer_ = std::move(buf);
    }
    block_size_ = new_size;
}

void buf_ptr_t::fill_padding_zero() const {
    guarantee(has());
    char *p = reinterpret_cast<char *>(ser_buffer());
    uint16_t ser_block_size = block_size().ser_value();
    uint16_t aligned = aligned_block_size();
    memset(p + ser_block_size, 0, aligned - ser_block_size);
}

#ifndef NDEBUG
void buf_ptr_t::assert_padding_zero() const {
    guarantee(has());
    char *p = reinterpret_cast<char *>(ser_buffer());
    uint16_t ser_block_size = block_size().ser_value();
    uint16_t aligned = aligned_block_size();
    for (uint16_t i = ser_block_size; i < aligned; ++i) {
        rassert(p[i] == 0);
    }
}
#endif

