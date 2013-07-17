#ifndef CONTAINERS_INFINITE_ARRAY_HPP_
#define CONTAINERS_INFINITE_ARRAY_HPP_

#include <vector>

#include "utils.hpp"

// Represents an infinite array (for non-negative indices) whose values all start
// with zero (or default-constructed values).  It may not be sparse (you'll waste
// memory).

template <class T>
class infinite_array_t {
public:
    infinite_array_t() : nonzero_size_(0) { }

    T get(size_t i) const {
        return i < nonzero_size_ ? vec_[i] : T();
    }

    void set(size_t i, T value) {
        // Grow vector if necessary.
        if (i >= nonzero_size_) {
            if (value == T()) {
                return;
            }

            nonzero_size_ = i + 1;
            vec_.resize(round_up_to_power_of_two(nonzero_size_));
        }

        vec_[i] = value;

        // Shrink vector if possible.
        if (value == T() && i == nonzero_size_ - 1) {
            while (nonzero_size_ > 0 && vec_[nonzero_size_ - 1] == T()) {
                --nonzero_size_;
            }
            if (nonzero_size_ <= vec_.size() / 4) {
                vec_.resize(round_up_to_power_of_two(nonzero_size_));
            }
        }
    }

    T &operator[](size_t i) {
        // Grow vector if necessary.
        if (i >= nonzero_size_) {
            nonzero_size_ = i + 1;
            vec_.resize(round_up_to_power_of_two(nonzero_size_));
        }

        // Shrink vector if possible.
        while (nonzero_size_ > i + 1 && vec_[nonzero_size_ - 1] == T()) {
            --nonzero_size_;
        }
        if (nonzero_size_ <= vec_.size() / 4) {
            vec_.resize(round_up_to_power_of_two(nonzero_size_));
        }

        return vec_[i];
    }

private:
    // A vector whose length is always 0 or a power of 2.
    std::vector<T> vec_;

    // The size of the "non-zero" portion of the vector.  All of the elements after
    // this value are zero (or default-constructed).  If you don't use operator[],
    // this will be as low a value is possible -- vec_[nonzero_size_ - 1] will have a
    // nonzero value.
    //
    // (We explicitly use powers of 2 instead of resize, to avoid O(n log n) average
    // or O(n^2) pathological initial fill cost.
    size_t nonzero_size_;

    DISABLE_COPYING(infinite_array_t);
};


#endif  // CONTAINERS_INFINITE_ARRAY_HPP_
