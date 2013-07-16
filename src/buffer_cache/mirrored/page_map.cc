// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "buffer_cache/mirrored/page_map.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"

void array_map_t::constructing_inner_buf(inner_buf_t *gbuf) {
    block_id_t id = gbuf->block_id;
    // (WTF, why is this a static method?)
    array_map_t *map = &gbuf->cache->page_map;

    if (id >= map->array.size()) {
        // This logic prevents pathological O(n^2) resize sequences when we
        // create a bunch of brand new blocks.
        int64_t new_size = round_up_to_power_of_two(id + 1);
        gbuf->cache->page_map.array.resize(new_size, NULL);
    }

    rassert(map->array[id] == NULL);
    map->array[id] = gbuf;
    ++gbuf->cache->page_map.count;
}

void array_map_t::destroying_inner_buf(inner_buf_t *gbuf) {
    // (WTF, why is this a static method?)
    array_map_t *map = &gbuf->cache->page_map;
    block_id_t id = gbuf->block_id;
    rassert(id < map->array.size());
    rassert(map->array[gbuf->block_id] != NULL);
    map->array[gbuf->block_id] = NULL;
    --gbuf->cache->page_map.count;

    // RSI: Might we care about shrinking the array at all when there are unused
    // entries?
}
