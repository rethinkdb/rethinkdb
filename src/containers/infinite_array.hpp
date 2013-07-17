#ifndef CONTAINERS_INFINITE_ARRAY_HPP_
#define CONTAINERS_INFINITE_ARRAY_HPP_

#include <vector>

#include "utils.hpp"

// Represents an infinite array (for non-negative indices) whose values all start
// with zero.  It may not be sparse.

template <class T>
class infinite_array_t {
public:
    infinite_array_t() : nonzero_size_(0) { }

    T get(size_t i) const {
        return i < nonzero_size_ ? vec_[i] : 0;
    }

    void set(size_t i, T value) {
        // Grow vector if necessary.
        if (i >= nonzero_size_) {
            if (value == 0) {
                return;
            }

            nonzero_size_ = i + 1;
            vec_.resize(round_up_to_power_of_two(nonzero_size_), 0);
        }

        vec_[i] = value;

        // Shrink vector if possible.
        if (value == 0 && i == nonzero_size_ - 1) {
            while (nonzero_size_ > 0 && vec_[nonzero_size_ - 1] == 0) {
                --nonzero_size_;
            }
            if (nonzero_size_ <= vec_.size() / 4) {
                vec_.resize(round_up_to_power_of_two(nonzero_size_));
            }
        }
    }

private:
    // A vector whose length is always 0 or a power of 2.
    std::vector<T> vec_;

    // The size of the "non-zero" portion of the vector.  `vec_[nonnull_size_ - 1]`
    // is the last non-zero element of the vector (if nonzero_size_ is not zero).
    // `vec_.size()` is always `round_up_to_power_of_two(nonzero_size_)`.
    //
    // (We explicitly use powers of 2 instead of resize, to avoid O(n log n) average
    // or O(n^2) pathological initial fill cost.
    size_t nonzero_size_;

    DISABLE_COPYING(infinite_array_t);
};


#endif  // CONTAINERS_INFINITE_ARRAY_HPP_
