// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "buffer_cache/mirrored/page_map.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"

void array_map_t::constructing_inner_buf(inner_buf_t *gbuf) {
    rassert(!gbuf->cache->page_map.array.get(gbuf->block_id));
    gbuf->cache->page_map.array.set(gbuf->block_id, gbuf);
}

void array_map_t::destroying_inner_buf(inner_buf_t *gbuf) {
    rassert(gbuf->cache->page_map.array.get(gbuf->block_id));
    gbuf->cache->page_map.array.set(gbuf->block_id, NULL);
}
