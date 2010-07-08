
#ifndef __PAGE_REPL_RANDOM_HPP__
#define __PAGE_REPL_RANDOM_HPP__

#include "config/args.hpp"

// TODO: We should use mlock (or mlockall or related) to make sure the
// OS doesn't swap out our pages, since we're doing swapping
// ourselves.

// TODO: we might want to decouple selection of pages to free from the
// actual cleaning process (e.g. we might have random vs. lru
// selection strategies, and immediate vs. proactive cleaing
// strategy).


template <class config_t>
struct page_repl_random_t {
	public:
	typedef typename config_t::cache_t cache_t;
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::page_map_t page_map_t;
    typedef typename config_t::buf_t buf_t;
    
public:
    page_repl_random_t(size_t _block_size, size_t _max_size,
                     page_map_t *_page_map,
                     buffer_alloc_t *_alloc,
                     cache_t *_cache)
        : block_size(_block_size), max_size(_max_size),
          page_map(_page_map),
          alloc(_alloc),
          cache(_cache)
        {}
    
    // make_space tries to make sure that the number of blocks currently in memory is at least
    // 'space_needed' less than the user-specified memory limit.
    void make_space(unsigned int space_needed) {
    	
    	unsigned int target;
    	if (space_needed > max_size / block_size) target = max_size / block_size;
    	else target = max_size / block_size - space_needed;
    	    	
    	while (page_map->num_blocks() > target) {
    	    		
    		// Try to find a block we can unload. Blocks are ineligible to be unloaded if they are
    		// dirty or in use.
    		int tries = 3;
    		buf_t *block_to_unload = NULL;
    		while (tries > 0 && !block_to_unload) {
    			block_to_unload = page_map->get_random_block();
    			assert(block_to_unload);
    			if (block_to_unload->is_dirty() || block_to_unload->is_pinned()) {
    				block_to_unload = NULL;
    			}
    			tries--;
    		}
    		
    		if (block_to_unload) {
    			cache->do_unload_buf(block_to_unload);
    		} else {
    			break;
    		}
    	}
    }
    
private:
    size_t block_size, max_size;
    page_map_t *page_map;
    buffer_alloc_t *alloc;
    cache_t *cache;
};

#endif // __PAGE_REPL_RANDOM_HPP__
