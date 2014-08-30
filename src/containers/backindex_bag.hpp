#ifndef CONTAINERS_BACKINDEX_BAG_HPP_
#define CONTAINERS_BACKINDEX_BAG_HPP_

#include <stdint.h>

#include "containers/segmented_vector.hpp"

// A backindex_bag_t<T> is a bag of elements of type T.  Typically T is a pointer
// type.  Why not just use an intrusive list?  The reason is, with an intrusive list,
// you can't efficiently select a random element from the list.  A backindex_bag_t
// lets you do that.  It also automatically makes it efficient to detect whether an
// element is contained by a specific bag (while with an intrusive list, you can only
// tell whether the node is an element of _some_ list).  To use, define a type T:
//
// struct T {
//   ...
//   backindex_bag_index_t index;
// };
//
// And then define a function
//
// backindex_bag_index_t *access_backindex(T *ptr) {
//   return &ptr->index;
// }
//
// And then you can use a backindex_bag_t<T *>.
//
// Or you could define a different function,
//
// backindex_bag_index_t *access_backindex(counted_t<T> ptr) {
//   return &ptr->index;
// }
//
// And then you could use a backindex_bag_t< counted_t<T> >.
//
// A backindex_bag_t is essentially a vector, and a backindex_bag_index_t is the
// element's modifiable index in that vector.  The index of other elements can
// change, behind the scenes, as things get added and removed from the bag.

class backindex_bag_index_t {
public:
    backindex_bag_index_t()
        : index_(NOT_IN_A_BAG) { }

    ~backindex_bag_index_t() {
        guarantee(index_ == NOT_IN_A_BAG);
    }

private:
    template <class, size_t>
    friend class backindex_bag_t;

    static const size_t NOT_IN_A_BAG = SIZE_MAX;

    // The item's index into a (specific) backindex_bag_t, or NOT_IN_A_BAG if it
    // doesn't belong to the backindex_bag_t.
    size_t index_;

    DISABLE_COPYING(backindex_bag_index_t);
};

// An unordered bag of elements, maintained using backindexes.  T is typically a
// pointer type (see the description above).  You can randomly select elements and
// test for membership.
template <class T, size_t ELEMENTS_PER_SEGMENT = (1 << 10)>
class backindex_bag_t {
public:
    backindex_bag_t() { }

    ~backindex_bag_t() {
        // Another way to implement this would be to simply remove all its elements.
        guarantee(vector_.size() == 0);
    }

    // Returns true if the potential element of this container is in fact an element
    // of this container.  The idea behind this function is that some value of type T
    // could be a member of one of several backindex_bag_t's (or none).  We see if
    // it's a memory of this one.
    bool has_element(T potential_element) const {
        const backindex_bag_index_t *const backindex
            = access_backindex(potential_element);
        return backindex->index_ < vector_.size()
            && vector_[backindex->index_] == potential_element;
    }

    // Removes the element from the bag.
    void remove(T element) {
        backindex_bag_index_t *const backindex
            = access_backindex(element);
        rassert(backindex->index_ != backindex_bag_index_t::NOT_IN_A_BAG);
        guarantee(backindex->index_ < vector_.size(),
                  "early index has wrong value: index=%zu, size=%zu",
                  backindex->index_, vector_.size());

        const size_t index = backindex->index_;

        // Move the element in the last position to the removed element's position.
        // The code here feels weird when back_element == element (i.e. when
        // backindex->index_ == back_element_backindex->index_) but it works (I
        // hope).
        const T back_element = vector_.back();
        backindex_bag_index_t *const back_element_backindex
            = access_backindex(back_element);

        rassert(back_element_backindex->index_ == vector_.size() - 1,
                "bag %p: index %p has wrong value: index_ = %zu, size = %zu",
                this, back_element_backindex,
                back_element_backindex->index_, vector_.size());

        back_element_backindex->index_ = index;
        vector_[index] = back_element;

        vector_.pop_back();

        backindex->index_ = backindex_bag_index_t::NOT_IN_A_BAG;
    }

    // Adds the element to the bag.
    void add(T element) {
        backindex_bag_index_t *const backindex
            = access_backindex(element);
        guarantee(backindex->index_ == backindex_bag_index_t::NOT_IN_A_BAG,
                  "bag %p, backindex = %p", this, backindex);

        backindex->index_ = vector_.size();
        vector_.push_back(element);
    }

    size_t size() const {
        return vector_.size();
    }

    // Accesses an element by index.  This is called "access random" because
    // hopefully the index was randomly generated.  There are no guarantees about the
    // ordering of elements in this bag.
    T access_random(size_t index) const {
        rassert(index != backindex_bag_index_t::NOT_IN_A_BAG);
        guarantee(index < vector_.size());
        return vector_[index];
    }

private:
    segmented_vector_t<T, ELEMENTS_PER_SEGMENT> vector_;

    DISABLE_COPYING(backindex_bag_t);
};




#endif  // CONTAINERS_BACKINDEX_BAG_HPP_
