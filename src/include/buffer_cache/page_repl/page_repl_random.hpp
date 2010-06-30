
#ifndef __PAGE_REPL_RANDOM_HPP__
#define __PAGE_REPL_RANDOM_HPP__

template <class config_t>
struct page_repl_random_t {
	public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::page_map_t page_map_t;
    typedef typename config_t::buffer_alloc_t buffer_alloc_t;
    
public:
    page_repl_random_t(size_t _block_size, size_t _max_size,
                     page_map_t *_page_map,
                     buffer_alloc_t *_alloc)
        : block_size(_block_size), max_size(_max_size),
          page_map(_page_map),
          alloc(_alloc)
        {}

    class buf_t {
    public:
        explicit buf_t(page_repl_random_t *_page_repl) : pinned(0) {}

        void pin() {
        	assert(pinned >= 0);
        	pinned++;
        }
        void unpin() {
        	assert(pinned > 0);
        	pinned--;
        }
        unsigned int is_pinned() const {
        	return pinned > 0;
        }

    private:
    	int pinned;
    };

private:
    size_t block_size, max_size;
    page_map_t *page_map;
    buffer_alloc_t *alloc;
};

#endif // __PAGE_REPL_RANDOM_HPP__