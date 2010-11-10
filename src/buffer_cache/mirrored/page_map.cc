#include "page_map.hpp"
#include "mirrored.hpp"

array_map_t::local_buf_t::local_buf_t(mc_buf_t *gbuf)
    : gbuf(gbuf)
{
    assert(!gbuf->cache->page_map.array.get(gbuf->get_block_id()));
    gbuf->cache->page_map.array.set(gbuf->get_block_id(), gbuf);
}

array_map_t::local_buf_t::~local_buf_t() {

    assert(gbuf->cache->page_map.array.get(gbuf->get_block_id()));
    gbuf->cache->page_map.array.set(gbuf->get_block_id(), NULL);
}