// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONTAINERS_LAZY_ERASE_VECTOR_HPP_
#define CONTAINERS_LAZY_ERASE_VECTOR_HPP_

#include <vector>

/* lazy_erase_vector_t is like an std::vector, except that it offers an erase_front()
operation that batches small deletions together to be a bit more efficient.

It currently doesn't implement the full std::vector interface, but feel free
to extend if if you need some additional functionality. */

template<class type_t>
class lazy_erase_vector_t {
public:
    explicit lazy_erase_vector_t(size_t erase_threshold)
        : erased_offset_(0), erase_threshold_(erase_threshold) { }

    size_t size() const {
        return vec_.size() - erased_offset_;
    }

    type_t *data() {
        return vec_.data() + erased_offset_;
    }

    const type_t *data() const {
        return vec_.data() + erased_offset_;
    }

    void resize(size_t new_size) {
        vec_.resize(new_size + erased_offset_);
    }

    void erase_front(size_t num_elems) {
        erased_offset_ += num_elems;
        if (erased_offset_ >= erase_threshold_) {
            vec_.erase(vec_.begin(), vec_.begin() + erased_offset_);
            erased_offset_ = 0;
        }
    }

private:
    std::vector<type_t> vec_;
    // This many elements have been erased from the front of `vec`
    size_t erased_offset_;
    // Actually erase from vec_ once erased_offset_ becomes larger than this.
    size_t erase_threshold_;
};

#endif  // CONTAINERS_LAZY_ERASE_VECTOR_HPP_
