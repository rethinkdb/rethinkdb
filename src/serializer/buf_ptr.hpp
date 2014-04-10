#ifndef SERIALIZER_BUF_PTR_HPP_
#define SERIALIZER_BUF_PTR_HPP_

#include "containers/scoped.hpp"
#include "errors.hpp"
#include "serializer/types.hpp"

// Memory-aligned bufs.
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

    // Allocates a buf_ptr whose block_size() is GREATER THAN OR EQUAL TO size.
    // (It's rounded to the nearest alignment multiple.)
    static buf_ptr alloc(block_size_t size);

    block_size_t block_size() const {
        guarantee(ser_buffer_.has());
        return block_size_;
    }

private:
    block_size_t block_size_;
    scoped_malloc_t<ser_buffer_t> ser_buffer_;

    DISABLE_COPYING(buf_ptr);
};




#endif  /* SERIALIZER_BUF_PTR_HPP_ */
