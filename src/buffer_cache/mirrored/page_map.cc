#include "page_map.hpp"
#include "mirrored.hpp"

array_map_t::local_buf_t::local_buf_t(mc_inner_buf_t *gbuf)
    : gbuf(gbuf)
{
    rassert(!gbuf->cache->page_map.array.get(gbuf->block_id));
    gbuf->cache->page_map.array.set(gbuf->block_id, gbuf);
}

array_map_t::local_buf_t::~local_buf_t() {
    rassert(gbuf->cache->page_map.array.get(gbuf->block_id));
    gbuf->cache->page_map.array.set(gbuf->block_id, NULL);
}
