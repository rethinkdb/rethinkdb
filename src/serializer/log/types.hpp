#ifndef SERIALIZER_LOG_TYPES_HPP_
#define SERIALIZER_LOG_TYPES_HPP_

#include <stdint.h>

#include "serializer/checksum.hpp"

struct checksum_filerange {
    int64_t offset;
    int64_t size;
    // has_checksum(checksum) is always true.
    serializer_checksum checksum;
};


#endif  // SERIALIZER_LOG_TYPES_HPP_
