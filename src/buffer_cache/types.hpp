// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_TYPES_HPP_
#define BUFFER_CACHE_TYPES_HPP_

#include <limits.h>
#include <stdint.h>

#include "containers/archive/archive.hpp"
#include "serializer/types.hpp"

// write_durability_t::INVALID is an invalid value, notably it can't be serialized.
enum class write_durability_t { INVALID, SOFT, HARD };
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(write_durability_t, int8_t,
                                      write_durability_t::SOFT,
                                      write_durability_t::HARD);


typedef uint32_t block_magic_comparison_t;

struct block_magic_t {
    char bytes[sizeof(block_magic_comparison_t)];

    bool operator==(const block_magic_t &other) const {
        union {
            block_magic_t x;
            block_magic_comparison_t n;
        } u, v;

        u.x = *this;
        v.x = other;

        return u.n == v.n;
    }

    bool operator!=(const block_magic_t &other) const {
        return !(operator==(other));
    }
};

void debug_print(printf_buffer_t *buf, block_magic_t magic);

#endif /* BUFFER_CACHE_TYPES_HPP_ */
