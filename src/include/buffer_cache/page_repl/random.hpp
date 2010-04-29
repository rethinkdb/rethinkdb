
#ifndef __PAGE_REPL_RANDOM_HPP__
#define __PAGE_REPL_RANDOM_HPP__

// TODO: we might want to decouple selection of pages to free from the
// actual cleaning process (e.g. we might have random vs. lru
// selection strategies, and immediate vs. proactive cleaing
// strategy).

template <class config_t>
struct page_repl_random_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    
public:
    page_repl_random_t(size_t _block_size, size_t _max_size)
        : nblocks(0), block_size(_block_size),
          max_size(_max_size)
        {}

    void pin(block_id_t block_id) {
        nblocks++;
    }

    void unpin(block_id_t block_id) {
        if(block_size * nblocks >= max_size) {
            // Find a random non-dirty block
            // Remove it from the map
            // Free the ram
        }
    }

private:
    unsigned int nblocks;
    size_t block_size, max_size;
};

#endif // __PAGE_REPL_RANDOM_HPP__

