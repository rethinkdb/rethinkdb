#ifndef SERIALIZER_BUF_PTR_HPP_
#define SERIALIZER_BUF_PTR_HPP_

#include "containers/scoped.hpp"
#include "errors.hpp"
#include "math.hpp"
#include "serializer/types.hpp"

// Memory-aligned bufs.

// Note: This wastes 4 bytes of space on a 64-bit system.  (Arguably, it wastes more
// than that given that block sizes could be 16 bits and pointers are really 48
// bits.)  If you want to optimize page_t, you could store a 32-bit type in here.
class buf_ptr {
public:
    buf_ptr() : block_size_(block_size_t::undefined()) { }
    buf_ptr(buf_ptr &&movee)
        : block_size_(movee.block_size_),
          ser_buffer_(std::move(movee.ser_buffer_)) {
        movee.block_size_ = block_size_t::undefined();
        movee.ser_buffer_.reset();
    }

    buf_ptr &operator=(buf_ptr &&movee) {
        buf_ptr tmp(std::move(movee));
        std::swap(block_size_, tmp.block_size_);
        // RSI: Check that this works on Ubuntu 10.04.
        std::swap(ser_buffer_, tmp.ser_buffer_);
        return *this;
    }

    static buf_ptr alloc(block_size_t size);

    block_size_t size() const {
        guarantee(ser_buffer_.has());
        return block_size_;
    }

    // RSI: Will anybody use this?
    block_size_t room_to_grow() const {
        guarantee(ser_buffer_.has());
        return block_size_t::unsafe_make(
                buf_ptr::compute_in_memory_size(block_size_.ser_value()));
    }

private:
    static uint32_t compute_in_memory_size(uint32_t ser_block_size) {
        return ceil_aligned(ser_block_size, DEVICE_BLOCK_SIZE);
    }

    block_size_t block_size_;
    scoped_malloc_t<ser_buffer_t> ser_buffer_;

    DISABLE_COPYING(buf_ptr);
};




#endif  /* SERIALIZER_BUF_PTR_HPP_ */
