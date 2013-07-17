// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "buffer_cache/mirrored/page_map.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"

void array_map_t::constructing_inner_buf(mc_inner_buf_t *gbuf) {
    // (WTF, why is this a static method?)
    const block_id_t id = gbuf->block_id;
    array_map_t *const map = &gbuf->cache->page_map;
    rassert(map->array.get(id) == NULL);
    map->array.set(id, gbuf);
    ++map->count;
}

void array_map_t::destroying_inner_buf(mc_inner_buf_t *gbuf) {
    // (WTF, why is this a static method?)
    const block_id_t id = gbuf->block_id;
    array_map_t *const map = &gbuf->cache->page_map;
    rassert(map->array.get(id) != NULL);
    map->array.set(id, NULL);
    --map->count;
}
