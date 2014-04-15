#ifndef SERIALIZER_BUF_PTR_HPP_
#define SERIALIZER_BUF_PTR_HPP_

#include "containers/scoped.hpp"
#include "errors.hpp"
#include "math.hpp"
#include "serializer/types.hpp"

// Memory-aligned bufs.  This type also keeps the unused part of the buf (up to the
// DEVICE_BLOCK_SIZE multiple) zeroed out.

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

    // RSI: Remove this.
    buf_ptr(block_size_t size,
            scoped_malloc_t<ser_buffer_t> ser_buffer)
        : block_size_(size),
          ser_buffer_(std::move(ser_buffer)) {
        guarantee(block_size_.ser_value() != 0);
        guarantee(ser_buffer_.has());
    }

    buf_ptr &operator=(buf_ptr &&movee) {
        buf_ptr tmp(std::move(movee));
        std::swap(block_size_, tmp.block_size_);
        // RSI: Check that this works on Ubuntu 10.04.
        std::swap(ser_buffer_, tmp.ser_buffer_);
        return *this;
    }

    void reset() {
        block_size_ = block_size_t::undefined();
        ser_buffer_.reset();
    }

    // Allocates a block, all of whose bytes are zeroed.
    static buf_ptr alloc_zeroed(block_size_t size);

    // Allocates a block, none of whose bytes are zeroed.  Be sure to zero its bytes!
    static buf_ptr alloc_uninitialized(block_size_t size);

    static buf_ptr alloc_copy(const buf_ptr &copyee);

    block_size_t block_size() const {
        guarantee(ser_buffer_.has());
        return block_size_;
    }

    ser_buffer_t *ser_buffer() const {
        guarantee(ser_buffer_.has());
        return ser_buffer_.get();
    }

    void *cache_data() const {
        return ser_buffer()->cache_data;
    }


    // RSI: Will anybody use this?
    block_size_t reserved() const {
        guarantee(ser_buffer_.has());
        return block_size_t::unsafe_make(
                buf_ptr::compute_in_memory_size(block_size_));
    }

    // The buf is DEVICE_BLOCK_SIZE-aligned in both offset and size.  Returns the
    // value of block_size().ser_value() rounded up to the next multiple of
    // DEVICE_BLOCK_SIZE -- this is the true size of the buffer.
    uint32_t aligned_block_size() const {
        return buf_ptr::compute_in_memory_size(block_size_);
    }

    // RSI: Get rid of this.
    void release(block_size_t *block_size_out,
                 scoped_malloc_t<ser_buffer_t> *ser_buffer_out) {
        buf_ptr tmp(std::move(*this));
        *block_size_out = tmp.block_size_;
        *ser_buffer_out = std::move(tmp.ser_buffer_);
    }

    bool has() const {
        return ser_buffer_.has();
    }

    // Increases or decreases the block size of the pointee, reallocating if
    // necessary, filling unused space with zeros.  RSI: I hope we can get rid of
    // this.
    void resize_fill_zero(block_size_t new_size);


private:
    static uint32_t compute_in_memory_size(block_size_t block_size) {
        return ceil_aligned(block_size.ser_value(), DEVICE_BLOCK_SIZE);
    }

    block_size_t block_size_;
    scoped_malloc_t<ser_buffer_t> ser_buffer_;

    DISABLE_COPYING(buf_ptr);
};




#endif  /* SERIALIZER_BUF_PTR_HPP_ */
