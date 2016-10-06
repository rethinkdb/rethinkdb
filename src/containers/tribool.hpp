#ifndef CONTAINERS_TRIBOOL_HPP_
#define CONTAINERS_TRIBOOL_HPP_

#include <stdint.h>

#include "containers/archive/archive.hpp"

enum class tribool : int8_t {
    False = 0,
    True = 1,
    Indeterminate = 2,
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(tribool, int8_t, 0, 2);

#endif  // CONTAINERS_TRIBOOL_HPP_
