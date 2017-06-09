// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONTAINERS_SEGMENTED_VECTOR_HPP_
#define CONTAINERS_SEGMENTED_VECTOR_HPP_

#include <stdio.h>

#include <memory>
#include <utility>
#include <vector>

#include "errors.hpp"

template <class element_t, size_t ELEMENTS_PER_SEGMENT = (1 << 14)>
class segmented_vector_t {
public:
    explicit segmented_vector_t(size_t _size = 0) : size_(0) {
        set_size(_size);
    }

    segmented_vector_t(segmented_vector_t &&movee)
        : segments_(std::move(movee.segments_)), size_(movee.size_) {
        movee.segments_.clear();
        movee.size_ = 0;
    }

    segmented_vector_t &operator=(segmented_vector_t &&movee) {
        segmented_vector_t tmp(std::move(movee));
        std::swap(segments_, tmp.segments_);
        std::swap(size_, tmp.size_);
        return *this;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    // Gets an element, without creating a segment_t if its segment doesn't exist.
    // (Thus, this method doesn't return a reference.)
    element_t get_sparsely(size_t index) const {
        guarantee(index < size_, "index = %zu, size_ = %zu", index, size_);
        const size_t segment_index = index / ELEMENTS_PER_SEGMENT;
        segment_t *seg = segments_[segment_index];
        if (seg == NULL) {
            return element_t();
        } else {
            return seg->elements[index % ELEMENTS_PER_SEGMENT];
        }
    }

    element_t &operator[](size_t index) {
        return get_element(index);
    }

    const element_t &operator[](size_t index) const {
        return get_element(index);
    }

    void push_back(const element_t &element) {
        size_t old_size = size_;
        set_size(old_size + 1);
        (*this)[old_size] = element;
    }

    void push_back(element_t &&element) {
        size_t old_size = size_;
        set_size(old_size + 1);
        (*this)[old_size] = std::move(element);
    }

    element_t &back() {
        return (*this)[size_ - 1];
    }

    void pop_back() {
        guarantee(size_ > 0);
        set_size(size_ - 1);
    }

    void clear() {
        segments_.clear();
        size_ = 0;
    }

    void resize_with_zeros(size_t new_size) {
        set_size(new_size);
    }

private:
    // Gets a non-const element with a const function... which breaks the guarantees
    // but compiles and lets this be called by both the const and non-const versions
    // of operator[].
    element_t &get_element(size_t index) const {
        guarantee(index < size_, "index = %zu, size_ = %zu", index, size_);
        const size_t segment_index = index / ELEMENTS_PER_SEGMENT;
        segment_t *seg = segments_[segment_index].get();
        if (seg == nullptr) {
            seg = new segment_t();
            segments_[segment_index].reset(seg);
        }
        return seg->elements[index % ELEMENTS_PER_SEGMENT];
    }

    // Note: sometimes elements will be initialized before you ask the
    // array to grow to that size (e.g. one hundred elements might be
    // initialized even though the array might be of size 1).
    //
    // Also some will remain initialized when you shrink.
    void set_size(size_t new_size) {
        const size_t new_num_segs = (new_size + (ELEMENTS_PER_SEGMENT - 1)) / ELEMENTS_PER_SEGMENT;

        const size_t cap = segments_.capacity();

        if (new_num_segs <= cap) {
            // Prevent resizing down to zero -- avoids unnecessary
            // deallocation+allocation when doing push/pop/push/pop on
            // zero-length segmented_vector.
            if (new_num_segs != 0) {
                segments_.resize(new_num_segs);
            }
        } else {
            // Force 2x growth.
            if (new_num_segs < cap * 2) {
                segments_.reserve(cap * 2);
            }
            segments_.resize(new_num_segs);
        }

        size_ = new_size;
    }

    struct segment_t {
        segment_t() : elements() { }  // Zero-initialize array.
        element_t elements[ELEMENTS_PER_SEGMENT];
    };

    mutable std::vector<std::unique_ptr<segment_t>> segments_;
    size_t size_;

    DISABLE_COPYING(segmented_vector_t);
};

#endif  // CONTAINERS_SEGMENTED_VECTOR_HPP_
