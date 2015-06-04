#ifndef SERIALIZER_BUF_PTR_HPP_
#define SERIALIZER_BUF_PTR_HPP_

#include <utility>

#include "containers/scoped.hpp"
#include "errors.hpp"
#include "math.hpp"
#include "serializer/types.hpp"

// Memory-aligned bufs.  This type also keeps the unused part of the buf (up to the
// DEVICE_BLOCK_SIZE multiple) zeroed out.

// Note: This wastes 4 bytes of space on a 64-bit system.  (Arguably, it wastes more
// than that given that block sizes could be 16 bits and pointers are really 48
// bits.)  If you want to optimize page_t, you could store a 32-bit type in here.
class buf_ptr_t {
public:
    buf_ptr_t() : block_size_(block_size_t::undefined()) { }
    buf_ptr_t(buf_ptr_t &&movee)
        : block_size_(movee.block_size_),
          ser_buffer_(std::move(movee.ser_buffer_)) {
        movee.block_size_ = block_size_t::undefined();
        movee.ser_buffer_.reset();
    }

    buf_ptr_t(block_size_t size,
            scoped_aligned_malloc_t<ser_buffer_t> ser_buffer)
        : block_size_(size),
          ser_buffer_(std::move(ser_buffer)) {
        guarantee(block_size_.ser_value() != 0);
        guarantee(ser_buffer_.has());
    }

    buf_ptr_t &operator=(buf_ptr_t &&movee) {
        buf_ptr_t tmp(std::move(movee));
        std::swap(block_size_, tmp.block_size_);
        std::swap(ser_buffer_, tmp.ser_buffer_);
        return *this;
    }

    void reset() {
        block_size_ = block_size_t::undefined();
        ser_buffer_.reset();
    }

    // Allocates a block, all of whose bytes are zeroed.
    static buf_ptr_t alloc_zeroed(block_size_t size);

    // Allocates a block, none of whose bytes are zeroed.  Be sure to zero its bytes!
    static buf_ptr_t alloc_uninitialized(block_size_t size);

    static buf_ptr_t alloc_copy(const buf_ptr_t &copyee);

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

    // Returns the actual allocated size of the buffer, wich is
    // DEVICE_BLOCK_SIZE-aligned.  (Returns the value of block_size().ser_value()
    // rounded up to the next multiple of DEVICE_BLOCK_SIZE.)
    uint32_t aligned_block_size() const {
        guarantee(ser_buffer_.has());
        return buf_ptr_t::compute_aligned_block_size(block_size_);
    }

    // Returns what aligned_block_size() would return for a buf_ptr_t that has the
    // given block size.
    static uint32_t compute_aligned_block_size(block_size_t block_size) {
        return ceil_aligned(block_size.ser_value(), DEVICE_BLOCK_SIZE);
    }

    void release(block_size_t *block_size_out,
                 scoped_aligned_malloc_t<ser_buffer_t> *ser_buffer_out) {
        buf_ptr_t tmp(std::move(*this));
        *block_size_out = tmp.block_size_;
        *ser_buffer_out = std::move(tmp.ser_buffer_);
    }

    bool has() const {
        return ser_buffer_.has();
    }

    // Increases or decreases the block size of the pointee, reallocating if
    // necessary, filling unused space with zeros.
    void resize_fill_zero(block_size_t new_size);

    // Fills the padding space with zeroes.  Generally speaking, you want to write
    // blocks with zero padding, not uninitialized memory or other arbitrary scruff.
    void fill_padding_zero() const;

#ifndef NDEBUG
    void assert_padding_zero() const;
#else
    void assert_padding_zero() const { }
#endif


private:
    // Valid only when ser_buffer_.has() is true.  Contains the size of the buffer as
    // exposed to outside users of the cache.  The buffer is actually allocated to
    // size `compute_aligned_block_size(block_size_)` (the next multiple of
    // DEVICE_BLOCK_SIZE), and the extra space is left zero-padded, so that we can
    // more efficiently write the buffer to disk.
    block_size_t block_size_;
    // The buffer, or empty if this buf_ptr_t is empty.
    scoped_aligned_malloc_t<ser_buffer_t> ser_buffer_;

    DISABLE_COPYING(buf_ptr_t);
};




#endif  // SERIALIZER_BUF_PTR_HPP_
