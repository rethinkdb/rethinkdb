#ifndef __BUFFER_CACHE_TYPES_HPP__
#define __BUFFER_CACHE_TYPES_HPP__

#include "utils2.hpp"
#include "serializer/types.hpp"

typedef uint32_t block_id_t;
#define NULL_BLOCK_ID (block_id_t(-1))

typedef uint32_t block_magic_comparison_t;

struct block_magic_t {
    char bytes[sizeof(block_magic_comparison_t)];

    bool operator==(const block_magic_t& other) const {
        union {
            block_magic_t x;
            block_magic_comparison_t n;
        } u, v;

        u.x = *this;
        v.x = other;

        return u.n == v.n;
    }
};

template <class block_value_t>
bool check_magic(block_magic_t magic) {
    return magic == block_value_t::expected_magic;
}


struct large_buf_ref {
    int64_t size;
    int64_t offset;
    block_id_t block_ids[0];

    // TODO fix these and fix users of these.
    const block_id_t& checker_block_id() const { return block_ids[0]; }
    block_id_t& checker_block_id() { return block_ids[0]; }

    int refsize(block_size_t block_size) const;
    static int refsize(block_size_t block_size, int64_t size, int64_t offset);
} __attribute((__packed__));

#define LARGE_BUF_REF_SIZE (sizeof(large_buf_ref))


#endif /* __BUFFER_CACHE_TYPES_HPP__ */
