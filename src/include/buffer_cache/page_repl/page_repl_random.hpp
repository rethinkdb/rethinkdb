
#ifndef __PAGE_REPL_RANDOM_HPP__
#define __PAGE_REPL_RANDOM_HPP__

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

	void start() {
		int interval_ms = 3000; // TODO: This should be configurable?
		timespec ts;
		ts.tv_sec = interval_ms / 1000;
		ts.tv_nsec = (interval_ms % 1000) * 1000 * 1000;
		get_cpu_context()->event_queue->timer_add(&ts, timer_callback, this);
	}
	void shutdown(sync_callback<config_t> *cb) {
	}
		
private:
    size_t block_size, max_size;
    page_map_t *page_map;
    buffer_alloc_t *alloc;
    cache_t *cache;
    
    static void timer_callback(void *cxt) {
    	static_cast<page_repl_random_t*>(cxt)->consider_unloading();
    }
    
    void consider_unloading() {
    	
    	int num_misses = 0, num_unloaded = 0;
    	int threshold = max_size / block_size * 0.9;
    	
    	int initial_num_blocks = page_map->num_blocks();
    	
    	while (page_map->num_blocks() > threshold) {
    	    		
    		// Try to find a block we can unload. Blocks are ineligible to be unloaded if they are
    		// dirty or in use.
    		int tries = 3;
    		buf_t *block_to_unload = NULL;
    		while (tries > 0 && !block_to_unload) {
    			block_to_unload = page_map->get_random_block();
    			if (block_to_unload->is_dirty() || block_to_unload->lock.locked()) {
    				block_to_unload = NULL;
    				num_misses ++;
    			}
    			tries--;
    		}
    		
    		if (block_to_unload) {
    			num_unloaded ++;
    			cache->do_unload_buf(block_to_unload);
    		} else {
    			break;
    		}
    	}
    	
    	assert(num_unloaded == initial_num_blocks - page_map->num_blocks());
    	/*
    	printf("CPU: %d Prev: %-10d Lim: %-10d Unload: %-10d Miss:%-5d After: %-10d\n",
    		get_cpu_context()->event_queue->queue_id,
    		initial_num_blocks,
    		threshold,
    		num_unloaded,
    		num_misses,
    		page_map->num_blocks());
    	*/
    }
};

#endif // __PAGE_REPL_RANDOM_HPP__
