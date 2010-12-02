#include "page_repl_random.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"

page_repl_random_t::page_repl_random_t(unsigned int _unload_threshold, cache_t *_cache)
    : unload_threshold(_unload_threshold),
      cache(_cache)
    {}
    
page_repl_random_t::local_buf_t::local_buf_t(inner_buf_t *gbuf) : gbuf(gbuf) {
    index = gbuf->cache->page_repl.array.size();
    gbuf->cache->page_repl.array.set(index, gbuf);
}
    
page_repl_random_t::local_buf_t::~local_buf_t() {

    unsigned int last_index = gbuf->cache->page_repl.array.size() - 1;

    if (index == last_index) {
        gbuf->cache->page_repl.array.set(index, NULL);

    } else {
        inner_buf_t *replacement = gbuf->cache->page_repl.array.get(last_index);
        replacement->page_repl_buf.index = index;
        gbuf->cache->page_repl.array.set(index, replacement);
        gbuf->cache->page_repl.array.set(last_index, NULL);
        index = -1;
    }
}

// make_space tries to make sure that the number of blocks currently in memory is at least
// 'space_needed' less than the user-specified memory limit.
void page_repl_random_t::make_space(unsigned int space_needed) {
    
    unsigned int target;
    if (space_needed > unload_threshold) target = unload_threshold;
    else target = unload_threshold - space_needed;
            
    while (array.size() > target) {
                
        // Try to find a block we can unload. Blocks are ineligible to be unloaded if they are
        // dirty or in use.
        inner_buf_t *block_to_unload = NULL;
        for (int tries = PAGE_REPL_NUM_TRIES; tries > 0; tries --) {
            
            /* Choose a block in memory at random. */
            unsigned int n = random() % array.size();
            inner_buf_t *block = array.get(n);
            
            if (block->safe_to_unload()) {
                block_to_unload = block;
                break;
            }
        }
        
        if (block_to_unload) {
            //printf("Unloading: %ld\n", block_to_unload->block_id);
            
            /* inner_buf_t's destructor, and the destructors of the local_buf_ts, take care of the
            details */
            delete block_to_unload;
        } else {
            if (array.size() > target + (target / 100) + 10) {
                logDBG("cache %p exceeding memory target. %d blocks in memory, %d dirty, target is %d.\n",
                       cache, array.size(), cache->writeback.num_dirty_blocks(), target);
            }
            break;
        }
    }
}

mc_inner_buf_t *page_repl_random_t::get_first_buf() {
    if (array.size() == 0) return NULL;
    return array.get(0);
}

mc_inner_buf_t *page_repl_random_t::get_next_buf(inner_buf_t *buf) {
    if (buf->page_repl_buf.index == array.size() - 1) return NULL;
    else return array.get(buf->page_repl_buf.index + 1);
}
