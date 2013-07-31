// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_SEGMENTED_VECTOR_HPP_
#define CONTAINERS_SEGMENTED_VECTOR_HPP_

#include <stdio.h>

#include "errors.hpp"

template <class element_t>
class segmented_vector_t {
private:
    static const size_t ELEMENTS_PER_SEGMENT = 1 << 14;

    struct segment_t {
        element_t elements[ELEMENTS_PER_SEGMENT];
    };

    std::vector<segment_t *> segments_;
    size_t size_;

public:
    explicit segmented_vector_t(size_t size = 0) : size_(0) {
        set_size(size);
    }

    ~segmented_vector_t() {
        set_size(0);
    }

public:
    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    element_t &operator[](size_t index) {
        guarantee(index < size_, "index = %zu, size_ = %zu", index, size_);
        segment_t *seg = segments_[index / ELEMENTS_PER_SEGMENT];
        return seg->elements[index % ELEMENTS_PER_SEGMENT];
    }

    void push_back(const element_t &element) {
        size_t old_size = size_;
        set_size(old_size + 1);
        (*this)[old_size] = element;
    }

    element_t &back() {
        return (*this)[size_ - 1];
    }

    void pop_back() {
        guarantee(size_ > 0);
        set_size(size_ - 1);
    }

private:
    // Note: sometimes elements will be initialized before you ask the
    // array to grow to that size (e.g. one hundred elements might be
    // initialized even though the array might be of size 1).
    void set_size(size_t new_size) {
        {
            const size_t num_segs = size_ != 0 ? ((size_ - 1) / ELEMENTS_PER_SEGMENT) + 1 : 0;
            guarantee(num_segs == segments_.size());
        }
        const size_t new_num_segs = new_size != 0 ? ((new_size - 1) / ELEMENTS_PER_SEGMENT) + 1 : 0;

        while (segments_.size() > new_num_segs) {
            delete segments_.back();
            segments_.pop_back();
        }
        while (segments_.size() < new_num_segs) {
            segments_.push_back(new segment_t);
        }

        size_ = new_size;
    }

    DISABLE_COPYING(segmented_vector_t);
};

#endif  // CONTAINERS_SEGMENTED_VECTOR_HPP_
