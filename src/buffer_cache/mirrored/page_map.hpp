// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_MIRRORED_PAGE_MAP_HPP_
#define BUFFER_CACHE_MIRRORED_PAGE_MAP_HPP_

#include "containers/two_level_array.hpp"
#include "config/args.hpp"
#include "buffer_cache/types.hpp"
#include "serializer/types.hpp"

class mc_inner_buf_t;

class array_map_t {
public:
    array_map_t() : count(0) { }

    ~array_map_t() {
        rassert(count == 0);
    }

    static void constructing_inner_buf(mc_inner_buf_t *gbuf);
    static void destroying_inner_buf(mc_inner_buf_t *gbuf);

    mc_inner_buf_t *find(block_id_t block_id) {
        return block_id < array.size() ? array[block_id] : NULL;
    }

    size_t num_pages() const {
        return count;
    }

private:
    std::vector<mc_inner_buf_t *> array;
    // The count of non-null array entries.
    size_t count;
    // The minimum value such that {array[nonnull_back_offset],
    // array[nonnull_back_offset + 1], ... } are all NULL.
    size_t nonnull_back_offset;

    DISABLE_COPYING(array_map_t);
};

#endif // BUFFER_CACHE_MIRRORED_PAGE_MAP_HPP_

