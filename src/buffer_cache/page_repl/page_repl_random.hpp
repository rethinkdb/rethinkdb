
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
    typedef typename config_t::buf_t buf_t;
    
public:
    page_repl_random_t(unsigned int _unload_threshold,
                       cache_t *_cache)
        : unload_threshold(_unload_threshold),
          cache(_cache)
        {}
    
    // make_space tries to make sure that the number of blocks currently in memory is at least
    // 'space_needed' less than the user-specified memory limit.
    void make_space(unsigned int space_needed) {
        
        unsigned int target;
        if (space_needed > unload_threshold) target = unload_threshold;
        else target = unload_threshold - space_needed;
                
        while (cache->num_blocks() > target) {
                    
            // Try to find a block we can unload. Blocks are ineligible to be unloaded if they are
            // dirty or in use.
            buf_t *block_to_unload = NULL;
            for (int tries = PAGE_REPL_NUM_TRIES; tries > 0; tries --) {
                
                /* Choose a block in memory at random. This takes O(N) time. Can we do better? */
                unsigned int n = random() % cache->num_blocks();
                typename intrusive_list_t<buf_t>::iterator it = cache->buffers.begin();
                while (n--) it++;
                buf_t *block = &*it;
                
                if (block->safe_to_unload()) {
                    block_to_unload = block;
                    break;
                }
            }
            
            if (block_to_unload) {
                cache->do_unload_buf(block_to_unload);
            } else {
#ifndef NDEBUG
                printf("thread %d exceeding memory target. %d blocks in memory, %d dirty, target is %d.\n",
                    get_cpu_context()->event_queue->queue_id, cache->num_blocks(), cache->num_dirty_blocks(), target);
#endif
                break;
            }
        }
    }
    
private:
    unsigned int unload_threshold;
    cache_t *cache;
};

#endif // __PAGE_REPL_RANDOM_HPP__
