#ifndef CONTAINERS_UNALIGNED_HPP_
#define CONTAINERS_UNALIGNED_HPP_

#include "arch/compiler.hpp"

template <class T>
ATTR_PACKED(struct alignas(1) unaligned {
    T value;
    T copy() const { return value; }
});

#endif  // CONTAINERS_UNALIGNED_HPP_
