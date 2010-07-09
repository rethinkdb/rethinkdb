
#ifndef __PAGE_REPL_NONE_HPP__
#define __PAGE_REPL_NONE_HPP__

// TODO: We should use mlock (or mlockall or related) to make sure the
// OS doesn't swap out our pages, since we're doing swapping
// ourselves.

// TODO: we might want to decouple selection of pages to free from the
// actual cleaning process (e.g. we might have random vs. lru
// selection strategies, and immediate vs. proactive cleaing
// strategy).

// If we run out of cache space, just remove the block we're currently
// unpinning. Really really dumb, but hey, it's a placeholder
template <class config_t>
struct page_repl_none_t {
public:
	typedef typename config_t::cache_t cache_t;
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::page_map_t page_map_t;
    
public:
    page_repl_none_t(int unload_threshold,
                     page_map_t *_page_map,
                     cache_t *_cache)
        : unload_threshold(_unload_threshold)
          page_map(_page_map),
          cache(_cache)
        {}
	
	void make_space(unsigned int space_needed) {
	    // Do nothing. The user-specified limit will likely be exceeded.
	}
	
private:
    int unload_threshold;
    page_map_t *page_map;
    cache_t *cache;
};

#endif // __PAGE_REPL_NONE_HPP__

