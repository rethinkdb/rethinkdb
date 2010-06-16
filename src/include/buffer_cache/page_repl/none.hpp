
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
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::page_map_t page_map_t;
    typedef typename config_t::buffer_alloc_t buffer_alloc_t;
    
public:
    page_repl_none_t(size_t _block_size, size_t _max_size,
                     page_map_t *_page_map,
                     buffer_alloc_t *_alloc)
        : nblocks(0), block_size(_block_size),
          max_size(_max_size),
          page_map(_page_map),
          alloc(_alloc)
        {}

    void pin(block_id_t block_id) {
        nblocks++;
    }

    void unpin(block_id_t block_id) {
        if(block_size * nblocks >= max_size) {
            check("No more cache memory available", 1);
        }
    }

private:
    unsigned int nblocks;
    size_t block_size, max_size;
    page_map_t *page_map;
    buffer_alloc_t *alloc;
};

#endif // __PAGE_REPL_NONE_HPP__

