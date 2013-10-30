#ifndef CONTAINERS_BACKINDEX_BAG_HPP_
#define CONTAINERS_BACKINDEX_BAG_HPP_

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>

#include "containers/segmented_vector.hpp"

class backindex_bag_index_t {
public:
    backindex_bag_index_t() : index_(SIZE_MAX) { }

    ~backindex_bag_index_t() {
        guarantee(index_ == SIZE_MAX);
    }

private:
    template <class T>
    friend class backindex_bag_t;

    backindex_bag_index_t(size_t index) : index_(index) { }

    // The item's index into a (specific) backindex_bag_t, or SIZE_MAX if it doesn't
    // belong to the backindex_bag_t.
    size_t index_;
    DISABLE_COPYING(backindex_bag_index_t);
};

// A bag of elements that it _does not own_.
template <class T>
class backindex_bag_t {
public:
    typedef backindex_bag_index_t *(*backindex_bag_index_accessor_t)(T);

    explicit backindex_bag_t(backindex_bag_index_accessor_t accessor)
        : accessor_(accessor) { }

    // Retruns true if the potential element of this container is in fact an element
    // of this container.  The idea behind this function is that some value of type T
    // could be a member of one of several backindex_bag_t's (or none).  We see if
    // it's a memory of this one.
    bool has_element(T potential_element) const {
        const backindex_bag_index_t *const backindex = accessor_(potential_element);
        return backindex->index_ < vector_.size()
            && vector_[backindex->index_] == potential_element;
    }

    // Removes the element from the bag.
    void remove(T element) {
        backindex_bag_index_t *const backindex = accessor_(element);
        rassert(backindex->index_ != SIZE_MAX);
        guarantee(backindex->index_ < vector_.size());

        const size_t index = backindex->index_;

        // Move the element in the last position to the removed element's position.
        // The code here feels weird when back_element == element (i.e. when
        // backindex->index_ == back_element_backindex->index_) but it works (I
        // hope).
        const T back_element = vector_.back();
        backindex_bag_index_t *const back_element_backindex = accessor_(back_element);

        rassert(back_element_backindex->index_ == vector_.size() - 1);

        back_element_backindex->index_ = index;
        vector_[index] = back_element;

        vector_.pop_back();

        backindex->index_ = SIZE_MAX;
    }

    // Adds the element to the bag.
    void add(T element) {
        backindex_bag_index_t *const backindex = accessor_(element);
        guarantee(backindex->index_ == SIZE_MAX);

        backindex->index_ = vector_.size();
        vector_.push_back(element);
    }

    size_t size() const {
        return vector_.size();
    }

    T access_random(size_t index) const {
        rassert(index != SIZE_MAX);
        guarantee(index < vector_.size());
        return vector_[index];
    }

private:
    const backindex_bag_index_accessor_t accessor_;

    // RSP: Huge block size, shitty data structure for a bag.
    segmented_vector_t<T> vector_;

    DISABLE_COPYING(backindex_bag_t);
};




#endif  // CONTAINERS_BACKINDEX_BAG_HPP_
