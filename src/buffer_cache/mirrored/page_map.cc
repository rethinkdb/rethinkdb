// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "buffer_cache/mirrored/page_map.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"

void array_map_t::constructing_inner_buf(mc_inner_buf_t *gbuf) {
    // (WTF, why is this a static method?)
    const block_id_t id = gbuf->block_id;
    array_map_t *const map = &gbuf->cache->page_map;

    if (id >= map->array.size()) {
        // This logic prevents pathological O(n^2) resize sequences when we
        // create a bunch of brand new blocks.
        const int64_t new_size = round_up_to_power_of_two(id + 1);
        gbuf->cache->page_map.array.resize(new_size, NULL);
    }

    rassert(map->array[id] == NULL);
    map->array[id] = gbuf;
    if (id >= map->nonnull_back_offset) {
        map->nonnull_back_offset = id + 1;
    }

    ++map->count;
}

void array_map_t::destroying_inner_buf(mc_inner_buf_t *gbuf) {
    // (WTF, why is this a static method?)
    const block_id_t id = gbuf->block_id;
    array_map_t *const map = &gbuf->cache->page_map;
    rassert(id < map->array.size());
    rassert(map->array[gbuf->block_id] != NULL);
    map->array[gbuf->block_id] = NULL;

    // Horrid logic to shrink the array (and save precious memory) when block id
    // values no longer need all the room in the array.
    if (id + 1 == map->nonnull_back_offset) {
        do {
            --map->nonnull_back_offset;
        } while (map->nonnull_back_offset > 0 &&
                 map->array[map->nonnull_back_offset - 1] == NULL);

        if (map->nonnull_back_offset <= map->array.size() / 2) {
            map->array.resize(round_up_to_power_of_two(map->nonnull_back_offset));
        }
    }

    --map->count;
}
