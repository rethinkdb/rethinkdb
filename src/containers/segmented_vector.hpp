#ifndef CONTAINERS_SEGMENTED_VECTOR_HPP_
#define CONTAINERS_SEGMENTED_VECTOR_HPP_

#include <stdio.h>

#include "errors.hpp"
#include "logger.hpp"


#define ELEMENTS_PER_SEGMENT (1 << 14)

template<class element_t, size_t max_size>
class segmented_vector_t
{
private:
    struct segment_t {
        element_t elements[ELEMENTS_PER_SEGMENT];
    } *segments[max_size / ELEMENTS_PER_SEGMENT];
    size_t size;

public:
    explicit segmented_vector_t(size_t size = 0) : size(0) {
        set_size(size);
    }

    ~segmented_vector_t() {
        set_size(0);
    }

public:
    element_t &operator[](size_t i) {
        return get(i);
    }

    const element_t &operator[](size_t i) const {
        return const_get(i);
    }

    element_t &get(size_t i) {
        return const_cast<element_t &>(const_get(i));
    }

    size_t get_size() const {
        return size;
    }

    // Note: sometimes elements will be initialized before you ask the
    // array to grow to that size (e.g. one hundred elements might be
    // initialized even though the array might be of size 1).
    void set_size(size_t new_size) {
        rassert(new_size <= max_size);

        size_t num_segs = size ? ((size - 1) / ELEMENTS_PER_SEGMENT) + 1 : 0;
        size_t new_num_segs = new_size ? ((new_size - 1) / ELEMENTS_PER_SEGMENT) + 1 : 0;

        if (num_segs > new_num_segs) {
            for (size_t si = new_num_segs; si < num_segs; si ++) {
                delete segments[si];
            }
        }
        if (new_num_segs > num_segs) {
            for (size_t si = num_segs; si < new_num_segs; si ++) {
                segments[si] = new segment_t;
            }
        }

        size = new_size;
    }

    // This form of set_size fills the newly allocated space with a value
    void set_size(size_t new_size, element_t fill) {
        size_t old_size = size;
        set_size(new_size);
        for (; old_size < new_size; old_size++) (*this)[old_size] = fill;
    }

private:
    const element_t &const_get(size_t i) const {
        if (!(i < size)) {
            logSTDOUT("i is %lu, size is %lu\n", i, size);
        }
        rassert(i < size);

        segment_t *segment = segments[i / ELEMENTS_PER_SEGMENT];
        rassert(segment);
        return segment->elements[i % ELEMENTS_PER_SEGMENT];
    }

    DISABLE_COPYING(segmented_vector_t);
};

#endif  // CONTAINERS_SEGMENTED_VECTOR_HPP_
