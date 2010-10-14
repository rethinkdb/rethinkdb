
#ifndef __SEGMENTED_VECTOR_HPP__
#define __SEGMENTED_VECTOR_HPP__

#include "config/alloc.hpp"

#define ELEMENTS_PER_SEGMENT (1 << 14)

template<class element_t, int max_size>
class segmented_vector_t
{
private:
    struct segment_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, segment_t > {
        element_t elements[ELEMENTS_PER_SEGMENT];
    } *segments[max_size / ELEMENTS_PER_SEGMENT];
    size_t size;
    
public:
    segmented_vector_t(size_t size = 0) : size(0) {
        set_size(size);
    }
    
    ~segmented_vector_t() {
        set_size(0);
    }

public:
    element_t &operator[](unsigned i) {
    
        assert(i < size);
        
        segment_t *segment = segments[i / ELEMENTS_PER_SEGMENT];
        assert(segment);
        return segment->elements[i % ELEMENTS_PER_SEGMENT];
    }
    
    size_t get_size() {
    
        return size;
    }
    
    // Note: sometimes elements will be initialized before you ask the
    // array to grow to that size (e.g. one hundred elements might be
    // initialized eventhough the array might be of size 1).
    void set_size(size_t new_size) {
        
        assert(new_size < max_size);
        
        unsigned num_segs = size ? ((size - 1) / ELEMENTS_PER_SEGMENT) + 1 : 0;
        unsigned new_num_segs = new_size ? ((new_size - 1) / ELEMENTS_PER_SEGMENT) + 1 : 0;
        
        if (num_segs > new_num_segs) {
            for (unsigned si = new_num_segs; si < num_segs; si ++) {
                delete segments[si];
            }
        }
        if (new_num_segs > num_segs) {
            for (unsigned si = num_segs; si < new_num_segs; si ++) {
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
};

#endif /* __SEGMENTED_VECTOR_HPP_ */
